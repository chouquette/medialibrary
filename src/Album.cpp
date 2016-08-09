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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <algorithm>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "medialibrary/IGenre.h"
#include "Media.h"

#include "database/SqliteTools.h"

namespace medialibrary
{

const std::string policy::AlbumTable::Name = "Album";
const std::string policy::AlbumTable::PrimaryKeyColumn = "id_album";
int64_t Album::* const policy::AlbumTable::PrimaryKey = &Album::m_id;

Album::Album(MediaLibraryPtr ml, sqlite::Row& row)
    : m_ml( ml )
{
    row >> m_id
        >> m_title
        >> m_artistId
        >> m_releaseYear
        >> m_shortSummary
        >> m_artworkMrl
        >> m_nbTracks
        >> m_duration
        >> m_isPresent;
}

Album::Album( MediaLibraryPtr ml, const std::string& title )
    : m_ml( ml )
    , m_id( 0 )
    , m_title( title )
    , m_artistId( 0 )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_duration( 0 )
    , m_isPresent( true )
{
}

Album::Album( MediaLibraryPtr ml, const Artist* artist )
    : m_ml( ml )
    , m_id( 0 )
    , m_artistId( artist->id() )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_duration( 0 )
    , m_isPresent( true )
{
}

int64_t Album::id() const
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
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, date, m_id ) == false )
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
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET short_summary = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

const std::string& Album::artworkMrl() const
{
    return m_artworkMrl;
}

bool Album::setArtworkMrl( const std::string& artworkMrl )
{
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET artwork_mrl = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

std::string Album::orderTracksBy( SortingCriteria sort, bool desc )
{
    std::string req = " ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::Alpha:
        req += "med.title";
        break;
    case SortingCriteria::Duration:
        req += "med.duration";
        break;
    case SortingCriteria::ReleaseDate:
        req += "med.release_date";
        break;
    default:
        if ( desc == true )
            req += "att.disc_number DESC, att.track_number DESC, med.filename";
        else
            req += "att.disc_number, att.track_number, med.filename";
        break;
    }

    if ( desc == true )
        req += " DESC";
    return req;
}

std::string Album::orderBy( SortingCriteria sort, bool desc )
{
    std::string req = " ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::ReleaseDate:
        if ( desc == true )
            req += "release_year DESC, title";
        else
            req += "release_year, title";
        break;
    case SortingCriteria::Duration:
        req += "duration";
        if ( desc == true )
            req += " DESC";
        break;
    default:
        req += "title";
        if ( desc == true )
            req += " DESC";
        break;
    }
    return req;
}

std::vector<MediaPtr> Album::tracks( SortingCriteria sort, bool desc ) const
{
    // This doesn't return the cached version, because it would be fairly complicated, if not impossible or
    // counter productive, to maintain a cache that respects all orderings.
    std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
        " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
        " WHERE att.album_id = ? AND med.is_present = 1";
    req += orderTracksBy( sort, desc );
    return Media::fetchAll<IMedia>( m_ml, req, m_id );
}

std::vector<MediaPtr> Album::tracks( GenrePtr genre, SortingCriteria sort, bool desc ) const
{
    if ( genre == nullptr )
        return {};
    std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? AND med.is_present = 1"
            " AND genre_id = ?";
    req += orderTracksBy( sort, desc );
    return Media::fetchAll<IMedia>( m_ml, req, m_id, genre->id() );
}

std::vector<MediaPtr> Album::cachedTracks() const
{
    auto lock = m_tracks.lock();
    if ( m_tracks.isCached() == false )
        m_tracks = tracks( SortingCriteria::Default, false );
    return m_tracks.get();
}

std::shared_ptr<AlbumTrack> Album::addTrack( std::shared_ptr<Media> media, unsigned int trackNb, unsigned int discNumber )
{
    auto t = m_ml->getConn()->newTransaction();

    auto track = AlbumTrack::create( m_ml, m_id, media, trackNb, discNumber );
    if ( track == nullptr )
        return nullptr;
    media->setAlbumTrack( track );
    // Assume the media will be saved by the caller
    m_nbTracks++;
    m_duration += media->duration();
    t->commit();
    auto lock = m_tracks.lock();
    // Don't assume we have always have a valid value in m_tracks.
    // While it's ok to assume that if we are currently parsing the album, we
    // have a valid cache tracks, this isn't true when restarting an interrupted parsing.
    // The nbTracks value will be correct however. If it's equal to one, it means we're inserting
    // the first track in this album
    if ( m_tracks.isCached() == false && m_nbTracks == 1 )
        m_tracks.markCached();
    if ( m_tracks.isCached() == true )
        m_tracks.get().push_back( media );
    return track;
}

