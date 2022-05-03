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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <cassert>

#include "ModificationsNotifier.h"
#include "MediaLibrary.h"
#include "utils/File.h"
#include "database/SqliteErrors.h"
#include "Common.h"
#include "thumbnails/ThumbnailerWorker.h"

namespace medialibrary
{

const ModificationNotifier::TimeoutChrono ModificationNotifier::ZeroTimeout =
        std::chrono::time_point<std::chrono::steady_clock>{};
const std::chrono::milliseconds ModificationNotifier::BatchDelay =
        std::chrono::milliseconds{ 1000 };

ModificationNotifier::ModificationNotifier( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_cb( ml->getCb() )
    , m_stop( false )
    , m_flushing( false )
    , m_wakeUpScheduled( false )
{
}

ModificationNotifier::~ModificationNotifier()
{
    {
        std::unique_lock<compat::Mutex> lock( m_lock );
        if ( m_notifierThread.joinable() == false )
            return;
        m_stop = true;
    }
    m_cond.notify_all();
    m_notifierThread.join();
}

void ModificationNotifier::start()
{
    assert( m_notifierThread.get_id() == compat::Thread::id{} );
    m_notifierThread = compat::Thread{ &ModificationNotifier::run, this };
}

void ModificationNotifier::notifyMediaCreation( MediaPtr media )
{
    notifyCreation( std::move( media ), m_media );
}

void ModificationNotifier::notifyMediaModification( int64_t media )
{
    notifyModification( media, m_media );
}

void ModificationNotifier::notifyMediaRemoval( int64_t mediaId )
{
    notifyRemoval( mediaId, m_media );
}

void ModificationNotifier::notifyArtistCreation( ArtistPtr artist )
{
    notifyCreation( std::move( artist ), m_artists );
}

void ModificationNotifier::notifyArtistModification( int64_t artist )
{
    notifyModification( artist, m_artists );
}

void ModificationNotifier::notifyArtistRemoval( int64_t artist )
{
    notifyRemoval( artist, m_artists );
}

void ModificationNotifier::notifyAlbumCreation( AlbumPtr album )
{
    notifyCreation( std::move( album ), m_albums );
}

void ModificationNotifier::notifyAlbumModification( int64_t album )
{
    notifyModification( album, m_albums );
}

void ModificationNotifier::notifyAlbumRemoval( int64_t albumId )
{
    notifyRemoval( albumId, m_albums );
}

void ModificationNotifier::notifyPlaylistCreation( PlaylistPtr playlist )
{
    notifyCreation( std::move( playlist ), m_playlists );
}

void ModificationNotifier::notifyPlaylistModification( int64_t playlist )
{
    notifyModification( playlist, m_playlists );
}

void ModificationNotifier::notifyPlaylistRemoval( int64_t playlistId )
{
    notifyRemoval( playlistId, m_playlists );
}

void ModificationNotifier::notifyGenreCreation( GenrePtr genre )
{
    notifyCreation( std::move( genre ), m_genres );
}

void ModificationNotifier::notifyGenreModification( int64_t genre )
{
    notifyModification( genre, m_genres );
}

void ModificationNotifier::notifyGenreRemoval( int64_t genreId )
{
    notifyRemoval( genreId, m_genres );
}

void ModificationNotifier::notifyMediaGroupCreation( MediaGroupPtr mediaGroup )
{
    notifyCreation( std::move( mediaGroup ), m_mediaGroups );
}

void ModificationNotifier::notifyMediaGroupModification( int64_t mediaGroupId )
{
    notifyModification( mediaGroupId, m_mediaGroups );
}

void ModificationNotifier::notifyMediaGroupRemoval( int64_t mediaGroupId )
{
    notifyRemoval( mediaGroupId, m_mediaGroups );
}

void ModificationNotifier::notifyBookmarkCreation( BookmarkPtr bookmark )
{
    notifyCreation( std::move( bookmark ), m_bookmarks );
}

void ModificationNotifier::notifyBookmarkModification(int64_t bookmarkId)
{
    notifyModification( bookmarkId, m_bookmarks );
}

void ModificationNotifier::notifyBookmarkRemoval( int64_t bookmarkId )
{
    notifyRemoval( bookmarkId, m_bookmarks );
}

void ModificationNotifier::notifyFolderCreation( FolderPtr folder )
{
    notifyCreation( std::move( folder ), m_folders );
}

void ModificationNotifier::notifyFolderModification( int64_t folderId )
{
    notifyModification( folderId, m_folders );
}

void ModificationNotifier::notifyFolderRemoval( int64_t folderId )
{
    notifyRemoval( folderId, m_folders );
}

void ModificationNotifier::notifyThumbnailCleanupInserted( int64_t requestId )
{
    /*
     * We are actually notifying an insertion, but the Queue specialization for
     * void (ie without attached instance) only contains a removal queue.
     * This doesn't really matter since all we care about is batching the requests
     * in case multiple thumbnails need to be cleaned up at once, and avoid
     * spamming the thumbnailer from a sqlite hook
     */
    notifyRemoval( requestId, m_thumbnailsCleanupRequests );
}

void ModificationNotifier::flush()
{
    std::unique_lock<compat::Mutex> lock( m_lock );
    m_flushing = true;
    m_cond.notify_all();
    m_flushedCond.wait( lock, [this](){
        return m_flushing == false;
    });
}

void ModificationNotifier::run()
{
    // Create some other queue to swap with the ones that are used
    // by other threads. That way we can release those early and allow
    // more insertions to proceed
    Queue<IMedia> media;
    Queue<IArtist> artists;
    Queue<IAlbum> albums;
    Queue<IPlaylist> playlists;
    Queue<IGenre> genres;
    Queue<IMediaGroup> mediaGroups;
    Queue<IBookmark> bookmarks;
    Queue<IFolder> folders;
    Queue<void> thumbnailsCleanup;

    TimeoutChrono timeout = ZeroTimeout;

    while ( true )
    {
        ML_UNHANDLED_EXCEPTION_INIT
        {
            bool flushing;
            {
                std::unique_lock<compat::Mutex> lock( m_lock );
                if ( m_flushing == true )
                {
                    m_flushing = false;
                    m_flushedCond.notify_all();
                }
                if ( m_stop == true )
                    break;
                if ( timeout == ZeroTimeout )
                {
                    m_cond.wait( lock, [this](){
                        return m_wakeUpScheduled.load( std::memory_order_relaxed ) == true ||
                                m_stop == true || m_flushing == true;
                    });
                    /*
                     * We are woken up because a queue will need to be notified.
                     * The accurate way would be to iterate over all queues and
                     * probe their timeout, but we know that before being woken
                     * up, all queues were empty, so we can take a shortcut and
                     * assume that a single queue was scheduled to be notified,
                     * which will happen in BatchDelay.
                     */
                    bool expected = true;
                    if ( m_wakeUpScheduled.compare_exchange_strong(
                             expected, false ) == true )
                    {
                        timeout = std::chrono::steady_clock::now() + BatchDelay;
                    }
                }
                m_cond.wait_until( lock, timeout, [this]() {
                    return m_stop == true || m_flushing == true;
                });
                if ( m_stop == true )
                    break;
                flushing = m_flushing;
            }

            const auto now = std::chrono::steady_clock::now();
            auto nextTimeout = ZeroTimeout;
            checkQueue( m_media, media, nextTimeout, now, flushing );
            checkQueue( m_artists, artists, nextTimeout, now, flushing );
            checkQueue( m_albums, albums, nextTimeout, now, flushing );
            checkQueue( m_playlists, playlists, nextTimeout, now, flushing );
            checkQueue( m_genres, genres, nextTimeout, now, flushing );
            checkQueue( m_mediaGroups, mediaGroups, nextTimeout, now, flushing );
            checkQueue( m_thumbnailsCleanupRequests, thumbnailsCleanup, nextTimeout, now, flushing );
            checkQueue( m_bookmarks, bookmarks, nextTimeout, now, flushing );
            checkQueue( m_folders, folders, nextTimeout, now, flushing );
            timeout = nextTimeout;

            notify( std::move( media ), &IMediaLibraryCb::onMediaAdded,
                    &IMediaLibraryCb::onMediaModified, &IMediaLibraryCb::onMediaDeleted );
            notify( std::move( artists ), &IMediaLibraryCb::onArtistsAdded,
                    &IMediaLibraryCb::onArtistsModified, &IMediaLibraryCb::onArtistsDeleted );
            notify( std::move( albums ), &IMediaLibraryCb::onAlbumsAdded,
                    &IMediaLibraryCb::onAlbumsModified, &IMediaLibraryCb::onAlbumsDeleted );
            notify( std::move( playlists ), &IMediaLibraryCb::onPlaylistsAdded,
                    &IMediaLibraryCb::onPlaylistsModified, &IMediaLibraryCb::onPlaylistsDeleted );
            notify( std::move( genres ), &IMediaLibraryCb::onGenresAdded,
                    &IMediaLibraryCb::onGenresModified, &IMediaLibraryCb::onGenresDeleted );
            notify( std::move( mediaGroups ), &IMediaLibraryCb::onMediaGroupsAdded,
                    &IMediaLibraryCb::onMediaGroupsModified, &IMediaLibraryCb::onMediaGroupsDeleted );
            notify( std::move( bookmarks ), &IMediaLibraryCb::onBookmarksAdded,
                    &IMediaLibraryCb::onBookmarksModified, &IMediaLibraryCb::onBookmarksDeleted );
            notify( std::move( folders ), &IMediaLibraryCb::onFoldersAdded,
                    &IMediaLibraryCb::onFoldersModified, &IMediaLibraryCb::onFoldersDeleted );

            if ( thumbnailsCleanup.removed.empty() == false )
            {
                auto t = m_ml->thumbnailer();
                if ( t != nullptr )
                    t->requestCleanupRun();
                thumbnailsCleanup.removed.clear();
                thumbnailsCleanup.timeout = ZeroTimeout;
            }
        }
        ML_UNHANDLED_EXCEPTION_BODY( "ModificationNotifier" )
    }
}

}
