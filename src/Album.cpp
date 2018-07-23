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
#include "Genre.h"
#include "Media.h"
#include "Thumbnail.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

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
        >> m_thumbnailId
        >> m_nbTracks
        >> m_duration
        >> m_isPresent;
}

Album::Album( MediaLibraryPtr ml, const std::string& title, int64_t thumbnailId )
    : m_ml( ml )
    , m_id( 0 )
    , m_title( title )
    , m_artistId( 0 )
    , m_releaseYear( ~0u )
    , m_thumbnailId( thumbnailId )
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
    , m_thumbnailId( 0 )
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
    if ( m_thumbnailId == 0 )
        return Thumbnail::EmptyMrl;

    auto lock = m_thumbnail.lock();
    if ( m_thumbnail.isCached() == false )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
        if ( thumbnail == nullptr )
            return Thumbnail::EmptyMrl;
        m_thumbnail = std::move( thumbnail );
    }
    return m_thumbnail.get()->mrl();
}

std::shared_ptr<Thumbnail> Album::thumbnail()
{
    if ( m_thumbnailId == 0 )
        return nullptr;
    auto lock = m_thumbnail.lock();
    if ( m_thumbnail.isCached() == false )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
        if ( thumbnail == nullptr )
            return nullptr;
        m_thumbnail = std::move( thumbnail );
    }
    return m_thumbnail.get();
}

bool Album::setArtworkMrl( const std::string& artworkMrl, Thumbnail::Origin origin )
{
    if ( m_thumbnailId != 0 )
        return Thumbnail::setMrlFromPrimaryKey( m_ml, m_thumbnail, m_thumbnailId,
                                                artworkMrl, origin );

    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = m_ml->getConn()->newTransaction();
    auto lock = m_thumbnail.lock();
    m_thumbnail = Thumbnail::create( m_ml, artworkMrl, Thumbnail::Origin::Album );
    if ( m_thumbnail.get() == nullptr )
        return false;
    static const std::string req = "UPDATE " + policy::AlbumTable::Name
            + " SET thumbnail_id = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_thumbnail.get()->id(), m_id ) == false )
        return false;
    m_thumbnailId = m_thumbnail.get()->id();
    if ( t != nullptr )
        t->commit();
    return true;
}

std::string Album::orderTracksBy( const QueryParameters* params = nullptr )
{
    std::string req = " ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
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

std::string Album::orderBy( const QueryParameters* params )
{
    std::string req = " ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
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
    case SortingCriteria::TrackNumber:
        req += "nb_tracks";
        if ( desc == false )
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

Query<IMedia> Album::tracks( const QueryParameters* params ) const
{
    // This doesn't return the cached version, because it would be fairly complicated, if not impossible or
    // counter productive, to maintain a cache that respects all orderings.
    std::string req = "FROM " + policy::MediaTable::Name + " med "
        " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
        " WHERE att.album_id = ? AND med.is_present != 0";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id );
}

Query<IMedia> Album::tracks( GenrePtr genre, const QueryParameters* params ) const
{
    if ( genre == nullptr )
        return {};
    std::string req = "FROM " + policy::MediaTable::Name + " med "
            " INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? AND med.is_present != 0"
            " AND genre_id = ?";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id, genre->id() );
}

std::vector<MediaPtr> Album::cachedTracks() const
{
    auto lock = m_tracks.lock();
    if ( m_tracks.isCached() == false )
        m_tracks = tracks( nullptr )->all();
    return m_tracks.get();
}

Query<IMedia> Album::searchTracks( const std::string& pattern,
                                   const QueryParameters* params ) const
{
    return Media::searchAlbumTracks( m_ml, pattern, m_id, params );
}

