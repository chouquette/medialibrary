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
#include "Artist.h"
#include "File.h"
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
    m_title = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_genre = sqlite::Traits<std::string>::Load( stmt, 2 );
    m_trackNumber = sqlite::Traits<unsigned int>::Load( stmt, 3 );
    m_albumId = sqlite::Traits<unsigned int>::Load( stmt, 4 );
}

AlbumTrack::AlbumTrack( const std::string& title, unsigned int trackNumber, unsigned int albumId )
    : m_id( 0 )
    , m_title( title )
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
                "title TEXT,"
                "genre TEXT,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) ON DELETE CASCADE"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS TrackArtistRelation("
                "id_track INTEGER NOT NULL,"
                "id_artist INTEGER,"
                "PRIMARY KEY (id_track, id_artist),"
                "FOREIGN KEY(id_track) REFERENCES " + policy::AlbumTrackTable::Name + "("
                    + policy::AlbumTrackTable::CacheColumn + ") ON DELETE CASCADE,"
                "FOREIGN KEY(id_artist) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::CacheColumn + ") ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create(DBConnection dbConnection, unsigned int albumId, const std::string& name, unsigned int trackNb)
{
    auto self = std::make_shared<AlbumTrack>( name, trackNb, albumId );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(title, track_number, album_id) VALUES(?, ?, ?)";
    if ( _Cache::insert( dbConnection, self, req, name, trackNb, albumId ) == false )
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

bool AlbumTrack::destroy()
{
    // Manually remove Files from cache, and let foreign key handling delete them from the DB
    auto fs = files();
    if ( fs.size() == 0 )
        LOG_WARN( "No files found for AlbumTrack ", m_id );
    for ( auto& f : fs )
    {
        // Ignore failures to discard from cache, we might want to discard records from
        // cache in a near future to avoid running out of memory on mobile devices
        File::discard( std::static_pointer_cast<File>( f ) );
    }
    return _Cache::destroy( m_dbConnection, this );
}

bool AlbumTrack::addArtist( ArtistPtr artist )
{
    static const std::string req = "INSERT INTO TrackArtistRelation VALUES(?, ?)";
    // If track's ID is 0, the request will fail due to table constraints
    sqlite::ForeignKey artistForeignKey( artist != nullptr ? artist->id() : 0 );
    return sqlite::Tools::executeRequest( m_dbConnection, req, m_id, artistForeignKey );
}

std::vector<ArtistPtr> AlbumTrack::artists() const
{
    static const std::string req = "SELECT art.* FROM " + policy::ArtistTable::Name + " art "
            "LEFT JOIN TrackArtistRelation tar ON tar.id_artist = art.id_artist "
            "WHERE tar.id_track = ?";
    return Artist::fetchAll( m_dbConnection, req, m_id );
}

std::vector<FilePtr> AlbumTrack::files()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE album_track_id = ? ";
    return File::fetchAll( m_dbConnection, req, m_id );
}
