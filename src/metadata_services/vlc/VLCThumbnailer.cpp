/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "VLCThumbnailer.h"

#include "AlbumTrack.h"
#include "Album.h"
#include "Artist.h"
#include "Media.h"
#include "File.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/VLCInstance.h"
#include "utils/ModificationsNotifier.h"
#include "metadata_services/vlc/Common.hpp"

#ifdef HAVE_JPEG
#include "imagecompressors/JpegCompressor.h"
#else
#error No image compressor available
#endif

namespace medialibrary
{

VLCThumbnailer::VLCThumbnailer( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_run( false )
    , m_prevSize( 0 )
{
#ifdef HAVE_JPEG
    m_compressor.reset( new JpegCompressor );
#endif
}

VLCThumbnailer::~VLCThumbnailer()
{
    stop();
}

void VLCThumbnailer::requestThumbnail( MediaPtr media )
{
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        auto task = std::unique_ptr<Task>( new Task( std::move( media ) ) );
        m_tasks.push( std::move( task ) );
    }
    if ( m_thread.get_id() == compat::Thread::id{} )
    {
        m_run = true;
        m_thread = compat::Thread( &VLCThumbnailer::run, this );
    }
}

void VLCThumbnailer::run()
{
    LOG_INFO( "Starting thumbnailer thread" );
    while ( m_run == true )
    {
        std::unique_ptr<Task> task;
        {
            std::unique_lock<compat::Mutex> lock( m_mutex );
            if ( m_tasks.size() == 0 )
            {
                m_cond.wait( lock, [this]() { return m_tasks.size() > 0 || m_run == false; } );
                if ( m_run == false )
                    break;
            }
            task = std::move( m_tasks.front() );
            m_tasks.pop();
        }
        bool res = generateThumbnail( *task );
        m_ml->getCb()->onMediaThumbnailReady( task->media, res );
    }
    LOG_INFO( "Exiting thumbnailer thread" );
}

void VLCThumbnailer::stop()
{
    bool running = true;
    if ( m_run.compare_exchange_strong( running, false ) )
    {
        {
            std::unique_lock<compat::Mutex> lock( m_mutex );
            while ( m_tasks.empty() == false )
                m_tasks.pop();
        }
        m_cond.notify_all();
        m_thread.join();
    }
}

bool VLCThumbnailer::generateThumbnail( Task& task )
{
    assert( task.media->type() != Media::Type::Audio );

    auto files = task.media->files();
    if ( files.empty() == true )
    {
        LOG_WARN( "Can't generate thumbnail for a media without associated files (",
                  task.media->title() );
        return false;
    }
    task.mrl = files[0]->mrl();

    LOG_INFO( "Generating ", task.mrl, " thumbnail..." );

    VLC::Media vlcMedia = VLC::Media( VLCInstance::get(), task.mrl,
                                      VLC::Media::FromType::FromLocation );

    vlcMedia.addOption( ":no-audio" );
    vlcMedia.addOption( ":no-osd" );
    vlcMedia.addOption( ":no-spu" );
    vlcMedia.addOption( ":input-fast-seek" );
    vlcMedia.addOption( ":avcodec-hw=none" );
    vlcMedia.addOption( ":no-mkv-preload-local-dir" );
    auto duration = vlcMedia.duration();
    if ( duration > 0 )
    {
        std::ostringstream ss;
        // Duration is in ms, start-time in seconds, and we're aiming at 1/4th of the media
        ss << ":start-time=" << duration / 4000;
        vlcMedia.addOption( ss.str() );
    }

    task.mp = VLC::MediaPlayer( vlcMedia );

    setupVout( task );

    auto res = MetadataCommon::startPlayback( vlcMedia, task.mp );
    if ( res == false )
    {
        LOG_WARN( "Failed to generate ", task.mrl, " thumbnail: Can't start playback" );
        return false;
    }

    if ( duration <= 0 )
    {
        // Seek ahead to have a significant preview
        res = seekAhead( task );
        if ( res == false )
        {
            LOG_WARN( "Failed to generate ", task.mrl, " thumbnail: Failed to seek ahead" );
            return false;
        }
    }
    res = takeThumbnail( task );
    if ( res == false )
        return false;

    LOG_INFO( "Done generating ", task.mrl, " thumbnail" );

    m_ml->getNotifier()->notifyMediaModification( task.media );

    return true;
}

bool VLCThumbnailer::seekAhead( Task& task )
{
    float pos = .0f;
    auto event = task.mp.eventManager().onPositionChanged([&task, &pos](float p) {
        std::unique_lock<compat::Mutex> lock( task.mutex );
        pos = p;
        task.cond.notify_all();
    });
    auto success = false;
    {
        std::unique_lock<compat::Mutex> lock( task.mutex );
        task.mp.setPosition( .4f );
        success = task.cond.wait_for( lock, std::chrono::seconds( 3 ), [&pos]() {
            return pos >= .1f;
        });
    }
    // Since we're locking a mutex for each position changed, let's unregister ASAP
    event->unregister();
    return success;
}

void VLCThumbnailer::setupVout( Task& task )
{
    task.mp.setVideoFormatCallbacks(
        // Setup
        [this, &task](char* chroma, unsigned int* width, unsigned int *height,
                    unsigned int *pitches, unsigned int *lines) {
            strcpy( chroma, m_compressor->fourCC() );

            const float inputAR = (float)*width / *height;

            task.width = DesiredWidth;
            task.height = (float)task.width / inputAR + 1;
            if ( task.height < DesiredHeight )
            {
                // Avoid downscaling too much for really wide pictures
                task.width = inputAR * DesiredHeight;
                task.height = DesiredHeight;
            }
            auto size = task.width * task.height * m_compressor->bpp();
            // If our buffer isn't enough anymore, reallocate a new one.
            if ( size > m_prevSize )
            {
                m_buff.reset( new uint8_t[size] );
                m_prevSize = size;
            }
            *width = task.width;
            *height = task.height;
            *pitches = task.width * m_compressor->bpp();
            *lines = task.height;
            return 1;
        },
        // Cleanup
        nullptr);
    task.mp.setVideoCallbacks(
        // Lock
        [this](void** pp_buff) {
            *pp_buff = m_buff.get();
            return nullptr;
        },
        //unlock
        nullptr,

        //display
        [this, &task](void*) {
            bool expected = true;
            if ( task.thumbnailRequired.compare_exchange_strong( expected, false ) )
            {
                task.cond.notify_all();
            }
        }
    );
}

bool VLCThumbnailer::takeThumbnail( Task& task )
{
    // lock, signal that we want a thumbnail, and wait.
    {
        std::unique_lock<compat::Mutex> lock( task.mutex );
        task.thumbnailRequired = true;
        bool success = task.cond.wait_for( lock, std::chrono::seconds( 15 ), [&task]() {
            // Keep waiting if the vmem thread hasn't restored m_thumbnailRequired to false
            return task.thumbnailRequired == false;
        });
        if ( success == false )
        {
            LOG_WARN( "Timed out while computing ", task.mrl, " snapshot" );
            return false;
        }
    }
    task.mp.stop();
    return compress( task );
}

bool VLCThumbnailer::compress( Task& task )
{
    auto path = m_ml->thumbnailPath();
    path += "/";
    path += std::to_string( task.media->id() ) + "." + m_compressor->extension();

    auto hOffset = task.width > DesiredWidth ? ( task.width - DesiredWidth ) / 2 : 0;
    auto vOffset = task.height > DesiredHeight ? ( task.height - DesiredHeight ) / 2 : 0;

    if ( m_compressor->compress( m_buff.get(), path, task.width, task.height,
                                 DesiredWidth, DesiredHeight, hOffset, vOffset ) == false )
        return false;

    task.media->setThumbnail( path );
    return true;
}

VLCThumbnailer::Task::Task( MediaPtr m )
    : media( std::move( m ) )
    , width( 0 )
    , height( 0 )
    , thumbnailRequired( false )
{
}

}
