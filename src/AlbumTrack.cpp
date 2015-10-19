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
const std::string policy::AlbumTrackTable::CacheColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
    , m_album( nullptr )
{
    m_id = sqlite::Traits<unsigned int>::Load( stmt, 0 );
    m_mediaId = sqlite::Traits<unsigned int>::Load( stmt, 1 );
    m_title = sqlite::Traits<std::string>::Load( stmt, 2 );
    m_genre = sqlite::Traits<std::string>::Load( stmt, 3 );
    m_trackNumber = sqlite::Traits<unsigned int>::Load( stmt, 4 );
    m_albumId = sqlite::Traits<unsigned int>::Load( stmt, 5 );
}

//FIXME: constify media
AlbumTrack::AlbumTrack( Media* media, unsigned int trackNumber, unsigned int albumId )
    : m_id( 0 )
    , m_mediaId( media->id() )
    , m_title( media->name() )
    , m_trackNumber( trackNumber )
    , m_albumId( albumId )
    , m_album( nullptr )
{
}

unsigned int AlbumTrack::id() const
{
    return m_id;
}

bool AlbumTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AlbumTrackTable::Name + "("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id INTEGER,"
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name + "(id_media)"
                    " ON DELETE CASCADE, "
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) "
                    " ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create(DBConnection dbConnection, unsigned int albumId, Media* media, unsigned int trackNb)
{
    auto self = std::make_shared<AlbumTrack>( media, trackNb, albumId );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(media_id, title, track_number, album_id) VALUES(?, ?, ?, ?)";
    if ( _Cache::insert( dbConnection, self, req, media->id(), media->name(), trackNb, albumId ) == false )
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
            + " SET genre = ? WHERE id_track = ? ";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, genre, m_id ) == false )
        return false;
    m_genre = genre;
    return true;
}

const std::string& AlbumTrack::title()
{
    return m_title;
}

unsigned int AlbumTrack::trackNumber()
{
    return m_trackNumber;
}

std::shared_ptr<IAlbum> AlbumTrack::album()
{
    if ( m_album == nullptr && m_albumId != 0 )
    {
        m_album = Album::fetch( m_dbConnection, m_albumId );
    }
    return m_album;
}

std::vector<MediaPtr> AlbumTrack::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name
            + " WHERE album_track_id = ? ";
    return Media::fetchAll( m_dbConnection, req, m_id );
}
