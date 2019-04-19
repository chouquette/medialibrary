/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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
# include "config.h"
#endif

#include "VmemThumbnailer.h"

#include "Media.h"
#include "Thumbnail.h"
#include "utils/VLCInstance.h"
#include "metadata_services/vlc/Common.hpp"
#ifdef HAVE_JPEG
#include "imagecompressors/JpegCompressor.h"
#else
#error No image compressor available
#endif

namespace medialibrary
{

VmemThumbnailer::VmemThumbnailer( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_prevSize( 0 )
{
#ifdef HAVE_JPEG
    m_compressor.reset( new JpegCompressor );
#endif
}

bool VmemThumbnailer::generate( MediaPtr media, const std::string& mrl )
{
    VLC::Media vlcMedia = VLC::Media( VLCInstance::get(), mrl,
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

    Task task{ media, mrl };

    task.mp = VLC::MediaPlayer( vlcMedia );

    setupVout( task );

    auto res = MetadataCommon::startPlayback( vlcMedia, task.mp );
    if ( res == false )
    {
        LOG_WARN( "Failed to generate ", mrl, " thumbnail: Can't start playback" );
        return false;
    }

    if ( duration <= 0 )
    {
        // Seek ahead to have a significant preview
        res = seekAhead( task );
        if ( res == false )
        {
            LOG_WARN( "Failed to generate ", mrl, " thumbnail: Failed to seek ahead" );
            return false;
        }
    }
    return takeThumbnail( task );
}

bool VmemThumbnailer::seekAhead( Task& task )
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

void VmemThumbnailer::setupVout( Task& task )
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
        [&task](void*) {
            bool expected = true;
            if ( task.thumbnailRequired.compare_exchange_strong( expected, false ) )
            {
                task.cond.notify_all();
            }
        }
    );
}

bool VmemThumbnailer::takeThumbnail( Task& task )
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

bool VmemThumbnailer::compress( Task& task )
{
    auto path = m_ml->thumbnailPath();
    path += "/";
    path += std::to_string( task.media->id() ) + "." + m_compressor->extension();

    auto hOffset = task.width > DesiredWidth ? ( task.width - DesiredWidth ) / 2 : 0;
    auto vOffset = task.height > DesiredHeight ? ( task.height - DesiredHeight ) / 2 : 0;

    if ( m_compressor->compress( m_buff.get(), path, task.width, task.height,
                                 DesiredWidth, DesiredHeight, hOffset, vOffset ) == false )
        return false;

    auto media = static_cast<Media*>( task.media.get() );
    media->setThumbnail( path, Thumbnail::Origin::Media, true );
    return true;
}

VmemThumbnailer::Task::Task( MediaPtr m, std::string mrl )
    : media( std::move( m ) )
    , mrl( std::move( mrl ) )
    , width( 0 )
    , height( 0 )
    , thumbnailRequired( false )
{
}

}
