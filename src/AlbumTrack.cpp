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

#include "AlbumTrack.h"
#include "Album.h"
#include "Media.h"
#include "database/SqliteTools.h"
#include "logging/Logger.h"

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::PrimaryKeyColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack(DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_mediaId
        >> m_artist
        >> m_genre
        >> m_trackNumber
        >> m_albumId
        >> m_releaseYear
        >> m_discNumber;
}

//FIXME: constify media
AlbumTrack::AlbumTrack( Media* media, unsigned int trackNumber, unsigned int albumId, unsigned int discNumber )
    : m_id( 0 )
    , m_mediaId( media->id() )
    , m_trackNumber( trackNumber )
    , m_albumId( albumId )
    , m_releaseYear( 0 )
    , m_discNumber( discNumber )
{
}

unsigned int AlbumTrack::id() const
{
    return m_id;
}

const std::string&AlbumTrack::artist() const
{
    return m_artist;
}

bool AlbumTrack::setArtist( const std::string& artist )
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name +
            " SET artist = ? WHERE id_track = ?";
    if ( artist == m_artist )
        return true;
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artist, m_id ) == false )
        return false;
    m_artist = artist;
    return true;
}

bool AlbumTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AlbumTrackTable::Name + "("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id INTEGER,"
                "artist TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "release_year UNSIGNED INTEGER,"
                "disc_number UNSIGNED INTEGER,"
                "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name + "(id_media)"
                    " ON DELETE CASCADE, "
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) "
                    " ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create(DBConnection dbConnection, unsigned int albumId, Media* media, unsigned int trackNb, unsigned int discNumber )
{
    auto self = std::make_shared<AlbumTrack>( media, trackNb, albumId, discNumber );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(media_id, track_number, album_id, disc_number) VALUES(?, ?, ?, ?)";
    if ( insert( dbConnection, self, req, media->id(), trackNb, albumId, discNumber ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

const std::string& AlbumTrack::genre()
{
    return m_genre;
}

bool AlbumTrack::setGenre(const std::string& genre)
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name
            + " SET genre = ? WHERE id_track = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, genre, m_id ) == false )
        return false;
    m_genre = genre;
    return true;
}

unsigned int AlbumTrack::trackNumber()
{
    return m_trackNumber;
}

unsigned int AlbumTrack::releaseYear() const
{
    return m_releaseYear;
}

bool AlbumTrack::setReleaseYear(unsigned int year)
{
    if ( m_releaseYear == year )
        return true;
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name +
            " SET release_year = ? WHERE id_track = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, year, m_id ) == false )
        return false;
    m_releaseYear = year;
    return true;
}

unsigned int AlbumTrack::discNumber() const
{
    return m_discNumber;
}

std::shared_ptr<IAlbum> AlbumTrack::album()
{
    if ( m_album == nullptr && m_albumId != 0 )
    {
        m_album = Album::fetch( m_dbConnection, m_albumId );
    }
    return m_album;
}
