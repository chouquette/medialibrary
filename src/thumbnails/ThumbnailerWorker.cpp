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
#include "utils/File.h"
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

void ThumbnailerWorker::requestThumbnailInternal( int64_t mediaId, MediaPtr media,
                                                  ThumbnailSizeType sizeType,
                                                  uint32_t desiredWidth,
                                                  uint32_t desiredHeight,
                                                  float position )
{
    std::unique_lock<compat::Mutex> lock( m_mutex );

    if ( m_queuedMedia.find( mediaId ) != cend( m_queuedMedia ) )
        return;

    Task t{
        mediaId,
        std::move( media ),
        sizeType,
        desiredWidth,
        desiredHeight,
        position
    };
    m_queuedMedia.insert( mediaId );
    m_tasks.push( std::move( t ) );
    assert( m_tasks.size() == m_queuedMedia.size() );
    if ( m_thread.get_id() == compat::Thread::id{} )
    {
        m_run = true;
        m_thread = compat::Thread( &ThumbnailerWorker::run, this );
    }
    else
        m_cond.notify_all();
}

void ThumbnailerWorker::runCleanupRequests()
{
    auto requests = Thumbnail::fetchCleanups( m_ml );
    for ( const auto& r : requests )
    {
        auto path = m_ml->thumbnailPath() + r.second;
        LOG_DEBUG( "Running cleanup request #", r.first, ": removing ", path );
        if ( !utils::fs::remove( path ) )
        {
            try
            {
                utils::fs::fileSize( path );
                /*
                 * If we can compute the file size, the file still exists so we
                 * should try to delete it again later
                 */
                continue;
            }
            catch ( const medialibrary::fs::errors::Exception& )
            {
                /*
                 * Let's assume the file doesn't exist anymore so we can still
                 * remove the cleanup request.
                 */
            }
        }
        if ( Thumbnail::removeCleanupRequest( m_ml, r.first ) == false )
            LOG_WARN( "Failed to remove thumbnail cleanup request #", r.first );
    }
}

void ThumbnailerWorker::requestThumbnail( int64_t mediaId, ThumbnailSizeType sizeType,
                                          uint32_t desiredWidth, uint32_t desiredHeight,
                                          float position )
{
    requestThumbnailInternal( mediaId, nullptr, sizeType, desiredWidth, desiredHeight, position );
}

void ThumbnailerWorker::requestCleanupRun()
{
    requestThumbnailInternal( 0, nullptr, ThumbnailSizeType::Thumbnail,
                              0, 0, 0.f );
}

void ThumbnailerWorker::requestThumbnail( MediaPtr media, ThumbnailSizeType sizeType,
                                          uint32_t desiredWidth, uint32_t desiredHeight,
                                          float position )
{
    /* Call media->id() before moving media */
    int64_t mediaId = media->id();
    requestThumbnailInternal( mediaId, std::move(media), sizeType, desiredWidth, desiredHeight, position );
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
                m_cond.wait( lock, [this]() {
                    return ( m_tasks.empty() == false && m_paused == false ) ||
                            m_run == false;
                });
                if ( m_run == false )
                    break;
                t = std::move( m_tasks.front() );
                m_tasks.pop();
                m_queuedMedia.erase( t.mediaId );
                assert( m_tasks.size() == m_queuedMedia.size() );
            }
            if ( t.mediaId == 0 )
                runCleanupRequests();
            if ( t.media == nullptr )
            {
                t.media = m_ml->media( t.mediaId );
                if ( t.media == nullptr )
                    /* No media found with this id */
                    continue;
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
            m_queuedMedia.clear();
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
                                    [](const FilePtr& f) {
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
    bool isFirstThumbnailGeneration = false;
    if ( m->thumbnailStatus( task.sizeType ) == ThumbnailStatus::Missing )
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
        m->setThumbnail( std::make_shared<Thumbnail>( m_ml, ThumbnailStatus::Crash,
                                                      Thumbnail::Origin::Media,
                                                      task.sizeType ) );
        isFirstThumbnailGeneration = true;
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

    if ( m_generator->generate( *m, mrl, task.desiredWidth, task.desiredHeight,
                                task.position, dest ) == false )
    {
        if ( m_run == false )
        {
            /*
             * The generation failed because the thumbnailer was interrupted.
             *
             * If we were trying to generate the first thumbnail for this media
             * we need to remove the record, as there were no crashes, and we
             * don't want to report that information to the user.
             * Otherwise, just keep the previous thumbnail.
             */
            if ( isFirstThumbnailGeneration == true )
                m->removeThumbnail( task.sizeType );
        }
        else
        {
            // Otherwise, ensure the status is "Failure" (since getting here means
            // there was no crash) and bump the number of attempt
            thumbnail->markFailed();
        }
        return false;
    }

    auto destMrl = utils::file::toMrl( dest );
    /*
     * Even if we had a thumbnail before, we still might need to update its
     * status, so we still invoke setThumbnail and let it decide what needs
     * to be updated in db
     */
    return m->setThumbnail( std::make_shared<Thumbnail>( m_ml, std::move( destMrl ),
                        Thumbnail::Origin::Media, task.sizeType, true ) );
}

}
