/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include "ThumbnailerWorker.h"

#include "Media.h"
#include "File.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"
#include "medialibrary/IThumbnailer.h"

#include <algorithm>

namespace medialibrary
{

ThumbnailerWorker::ThumbnailerWorker( MediaLibraryPtr ml,
                                      std::shared_ptr<IThumbnailer> thumbnailer )
    : m_ml( ml )
    , m_run( false )
    , m_generator( std::move( thumbnailer ) )
    , m_paused( false )
{
}

ThumbnailerWorker::~ThumbnailerWorker()
{
    stop();
}

void ThumbnailerWorker::requestThumbnail( MediaPtr media, ThumbnailSizeType sizeType,
                                          uint32_t desiredWidth, uint32_t desiredHeight,
                                          float position )
{
    std::unique_lock<compat::Mutex> lock( m_mutex );

    Task t{
        std::move( media ),
        sizeType,
        desiredWidth,
        desiredHeight,
        position
    };
    m_tasks.push( std::move( t ) );
    if ( m_thread.get_id() == compat::Thread::id{} )
    {
        m_run = true;
        m_thread = compat::Thread( &ThumbnailerWorker::run, this );
    }
    else
        m_cond.notify_all();
}

void ThumbnailerWorker::pause()
{
    std::lock_guard<compat::Mutex> lock( m_mutex );
    m_paused = true;
}

void ThumbnailerWorker::resume()
{
    std::lock_guard<compat::Mutex> lock( m_mutex );
    if ( m_paused == false )
        return;
    m_paused = false;
    m_cond.notify_all();
}

void ThumbnailerWorker::run()
{
    LOG_INFO( "Starting thumbnailer thread" );
    while ( m_run == true )
    {
        ML_UNHANDLED_EXCEPTION_INIT
        {
            Task t;
            {
                std::unique_lock<compat::Mutex> lock( m_mutex );
                if ( m_tasks.size() == 0 || m_paused == true )
                {
                    m_cond.wait( lock, [this]() {
                        return ( m_tasks.size() > 0 && m_paused == false ) ||
                                m_run == false;
                    });
                    if ( m_run == false )
                        break;
                }
                t = std::move( m_tasks.front() );
                m_tasks.pop();
            }
            bool res = generateThumbnail( t );
            m_ml->getCb()->onMediaThumbnailReady( t.media, t.sizeType, res );
        }
        ML_UNHANDLED_EXCEPTION_BODY( "ThumbnailerWorker" )
    }
    LOG_INFO( "Exiting thumbnailer thread" );
}

void ThumbnailerWorker::stop()
{
    bool running = true;
    if ( m_run.compare_exchange_strong( running, false ) )
    {
        m_generator->stop();
        {
            std::unique_lock<compat::Mutex> lock( m_mutex );
            while ( m_tasks.empty() == false )
                m_tasks.pop();
        }
        m_cond.notify_all();
        m_thread.join();
    }
}

bool ThumbnailerWorker::generateThumbnail( Task task )
{
    assert( task.media->type() != Media::Type::Audio );

    const auto files = task.media->files();
    if ( files.empty() == true )
    {
        LOG_WARN( "Can't generate thumbnail for a media without associated files (",
                  task.media->title() );
        return false;
    }
    auto mainFileIt = std::find_if( files.cbegin(), files.cend(),
                                    [](FilePtr f) {
                                        return f->isMain();
                                   });
    if ( mainFileIt == files.cend() )
    {
        assert( !"A media must have a file of type Main" );
        return false;
    }
    auto file = std::static_pointer_cast<File>( *mainFileIt );
    std::string mrl;
    try
    {
        mrl = file->mrl();
    }
    catch ( const fs::errors::DeviceRemoved& )
    {
        LOG_WARN( "Aborting file ", file->rawMrl(), " generation due to its "
                  "containing device being missing" );
        return false;
    }

    auto m = static_cast<Media*>( task.media.get() );
    if ( m->isThumbnailGenerated( task.sizeType ) == false )
    {
        /*
         * Insert a failure record before computing the thumbnail.
         * If the thumbnailer crashes, we don't want to re-run it. If it succeeds,
         * the thumbnail will be updated right after
         * This is done here instead of from the mainloop as we don't want to prevent
         * the thumbnail generation of a file that has been removed.
         *
         * This assumes that the thumbnail won't crash if it succeeded once.
         */
        m->setThumbnail( "", Thumbnail::Origin::Media, task.sizeType, false );
    }
    auto thumbnail = m->thumbnail( task.sizeType );
    if ( thumbnail == nullptr )
    {
        // Handle sporadic read errors gracefully
        assert( !"The thumbnail can't be nullptr as it just was inserted" );
        return false;
    }
    auto dest = Thumbnail::path( m_ml, thumbnail->id() );
    LOG_DEBUG( "Generating ", mrl, " thumbnail in ", dest );

    if ( m_generator->generate( mrl, task.desiredWidth, task.desiredHeight,
                                task.position, dest ) == false )
    {
        if ( m_run == false )
        {
            // The generation failed because the thumbnailer was interrupted.

            // Unlink the media and the thumbnail, otherwise we will have a
            // failure record and no way of regenerating the thumbnail, while
            // the generation was only cancelled, and might not fail at all.
            m->removeThumbnail( task.sizeType );
        }
        return false;
    }

    auto destMrl = utils::file::toMrl( dest );
    return m->setThumbnail( destMrl, Thumbnail::Origin::Media, task.sizeType, true );
}

}
