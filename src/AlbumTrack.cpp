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
#include "Media.h"
#include "Genre.h"
#include "database/SqliteTools.h"
#include "logging/Logger.h"

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::PrimaryKeyColumn = "id_track";
unsigned int AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_mediaId
        >> m_artistId
        >> m_genreId
        >> m_trackNumber
        >> m_albumId
        >> m_releaseYear
        >> m_discNumber
        >> m_isPresent;
}

AlbumTrack::AlbumTrack( unsigned int mediaId, unsigned int trackNumber, unsigned int albumId, unsigned int discNumber )
    : m_id( 0 )
    , m_mediaId( mediaId )
    , m_trackNumber( trackNumber )
    , m_albumId( albumId )
    , m_releaseYear( 0 )
    , m_discNumber( discNumber )
    , m_isPresent( true )
{
}

unsigned int AlbumTrack::id() const
{
    return m_id;
}

ArtistPtr AlbumTrack::artist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    auto lock = m_artist.lock();
    if ( m_artist.isCached() == false )
    {
        m_artist = Artist::fetch( m_dbConnection, m_artistId );
    }
    return m_artist.get();
}

bool AlbumTrack::setArtist( std::shared_ptr<Artist> artist )
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name +
            " SET artist_id = ? WHERE id_track = ?";
    if ( artist->id() == m_artistId )
        return true;
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artist->id(), m_id ) == false )
        return false;
    m_artistId = artist->id();
    m_artist = artist;
    return true;
}

bool AlbumTrack::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AlbumTrackTable::Name + "("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id INTEGER,"
                "artist_id UNSIGNED INTEGER,"
                "genre_id INTEGER,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "release_year UNSIGNED INTEGER,"
                "disc_number UNSIGNED INTEGER,"
                "is_present BOOLEAN NOT NULL DEFAULT 1,"
                "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name + "(id_media)"
                    " ON DELETE CASCADE,"
                "FOREIGN KEY (artist_id) REFERENCES " + policy::ArtistTable::Name + "(id_artist)"
                    " ON DELETE CASCADE,"
                "FOREIGN KEY (genre_id) REFERENCES " + policy::GenreTable::Name + "(id_genre),"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) "
                    " ON DELETE CASCADE"
            ")";
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_track_present AFTER UPDATE OF is_present "
            "ON " + policy::MediaTable::Name +
            " BEGIN"
            " UPDATE " + policy::AlbumTrackTable::Name + " SET is_present = new.is_present WHERE media_id = new.id_media;"
            " END";
    static const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::AlbumTrackTable::Name + "Fts USING FTS3("
                "title"
            ")";
    static const std::string vtableTrigger = "CREATE TRIGGER IF NOT EXISTS delete_fts_track"
            " AFTER DELETE ON " + policy::MediaTable::Name +
            " BEGIN"
            " DELETE FROM " + policy::AlbumTrackTable::Name + "Fts WHERE rowid = old.id_media;"
            " END";
    static const std::string vtableTrigger2 = "CREATE TRIGGER IF NOT EXISTS update_fts_track"
              " AFTER UPDATE OF title ON " + policy::MediaTable::Name +
              " BEGIN"
              " UPDATE " + policy::AlbumTrackTable::Name + "Fts SET title = new.title WHERE rowid = new.id_media;"
              " END";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, vtableReq ) &&
            sqlite::Tools::executeRequest( dbConnection, vtableTrigger ) &&
            sqlite::Tools::executeRequest( dbConnection, vtableTrigger2 );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create( DBConnection dbConnection, unsigned int albumId, const Media& media, unsigned int trackNb, unsigned int discNumber )
{
    auto self = std::make_shared<AlbumTrack>( media.id(), trackNb, albumId, discNumber );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(media_id, track_number, album_id, disc_number) VALUES(?, ?, ?, ?)";
    if ( insert( dbConnection, self, req, media.id(), trackNb, albumId, discNumber ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    static const std::string ftsReq = "INSERT INTO " + policy::AlbumTrackTable::Name + "Fts"
            "(rowid, title) VALUES(?, ?)";
    sqlite::Tools::insert( dbConnection, ftsReq, self->m_id, media.title() );
    return self;
}

AlbumTrackPtr AlbumTrack::fromMedia( DBConnection dbConnection, unsigned int mediaId )
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTrackTable::Name +
            " WHERE media_id = ?";
    return fetch( dbConnection, req, mediaId );
}

std::vector<AlbumTrackPtr> AlbumTrack::fromGenre( DBConnection dbConn, unsigned int genreId )
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTrackTable::Name +
            " WHERE genre_id = ?";
    return fetchAll<IAlbumTrack>( dbConn, req, genreId );
}

std::vector<MediaPtr> AlbumTrack::search( DBConnection dbConn, const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE"
            " id_media IN (SELECT rowid FROM " + policy::AlbumTrackTable::Name + "Fts"
            " WHERE title MATCH ?)";
    return Media::fetchAll<IMedia>( dbConn, req, title + "*" );
}

GenrePtr AlbumTrack::genre()
{
    auto l = m_genre.lock();
    if ( m_genre.isCached() == false )
    {
        m_genre = Genre::fetch( m_dbConnection, m_genreId );
    }
    return m_genre.get();
}

bool AlbumTrack::setGenre( std::shared_ptr<Genre> genre )
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name
            + " SET genre_id = ? WHERE id_track = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req,
                                       sqlite::ForeignKey( genre != nullptr ? genre->id() : 0 ),
                                       m_id ) == false )
        return false;
    auto l = m_genre.lock();
    m_genre = genre;
    m_genreId = genre != nullptr ? genre->id() : 0;
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
    auto album = m_album.lock();
    if ( album == nullptr && m_albumId != 0 )
    {
        album = Album::fetch( m_dbConnection, m_albumId );
        m_album = album;
    }
    return album;
}
