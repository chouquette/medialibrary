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

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::CacheColumn = "id_album";
unsigned int Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(DBConnection dbConnection, sqlite3_stmt* stmt)
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_title = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_releaseDate = sqlite3_column_int( stmt, 2 );
    m_shortSummary = sqlite::Traits<std::string>::Load( stmt, 3 );
    m_artworkUrl = sqlite::Traits<std::string>::Load( stmt, 4 );
    m_lastSyncDate = sqlite3_column_int( stmt, 5 );
}

Album::Album(const std::string& title )
    : m_id( 0 )
    , m_title( title )
    , m_releaseDate( 0 )
    , m_lastSyncDate( 0 )
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

time_t Album::releaseDate() const
{
    return m_releaseDate;
}

bool Album::setReleaseDate( time_t date )
{
    static const std::string& req = "UPDATE " + policy::AlbumTable::Name
            + " SET release_date = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, date, m_id ) == false )
        return false;
    m_releaseDate = date;
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
    static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            " LEFT JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ?";
    return Media::fetchAll( m_dbConnection, req, m_id );
}

std::shared_ptr<AlbumTrack> Album::addTrack(std::shared_ptr<Media> media, unsigned int trackNb )
{
    auto track = AlbumTrack::create( m_dbConnection, m_id, media.get(), trackNb );
    media->setAlbumTrack( track );
    return track;
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
    static const std::string req = "INSERT INTO AlbumArtistRelation VALUES(?, ?)";
    if ( m_id == 0 || artist->id() == 0 )
    {
        LOG_ERROR("Both artist * album need to be inserted in database before being linked together" );
        return false;
    }
    return sqlite::Tools::executeRequest( m_dbConnection, req, m_id, artist->id() );
}

bool Album::destroy()
{
    return _Cache::destroy( m_dbConnection, this );
}

bool Album::createTable(DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT UNIQUE ON CONFLICT FAIL,"
                "release_date UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_url TEXT,"
                "UNSIGNED INTEGER last_sync_date"
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
