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

#include <algorithm>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::CacheColumn = "id_album";
unsigned int Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(DBConnection dbConnection, sqlite::Row& row)
    : m_dbConnection( dbConnection )
    , m_tracksCached( false )
{
    row >> m_id
        >> m_title
        >> m_artistId
        >> m_releaseYear
        >> m_shortSummary
        >> m_artworkUrl
        >> m_lastSyncDate
        >> m_nbTracks;
}

Album::Album(const std::string& title )
    : m_id( 0 )
    , m_title( title )
    , m_artistId( 0 )
    , m_releaseYear( ~0u )
    , m_lastSyncDate( 0 )
    , m_nbTracks( 0 )
    , m_tracksCached( false )
{
}

Album::Album( const Artist* artist )
    : m_id( 0 )
    , m_artistId( artist->id() )
    , m_releaseYear( ~0u )
    , m_lastSyncDate( 0 )
    , m_nbTracks( 0 )
    , m_tracksCached( false )
{
}

unsigned int Album::id() const
{
    return m_id;
}

const std::string& Album::title() const
{
    return m_title;
}

unsigned int Album::releaseYear() const
{
    if ( m_releaseYear == ~0U )
        return 0;
    return m_releaseYear;
}

bool Album::setReleaseYear( unsigned int date, bool force )
{
    if ( date == m_releaseYear )
        return true;
    if ( force == false )
    {
        if ( m_releaseYear != ~0u && date != m_releaseYear )
        {
            // If we already have set the date back to 0, don't do it again.
            if ( m_releaseYear == 0 )
                return true;
            date = 0;
        }
    }
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET release_year = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
        return false;
    m_releaseYear = date;
    return true;
}

const std::string& Album::shortSummary() const
{
    return m_shortSummary;
}

bool Album::setShortSummary( const std::string& summary )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET short_summary = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Album::artworkUrl() const
{
    return m_artworkUrl;
}

bool Album::setArtworkUrl( const std::string& artworkUrl )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET artwork_url = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkUrl, m_id ) == false )
        return false;
    m_artworkUrl = artworkUrl;
    return true;
}

time_t Album::lastSyncDate() const
{
    return m_lastSyncDate;
}

std::vector<MediaPtr> Album::tracks() const
{
    std::lock_guard<std::mutex> lock( m_tracksLock );
    if ( m_tracksCached == true )
        return m_tracks;
    static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            " LEFT JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? ORDER BY att.disc_number, att.track_number";

    m_tracks = Media::fetchAll( m_dbConnection, req, m_id );
    m_tracksCached = true;
    return m_tracks;
}

std::shared_ptr<AlbumTrack> Album::addTrack(std::shared_ptr<Media> media, unsigned int trackNb, unsigned int discNumber )
{
    //FIXME: This MUST be executed as a transaction
    auto track = AlbumTrack::create( m_dbConnection, m_id, media.get(), trackNb, discNumber );
    if ( track == nullptr )
        return nullptr;
    if ( media->setAlbumTrack( track ) == false )
        return nullptr;
    static const std::string req = "UPDATE " + policy::AlbumTable::Name +
            " SET nb_tracks = nb_tracks + 1 WHERE id_album = ?";
    std::lock_guard<std::mutex> lock( m_tracksLock );
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, m_id ) == false )
        return nullptr;
    m_nbTracks++;
    m_tracks.push_back( std::move( media ) );
    std::sort( begin( m_tracks ), end( m_tracks ), [](const MediaPtr& a, const MediaPtr& b) {
        return a->albumTrack()->trackNumber() < b->albumTrack()->trackNumber();
    });
    m_tracksCached = true;
    return track;
}

unsigned int Album::nbTracks() const
{
    return m_nbTracks;
}

ArtistPtr Album::albumArtist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    return Artist::fetch( m_dbConnection, m_artistId );
}

bool Album::setAlbumArtist( Artist* artist )
{
    if ( m_artistId == artist->id() )
        return true;
    if ( artist->id() == 0 )
        return false;
    static const std::string req = "UPDATE " + policy::AlbumTable::Name + " SET "
            "artist_id = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artist->id(), m_id ) == false )
        return false;
    if ( m_artistId != 0 )
    {
        auto previousArtist = Artist::fetch( m_dbConnection, m_artistId );
        previousArtist->updateNbAlbum( -1 );
    }
    m_artistId = artist->id();
    artist->updateNbAlbum( 1 );
    return true;
}

std::vector<ArtistPtr> Album::artists() const
{
    static const std::string req = "SELECT art.* FROM " + policy::ArtistTable::Name + " art "
            "LEFT JOIN AlbumArtistRelation aar ON aar.id_artist = art.id_artist "
            "WHERE aar.id_album = ?";
    return Artist::fetchAll( m_dbConnection, req, m_id );
}

bool Album::addArtist( std::shared_ptr<Artist> artist )
{
    static const std::string req = "INSERT OR IGNORE INTO AlbumArtistRelation VALUES(?, ?)";
    if ( m_id == 0 || artist->id() == 0 )
    {
        LOG_ERROR("Both artist & album need to be inserted in database before being linked together" );
        return false;
    }
    return sqlite::Tools::executeRequest( m_dbConnection, req, m_id, artist->id() );
}

bool Album::removeArtist(Artist* artist)
{
    static const std::string req = "DELETE FROM AlbumArtistRelation WHERE id_album = ? "
            "AND id_artist = ?";
    return sqlite::Tools::executeDelete( m_dbConnection, req, m_id, artist->id() );
}

bool Album::createTable(DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT,"
                "artist_id UNSIGNED INTEGER,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_url TEXT,"
                "last_sync_date UNSIGNED INTEGER,"
                "nb_tracks UNSIGNED INTEGER DEFAULT 0"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS AlbumArtistRelation("
                "id_album INTEGER,"
                "id_artist INTEGER,"
                "PRIMARY KEY (id_album, id_artist),"
                "FOREIGN KEY(id_album) REFERENCES " + policy::AlbumTable::Name + "("
                    + policy::AlbumTable::CacheColumn + ") ON DELETE CASCADE,"
                "FOREIGN KEY(id_artist) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::CacheColumn + ") ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel );
}

std::shared_ptr<Album> Album::create(DBConnection dbConnection, const std::string& title )
{
    auto album = std::make_shared<Album>( title );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, title) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, album, req, title ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}

std::shared_ptr<Album> Album::createUnknownAlbum( DBConnection dbConnection, const Artist* artist )
{
    auto album = std::make_shared<Album>( artist );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, artist_id) VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, album, req, artist->id() ) == false )
        return nullptr;
    album->m_dbConnection = dbConnection;
    return album;
}
