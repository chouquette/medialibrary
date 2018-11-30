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

#include "ThumbnailerWorker.h"

#include "Media.h"
#include "File.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "utils/ModificationsNotifier.h"

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

void ThumbnailerWorker::requestThumbnail( MediaPtr media )
{
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_tasks.push( std::move( media ) );
    }
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
        MediaPtr media;
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
            media = std::move( m_tasks.front() );
            m_tasks.pop();
        }
        bool res = generateThumbnail( media );
        m_ml->getCb()->onMediaThumbnailReady( media, res );
    }
    LOG_INFO( "Exiting thumbnailer thread" );
}

void ThumbnailerWorker::stop()
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

bool ThumbnailerWorker::generateThumbnail( MediaPtr media )
{
    assert( media->type() != Media::Type::Audio );

    const auto files = media->files();
    if ( files.empty() == true )
    {
        LOG_WARN( "Can't generate thumbnail for a media without associated files (",
                  media->title() );
        return false;
    }
    auto mainFileIt = std::find_if( cbegin( files ), cend( files ),
                                    [](FilePtr f) {
                                        return f->type() == IFile::Type::Main;
                                   });
    assert( mainFileIt != cend( files ) );
    auto mrl = (*mainFileIt)->mrl();

    LOG_INFO( "Generating ", mrl, " thumbnail..." );
    if ( m_generator->generate( media, mrl ) == false )
        return false;

    m_ml->getNotifier()->notifyMediaModification( media );
    return true;
}

}
