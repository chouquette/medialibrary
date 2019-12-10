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

#pragma once

#include <atomic>
#include "compat/ConditionVariable.h"
#include <vector>
#include <chrono>

#include "medialibrary/Types.h"
#include "Types.h"
#include "compat/Thread.h"
#include "compat/Mutex.h"

namespace medialibrary
{

class ModificationNotifier
{
public:
    ModificationNotifier( MediaLibraryPtr ml );
    ~ModificationNotifier();

    void start();
    void notifyMediaCreation( MediaPtr media );
    void notifyMediaModification( int64_t media );
    void notifyMediaRemoval( int64_t media );

    void notifyArtistCreation( ArtistPtr artist );
    void notifyArtistModification( int64_t artist );
    void notifyArtistRemoval( int64_t artist );

    void notifyAlbumCreation( AlbumPtr album );
    void notifyAlbumModification( int64_t album );
    void notifyAlbumRemoval( int64_t albumId );

    void notifyPlaylistCreation( PlaylistPtr playlist );
    void notifyPlaylistModification( int64_t playlist );
    void notifyPlaylistRemoval( int64_t playlistId );

    void notifyGenreCreation( GenrePtr genre );
    void notifyGenreModification( int64_t genre );
    void notifyGenreRemoval( int64_t genreId );

    void notifyMediaGroupCreation( MediaGroupPtr mediaGroup );
    void notifyMediaGroupModification( int64_t mediaGroupId );
    void notifyMediaGroupRemoval( int64_t mediaGroupId );

    void notifyThumbnailRemoval( int64_t thumbnailId );

    /**
     * @brief flush Flushes the notifications queues
     *
     * This will cause all modifications to be sent to the listeners, regardless
     * of timeouts.
     * The function will return once all queues are flushed and notifications
     * are sent.
     */
    void flush();

private:
    void run();
    void notify();

private:
    // Use a dummy type since only partial specialization is allowed.
    template <typename T, typename DUMMY = void>
    struct Queue
    {
        std::vector<std::shared_ptr<T>> added;
        std::vector<int64_t> modified;
        std::vector<int64_t> removed;
        std::chrono::time_point<std::chrono::steady_clock> timeout;
    };

    template <typename DUMMY>
    struct Queue<void, DUMMY>
    {
        std::vector<int64_t> removed;
        std::chrono::time_point<std::chrono::steady_clock> timeout;
    };

    template <typename T, typename AddedCb, typename ModifiedCb, typename RemovedCb>
    void notify( Queue<T>&& queue, AddedCb addedCb, ModifiedCb modifiedCb, RemovedCb removedCb )
    {
        if ( queue.added.size() > 0 )
            (*m_cb.*addedCb)( std::move( queue.added ) );
        if ( queue.modified.size() > 0 )
            (*m_cb.*modifiedCb)( std::move( queue.modified ) );
        if ( queue.removed.size() > 0 )
            (*m_cb.*removedCb)( std::move( queue.removed ) );
        queue.timeout = std::chrono::time_point<std::chrono::steady_clock>{};
    }

    template <typename T>
    void notifyCreation( std::shared_ptr<T> entity, Queue<T>& queue )
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        queue.added.push_back( std::move( entity ) );
        updateTimeout( queue );
    }

    template <typename T>
    void notifyModification( int64_t entity, Queue<T>& queue )
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        queue.modified.push_back( std::move( entity ) );
        updateTimeout( queue );
    }

    template <typename T>
    void notifyRemoval( int64_t rowId, Queue<T>& queue )
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        queue.removed.push_back( rowId );
        updateTimeout( queue );
    }

    template <typename T>
    void updateTimeout( Queue<T>& queue )
    {
        if ( queue.timeout == std::chrono::time_point<std::chrono::steady_clock>{} )
        {
            queue.timeout = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds{ 1000 };
        }
        if ( m_timeout == std::chrono::time_point<std::chrono::steady_clock>{} )
        {
            // If no wake up has been scheduled, or if we need to wake up faster
            // than expected, update the timeout now
            m_timeout = queue.timeout;
            m_cond.notify_all();
        }
    }

    template <typename T>
    void checkQueue( Queue<T>& input, Queue<T>& output,
                     std::chrono::time_point<std::chrono::steady_clock>& nextTimeout,
                     std::chrono::time_point<std::chrono::steady_clock> now )
    {
#if !defined(_LIBCPP_STD_VER) || (_LIBCPP_STD_VER > 11 && !defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR))
        constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
#else
        const auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
#endif
        // If this queue has no timeout setup, there's nothing to do with it.
        if ( input.timeout == ZeroTimeout )
            return;
        // Otherwise, check if this queue is due for signaling now
        if ( input.timeout <= now || m_flushing == true )
        {
            using std::swap;
            swap( input, output );
            assert( input.timeout == ZeroTimeout );
        }
        // Or is we need to schedule it for the next timeout
        else if ( nextTimeout == ZeroTimeout || input.timeout < nextTimeout )
            nextTimeout = input.timeout;
    }

private:
    MediaLibraryPtr m_ml;
    IMediaLibraryCb* m_cb;

    // Queues
    Queue<IMedia> m_media;
    Queue<IArtist> m_artists;
    Queue<IAlbum> m_albums;
    Queue<IPlaylist> m_playlists;
    Queue<IGenre> m_genres;
    Queue<IMediaGroup> m_mediaGroups;
    Queue<void> m_thumbnails;

    // Notifier thread
    compat::Mutex m_lock;
    compat::ConditionVariable m_cond;
    compat::ConditionVariable m_flushedCond;
    compat::Thread m_notifierThread;
    std::atomic_bool m_stop;
    std::chrono::time_point<std::chrono::steady_clock> m_timeout;
    bool m_flushing;
};

}