std::shared_ptr<AlbumTrack> Album::addTrack( std::shared_ptr<Media> media, unsigned int trackNb,
                                             unsigned int discNumber, int64_t artistId, Genre* genre )
{
    auto track = AlbumTrack::create( m_ml, m_id, media, trackNb, discNumber, artistId,
                                     genre != nullptr ? genre->id() : 0, media->duration() );
    if ( track == nullptr )
        return nullptr;
    media->setAlbumTrack( track );
    if ( genre != nullptr )
        genre->updateCachedNbTracks( 1 );
    // Assume the media will be saved by the caller
    m_nbTracks++;
    if ( media->duration() > 0 )
        m_duration += media->duration();
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

Query<IArtist> Album::artists( const QueryParameters* params ) const
{
    std::string req = "FROM " + policy::ArtistTable::Name + " art "
            "INNER JOIN AlbumArtistRelation aar ON aar.artist_id = art.id_artist "
            "WHERE aar.album_id = ?";
    std::string orderBy = "ORDER BY art.name";
    if ( params != nullptr && params->desc == true )
        orderBy += " DESC";
    return make_query<Artist, IArtist>( m_ml, "art.*", std::move( req ),
                                        std::move( orderBy ), m_id );
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

void Album::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::AlbumTable::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT COLLATE NOCASE,"
                "artist_id UNSIGNED INTEGER,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "thumbnail_id UNSIGNED INT,"
                "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
                "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
                "is_present BOOLEAN NOT NULL DEFAULT 1,"
                "FOREIGN KEY( artist_id ) REFERENCES " + policy::ArtistTable::Name
                + "(id_artist) ON DELETE CASCADE,"
                "FOREIGN KEY(thumbnail_id) REFERENCES " + policy::ThumbnailTable::Name
                + "(id_thumbnail)"
            ")";
    const std::string reqRel = "CREATE TABLE IF NOT EXISTS AlbumArtistRelation("
                "album_id INTEGER,"
                "artist_id INTEGER,"
                "PRIMARY KEY (album_id, artist_id),"
                "FOREIGN KEY(album_id) REFERENCES " + policy::AlbumTable::Name + "("
                    + policy::AlbumTable::PrimaryKeyColumn + ") ON DELETE CASCADE,"
                "FOREIGN KEY(artist_id) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + policy::AlbumTable::Name + "Fts USING FTS3("
                "title,"
                "artist"
            ")";

    sqlite::Tools::executeRequest( dbConnection, req );
    sqlite::Tools::executeRequest( dbConnection, reqRel );
    sqlite::Tools::executeRequest( dbConnection, vtableReq );
}

void Album::createTriggers( sqlite::Connection* dbConnection )
{
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS album_artist_id_idx ON " +
            policy::AlbumTable::Name + "(artist_id)";
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_album_present AFTER UPDATE OF "
            "is_present ON " + policy::AlbumTrackTable::Name +
            " BEGIN "
            " UPDATE " + policy::AlbumTable::Name + " SET is_present="
                "(SELECT EXISTS("
                    "SELECT id_track FROM " + policy::AlbumTrackTable::Name +
                    " WHERE album_id=new.album_id AND is_present != 0 LIMIT 1"
                ") )"
                "WHERE id_album=new.album_id;"
            " END";
    static const std::string deleteTriggerReq = "CREATE TRIGGER IF NOT EXISTS delete_album_track AFTER DELETE ON "
             + policy::AlbumTrackTable::Name +
            " BEGIN "
            " UPDATE " + policy::AlbumTable::Name +
            " SET"
                " nb_tracks = nb_tracks - 1,"
                " duration = duration - old.duration"
                " WHERE id_album = old.album_id;"
            " DELETE FROM " + policy::AlbumTable::Name +
                " WHERE id_album=old.album_id AND nb_tracks = 0;"
            " END";
    static const std::string updateAddTrackTriggerReq = "CREATE TRIGGER IF NOT EXISTS add_album_track"
            " AFTER INSERT ON " + policy::AlbumTrackTable::Name +
            " BEGIN"
            " UPDATE " + policy::AlbumTable::Name +
            " SET duration = duration + new.duration,"
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
    sqlite::Tools::executeRequest( dbConnection, indexReq );
    sqlite::Tools::executeRequest( dbConnection, triggerReq );
    sqlite::Tools::executeRequest( dbConnection, deleteTriggerReq );
    sqlite::Tools::executeRequest( dbConnection, updateAddTrackTriggerReq );
    sqlite::Tools::executeRequest( dbConnection, vtriggerInsert );
    sqlite::Tools::executeRequest( dbConnection, vtriggerDelete );
}

std::shared_ptr<Album> Album::create( MediaLibraryPtr ml, const std::string& title, int64_t thumbnailId )
{
    auto album = std::make_shared<Album>( ml, title, thumbnailId );
    static const std::string req = "INSERT INTO " + policy::AlbumTable::Name +
            "(id_album, title, thumbnail_id) VALUES(NULL, ?, ?)";
    if ( insert( ml, album, req, title, sqlite::ForeignKey( thumbnailId ) ) == false )
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

Query<IAlbum> Album::search( MediaLibraryPtr ml, const std::string& pattern,
                             const QueryParameters* params )
{
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + policy::AlbumTable::Name + "Fts WHERE " +
            policy::AlbumTable::Name + "Fts MATCH '*' || ? || '*')"
            "AND is_present != 0";
    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      orderBy( params ), pattern );
}

Query<IAlbum> Album::searchFromArtist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + policy::AlbumTable::Name + "Fts WHERE " +
            policy::AlbumTable::Name + "Fts MATCH '*' || ? || '*')"
            "AND is_present != 0 "
            "AND artist_id = ?";
    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      orderBy( params ), pattern, artistId );
}