unsigned int Album::nbTracks() const
{
    return m_nbTracks;
}

unsigned int Album::duration() const
{
    return m_duration;
}

ArtistPtr Album::albumArtist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    auto lock = m_albumArtist.lock();
    if ( m_albumArtist.isCached() == false )
        m_albumArtist = Artist::fetch( m_ml, m_artistId );
    return m_albumArtist.get();
}

bool Album::setAlbumArtist( std::shared_ptr<Artist> artist )
{
    if ( m_artistId == artist->id() )
        return true;
    if ( artist->id() == 0 )
        return false;
    static const std::string req = "UPDATE " + policy::AlbumTable::Name + " SET "
            "artist_id = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artist->id(), m_id ) == false )
        return false;
    if ( m_artistId != 0 )
    {
        if ( m_albumArtist.isCached() == false )
            albumArtist();
        m_albumArtist.get()->updateNbAlbum( -1 );
    }
    m_artistId = artist->id();
    m_albumArtist = artist;
    artist->updateNbAlbum( 1 );
    static const std::string ftsReq = "UPDATE " + policy::AlbumTable::Name + "Fts SET "
            " artist = ? WHERE rowid = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), ftsReq, artist->name(), m_id );
    return true;
}

std::vector<ArtistPtr> Album::artists( bool desc ) const
{
    std::string req = "SELECT art.* FROM " + policy::ArtistTable::Name + " art "
            "INNER JOIN AlbumArtistRelation aar ON aar.artist_id = art.id_artist "
            "WHERE aar.album_id = ? ORDER BY art.name";
    if ( desc == true )
        req += " DESC";
    return Artist::fetchAll<IArtist>( m_ml, req, m_id );
}

bool Album::addArtist( std::shared_ptr<Artist> artist )
{
    static const std::string req = "INSERT OR IGNORE INTO AlbumArtistRelation VALUES(?, ?)";
    if ( m_id == 0 || artist->id() == 0 )
    {
        LOG_ERROR("Both artist & album need to be inserted in database before being linked together" );
        return false;
    }
    return sqlite::Tools::executeInsert( m_ml->getConn(), req, m_id, artist->id() ) != 0;
}

bool Album::removeArtist(Artist* artist)
{
    static const std::string req = "DELETE FROM AlbumArtistRelation WHERE album_id = ? "
            "AND id_artist = ?";
    return sqlite::Tools::executeDelete( m_ml->getConn(), req, m_id, artist->id() );
}

