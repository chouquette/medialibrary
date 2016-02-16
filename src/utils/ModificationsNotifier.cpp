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

#include <cassert>

#include "ModificationsNotifier.h"
#include "MediaLibrary.h"

ModificationNotifier::ModificationNotifier( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_cb( ml->getCb() )
    , m_stop( false )
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
    assert( m_notifierThread.get_id() == std::thread::id{} );
    m_notifierThread = std::thread{ &ModificationNotifier::run, this };
}

void ModificationNotifier::notifyMediaCreation( MediaPtr media )
{
    notifyCreation( std::move( media ), m_media );
}

void ModificationNotifier::notifyMediaModification( MediaPtr media )
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

void ModificationNotifier::notifyArtistModification( ArtistPtr artist )
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

void ModificationNotifier::notifyAlbumModification( AlbumPtr album )
{
    notifyModification( std::move( album ), m_albums );
}

void ModificationNotifier::notifyAlbumRemoval( int64_t albumId )
{
    notifyRemoval( albumId, m_albums );
}

void ModificationNotifier::notifyAlbumTrackCreation( AlbumTrackPtr track )
{
    notifyCreation( std::move( track ), m_tracks );
}

void ModificationNotifier::notifyAlbumTrackModification( AlbumTrackPtr track )
{
    notifyModification( std::move( track ), m_tracks );
}

void ModificationNotifier::notifyAlbumTrackRemoval( int64_t trackId )
{
    notifyRemoval( trackId, m_tracks );
}

void ModificationNotifier::run()
{
    constexpr auto ZeroTimeout = std::chrono::time_point<std::chrono::steady_clock>{};

    // Create some other queue to swap with the ones that are used
    // by other threads. That way we can release those early and allow
    // more insertions to proceed
    Queue<IMedia> media;
    Queue<IArtist> artists;
    Queue<IAlbum> albums;
    Queue<IAlbumTrack> tracks;

    while ( m_stop == false )
    {
        {
            std::unique_lock<std::mutex> lock( m_lock );
            if ( m_timeout == ZeroTimeout )
                m_cond.wait( lock, [this, ZeroTimeout](){ return m_timeout != ZeroTimeout || m_stop == true; } );
            m_cond.wait_until( lock, m_timeout, [this]() { return m_stop == true; });
            if ( m_stop == true )
                break;
            auto now = std::chrono::steady_clock::now();
            auto nextTimeout = ZeroTimeout;
            checkQueue( m_media, media, nextTimeout, now );
            checkQueue( m_artists, artists, nextTimeout, now );
            checkQueue( m_albums, albums, nextTimeout, now );
            checkQueue( m_tracks, tracks, nextTimeout, now );
            m_timeout = nextTimeout;
        }
        notify( std::move( media ), &IMediaLibraryCb::onMediaAdded, &IMediaLibraryCb::onMediaUpdated, &IMediaLibraryCb::onMediaDeleted );
        notify( std::move( artists ), &IMediaLibraryCb::onArtistsAdded, &IMediaLibraryCb::onArtistsModified, &IMediaLibraryCb::onArtistsDeleted );
        notify( std::move( albums ), &IMediaLibraryCb::onAlbumsAdded, &IMediaLibraryCb::onAlbumsModified, &IMediaLibraryCb::onAlbumsDeleted );
        // We pass the onTrackAdded callback twice, to avoid having to do some nifty templates specialization
        // for nullptr callbacks. There is no onTracksModified callback, as tracks are never modified.
        notify( std::move( tracks ), &IMediaLibraryCb::onTracksAdded, &IMediaLibraryCb::onTracksAdded, &IMediaLibraryCb::onTracksDeleted );
    }
}