Query<IAlbum> Album::fromArtist( MediaLibraryPtr ml, int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
                    "INNER JOIN " + policy::AlbumTrackTable::Name + " att "
                        "ON att.album_id = alb.id_album "
                    "WHERE (att.artist_id = ? OR alb.artist_id = ?) "
                        "AND att.is_present != 0 ";
    std::string groupAndOrder = "GROUP BY att.album_id ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
    case SortingCriteria::Alpha:
        groupAndOrder += "title";
        if ( desc == true )
            groupAndOrder += " DESC";
        break;
    default:
        // When listing albums of an artist, default order is by descending year (with album title
        // discrimination in case 2+ albums went out the same year)
        // This leads to DESC being used for "non-desc" case
        if ( desc == true )
            groupAndOrder += "release_year, title";
        else
            groupAndOrder += "release_year DESC, title";
        break;
    }

    return make_query<Album, IAlbum>( ml, "*", req, std::move( groupAndOrder ),
                                      artistId, artistId );
}

Query<IAlbum> Album::fromGenre( MediaLibraryPtr ml, int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
            "INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.album_id = alb.id_album "
            "WHERE att.genre_id = ? GROUP BY att.album_id";
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      orderBy( params ), genreId );
}

Query<IAlbum> Album::searchFromGenre( MediaLibraryPtr ml, const std::string& pattern,
                                      int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
            "INNER JOIN " + policy::AlbumTrackTable::Name + " att ON att.album_id = alb.id_album "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + policy::AlbumTable::Name + "Fts WHERE " +
            policy::AlbumTable::Name + "Fts MATCH '*' || ? || '*')"
            "AND att.genre_id = ? GROUP BY att.album_id";
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      orderBy( params ), pattern, genreId );
}

Query<IAlbum> Album::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    if ( sort == SortingCriteria::Artist )
    {
        std::string req = "FROM " + policy::AlbumTable::Name + " alb "
                "INNER JOIN " + policy::ArtistTable::Name + " art ON alb.artist_id = art.id_artist "
                "WHERE alb.is_present != 0 ";
        std::string orderBy = "ORDER BY art.name ";
        if ( desc == true )
            orderBy += "DESC ";
        orderBy += ", alb.title";
        return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                          std::move( orderBy ) );
    }
    if ( sort == SortingCriteria::PlayCount )
    {
        std::string req = "FROM " + policy::AlbumTable::Name + " alb "
                 "INNER JOIN " + policy::AlbumTrackTable::Name + " t ON alb.id_album = t.album_id "
                 "INNER JOIN " + policy::MediaTable::Name + " m ON t.media_id = m.id_media "
                 "WHERE alb.is_present != 0 ";
        std::string groupBy = "GROUP BY id_album "
                 "ORDER BY SUM(m.play_count) ";
        if ( desc == false )
            groupBy += "DESC "; // Most played first by default
        groupBy += ", alb.title";
        return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                          std::move( groupBy ) );
    }
    std::string req = "FROM " + policy::AlbumTable::Name + " alb "
                    " WHERE is_present != 0";
    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      orderBy( params ) );
}

}