bool Album::createTable(DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT COLLATE NOCASE,"
                "artist_id UNSIGNED INTEGER,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "artwork_mrl TEXT,"
                "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
                "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
                "is_present BOOLEAN NOT NULL DEFAULT 1,"
                "FOREIGN KEY( artist_id ) REFERENCES " + policy::ArtistTable::Name
                + "(id_artist) ON DELETE CASCADE"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS AlbumArtistRelation("
                "album_id INTEGER,"
                "artist_id INTEGER,"
                "PRIMARY KEY (album_id, artist_id),"
                "FOREIGN KEY(album_id) REFERENCES " + policy::AlbumTable::Name + "("
                    + policy::AlbumTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
                "FOREIGN KEY(artist_id) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    static const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::AlbumTable::Name + "Fts USING FTS3("
                "title,"
                "artist"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel ) &&
            sqlite::Tools::executeRequest( dbConnection, vtableReq );
}

bool Album::createTriggers(DBConnection dbConnection)
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_album_present AFTER UPDATE OF "
            "is_present ON " + policy::AlbumTrackTable::Name +
            " BEGIN "
            " UPDATE " + policy::AlbumTable::Name + " SET is_present="
                "(SELECT COUNT(id_track) FROM " + policy::AlbumTrackTable::Name + " WHERE album_id=new.album_id AND is_present=1) "
                "WHERE id_album=new.album_id;"
            " END";
    static const std::string deleteTriggerReq = "CREATE TRIGGER IF NOT EXISTS delete_album_track AFTER DELETE ON "
             + policy::AlbumTrackTable::Name +
            " BEGIN "
            " UPDATE " + policy::AlbumTable::Name + " SET nb_tracks = nb_tracks - 1 WHERE id_album = old.album_id;"
            " DELETE FROM " + policy::AlbumTable::Name +
                " WHERE id_album=old.album_id AND nb_tracks = 0;"
            " END";
    static const std::string updateAddTrackTriggerReq = "CREATE TRIGGER IF NOT EXISTS add_album_track"
            " AFTER INSERT ON " + policy::AlbumTrackTable::Name +
            " BEGIN"
            " UPDATE " + policy::AlbumTable::Name +
            " SET duration = duration + (SELECT duration FROM " + policy::MediaTable::Name + " WHERE id_media=new.media_id),"
            " nb_tracks = nb_tracks + 1"
            " WHERE id_album = new.album_id;"
            " END";
    static const std::string vtriggerInsert = "CREATE TRIGGER IF NOT EXISTS insert_album_fts AFTER INSERT ON "
            + policy::AlbumTable::Name +
            // Skip unknown albums
            " WHEN new.title IS NOT NULL"
            " BEGIN"
            " INSERT INTO " + policy::AlbumTable::Name + "Fts(rowid, title) VALUES(new.id_album, new.title);"
            " END";
    static const std::string vtriggerDelete = "CREATE TRIGGER IF NOT EXISTS delete_album_fts BEFORE DELETE ON "
            + policy::AlbumTable::Name +
            // Unknown album probably won't be deleted, but better safe than sorry
            " WHEN old.title IS NOT NULL"
            " BEGIN"
            " DELETE FROM " + policy::AlbumTable::Name + "Fts WHERE rowid = old.id_album;"
            " END";
    return sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, deleteTriggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, updateAddTrackTriggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, vtriggerInsert ) &&
            sqlite::Tools::executeRequest( dbConnection, vtriggerDelete );
}

std::shared_ptr<Album> Album::create( MediaLibraryPtr ml, const std::string& title )
{
    auto album = std::make_shared<Album>( ml, title );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, title) VALUES(NULL, ?)";
    if ( insert( ml, album, req, title ) == false )
        return nullptr;
    return album;
}

std::shared_ptr<Album> Album::createUnknownAlbum( MediaLibraryPtr ml, const Artist* artist )
{
    auto album = std::make_shared<Album>( ml, artist );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, artist_id) VALUES(NULL, ?)";
    if ( insert( ml, album, req, artist->id() ) == false )
        return nullptr;
    return album;
}

std::vector<AlbumPtr> Album::search( MediaLibraryPtr ml, const std::string& pattern )
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name + " WHERE id_album IN "
            "(SELECT rowid FROM " + policy::AlbumTable::Name + "Fts WHERE " +
            policy::AlbumTable::Name + "Fts MATCH ?)"
            "AND is_present = 1";
    return fetchAll<IAlbum>( ml, req, pattern + "*" );
}

std::vector<AlbumPtr> Album::fromArtist( MediaLibraryPtr ml, int64_t artistId, SortingCriteria sort, bool desc )
{
    std::string req = "SELECT * FROM " + policy::AlbumTable::Name + " alb "
                    "WHERE artist_id = ? AND is_present=1 ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::Alpha:
        req += "title";
        if ( desc == true )
            req += " DESC";
        break;
    default:
        // When listing albums of an artist, default order is by descending year (with album title
        // discrimination in case 2+ albums went out the same year)
        // This leads to DESC being used for "non-desc" case
        if ( desc == true )
            req += "release_year, title";
        else
            req += "release_year DESC, title";
        break;
    }

    return fetchAll<IAlbum>( ml, req, artistId );
}

std::vector<AlbumPtr> Album::fromGenre( MediaLibraryPtr ml, int64_t genreId, SortingCriteria sort, bool desc)
{
    std::string req = "SELECT a.* FROM " + policy::AlbumTable::Name + " a "
            "INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.album_id = a.id_album "
            "WHERE att.genre_id = ? GROUP BY att.album_id";
    req += orderBy( sort, desc );
    return fetchAll<IAlbum>( ml, req, genreId );
}

std::vector<AlbumPtr> Album::listAll( MediaLibraryPtr ml, SortingCriteria sort, bool desc )
{
    std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
                    " WHERE is_present=1";
    req += orderBy( sort, desc );
    return fetchAll<IAlbum>( ml, req );
}

}
