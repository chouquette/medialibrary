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
#include "Thumbnail.h"

namespace medialibrary
{

ModificationNotifier::ModificationNotifier( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_cb( ml->getCb() )
    , m_stop( false )
    , m_flushing( false )
{
}

ModificationNotifier::~ModificationNotifier()
{
    if ( m_notifierThread.joinable() == true )
    {
        m_stop = true;
        m_cond.notify_all();
        m_notifierThread.join();
    }
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
    notifyModification( std::move( media ), m_media );
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
    notifyModification( std::move( artist ), m_artists );
}

void ModificationNotifier::notifyArtistRemoval( int64_t artist )
{
    notifyRemoval( std::move( artist ), m_artists );
}

void ModificationNotifier::notifyAlbumCreation( AlbumPtr album )
{
    notifyCreation( std::move( album ), m_albums );
}

void ModificationNotifier::notifyAlbumModification( int64_t album )
{
    notifyModification( std::move( album ), m_albums );
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
    notifyModification( std::move( playlist ), m_playlists );
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
    notifyModification( std::move( genre ), m_genres );
}

void ModificationNotifier::notifyGenreRemoval( int64_t genreId )
{
    notifyRemoval( genreId, m_genres );
}

void ModificationNotifier::notifyThumbnailRemoval( int64_t thumbnailId )
{
    notifyRemoval( thumbnailId, m_thumbnails );
}

void ModificationNotifier::flush()
{
    std::unique_lock<compat::Mutex> lock( m_lock );
    m_timeout = std::chrono::steady_clock::now();
    m_flushing = true;
    m_cond.notify_all();
    m_flushedCond.wait( lock, [this](){
        return m_flushing == false;
    });
}

void ModificationNotifier::run()
{
#if !defined(_LIBCPP_STD_VER) || (_LIBCPP_STD_VER > 11 && !defined(_LIBCPP_HAS_NO_CXX14_CONSTEXPR))
    constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
#else
    const auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};
#endif

    // Create some other queue to swap with the ones that are used
    // by other threads. That way we can release those early and allow
    // more insertions to proceed
    Queue<IMedia> media;
    Queue<IArtist> artists;
    Queue<IAlbum> albums;
    Queue<IPlaylist> playlists;
    Queue<IGenre> genres;
    Queue<void> thumbnails;

    while ( m_stop == false )
    {
        {
            std::unique_lock<compat::Mutex> lock( m_lock );
            if ( m_flushing == true )
            {
                m_flushing = false;
                m_flushedCond.notify_all();
            }
            if ( m_timeout == ZeroTimeout )
            {
                m_cond.wait( lock, [this, ZeroTimeout](){
                    return m_timeout != ZeroTimeout || m_stop == true ||
                           m_flushing == true;
                });
            }
            m_cond.wait_until( lock, m_timeout, [this]() {
                return m_stop == true || m_flushing == true;
            });
            if ( m_stop == true )
                break;
            const auto now = std::chrono::steady_clock::now();
            auto nextTimeout = ZeroTimeout;
            checkQueue( m_media, media, nextTimeout, now );
            checkQueue( m_artists, artists, nextTimeout, now );
            checkQueue( m_albums, albums, nextTimeout, now );
            checkQueue( m_playlists, playlists, nextTimeout, now );
            checkQueue( m_genres, genres, nextTimeout, now );
            checkQueue( m_thumbnails, thumbnails, nextTimeout, now );
            m_timeout = nextTimeout;
        }
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
        for ( auto thumbnailId : thumbnails.removed )
        {
            auto path = Thumbnail::path( m_ml, thumbnailId );
            utils::fs::remove( path );
        }
    }
}

}
