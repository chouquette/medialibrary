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

const std::string Album::Table::Name = "Album";
const std::string Album::Table::PrimaryKeyColumn = "id_album";
int64_t Album::* const Album::Table::PrimaryKey = &Album::m_id;

Album::Album(MediaLibraryPtr ml, sqlite::Row& row)
    : m_ml( ml )
    , m_id( row.load<decltype(m_id)>( 0 ) )
    , m_title( row.load<decltype(m_title)>( 1 ) )
    , m_artistId( row.load<decltype(m_artistId)>( 2 ) )
    , m_releaseYear( row.load<decltype(m_releaseYear)>( 3 ) )
    , m_shortSummary( row.load<decltype(m_shortSummary)>( 4 ) )
    , m_thumbnailId( row.load<decltype(m_thumbnailId)>( 5 ) )
    , m_nbTracks( row.load<decltype(m_nbTracks)>( 6 ) )
    , m_duration( row.load<decltype(m_duration)>( 7 ) )
    , m_nbDiscs( row.load<decltype(m_nbDiscs)>( 8 ) )
    , m_isPresent( row.load<decltype(m_isPresent)>( 9 ) )
{
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
    , m_nbDiscs( 1 )
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
    , m_nbDiscs( 1 )
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
    static const std::string req = "UPDATE " + Album::Table::Name
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
    static const std::string req = "UPDATE " + Album::Table::Name
            + " SET short_summary = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, summary, m_id ) == false )
        return false;
    m_shortSummary = summary;
    return true;
}

bool Album::isThumbnailGenerated() const
{
    return m_thumbnailId != 0;
}

const std::string& Album::thumbnailMrl() const
{
    if ( m_thumbnailId == 0 )
        return Thumbnail::EmptyMrl;

    if ( m_thumbnail == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
        if ( thumbnail == nullptr )
            return Thumbnail::EmptyMrl;
        m_thumbnail = std::move( thumbnail );
    }
    return m_thumbnail->mrl();
}

std::shared_ptr<Thumbnail> Album::thumbnail()
{
    if ( m_thumbnailId == 0 )
        return nullptr;
    if ( m_thumbnail == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
        if ( thumbnail == nullptr )
            return nullptr;
        m_thumbnail = std::move( thumbnail );
    }
    return m_thumbnail;
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
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default" );
        /* fall-through */
    case SortingCriteria::Default:
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
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::Alpha:
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
    std::string req = "FROM " + Media::Table::Name + " med "
        " INNER JOIN " + AlbumTrack::Table::Name + " att ON att.media_id = med.id_media "
        " WHERE att.album_id = ? AND med.is_present != 0";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id );
}

Query<IMedia> Album::tracks( GenrePtr genre, const QueryParameters* params ) const
{
    if ( genre == nullptr )
        return {};
    std::string req = "FROM " + Media::Table::Name + " med "
            " INNER JOIN " + AlbumTrack::Table::Name + " att ON att.media_id = med.id_media "
            " WHERE att.album_id = ? AND med.is_present != 0"
            " AND genre_id = ?";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id, genre->id() );
}

std::vector<MediaPtr> Album::cachedTracks() const
{
    if ( m_tracks.size() == 0 )
        m_tracks = tracks( nullptr )->all();
    return m_tracks;
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
    // Don't assume we have always have a valid value in m_tracks.
    // While it's ok to assume that if we are currently parsing the album, we
    // have a valid cache tracks, this isn't true when restarting an interrupted parsing.
    // The nbTracks value will be correct however. If it's equal to one, it means we're inserting
    // the first track in this album
    if ( ( m_tracks.empty() == true && m_nbTracks == 1 ) ||
         ( m_tracks.empty() == false && m_nbTracks > 1 ) )
        m_tracks.push_back( media );
    return track;
}

bool Album::removeTrack( Media& media, AlbumTrack& track )
{
    m_duration -= media.duration();
    m_nbTracks--;
    auto genre = std::static_pointer_cast<Genre>( track.genre() );
    if ( genre != nullptr )
        genre->updateCachedNbTracks( -1 );
    auto it = std::find_if( begin( m_tracks ), end( m_tracks ), [&media]( MediaPtr m ) {
        return m->id() == media.id();
    });
    if ( it != end( m_tracks ) )
        m_tracks.erase( it );

    return true;
}

unsigned int Album::nbTracks() const
{
    return m_nbTracks;
}

uint32_t Album::nbDiscs() const
{
    return m_nbDiscs;
}

bool Album::setNbDiscs( uint32_t nbDiscs )
{
    static const std::string req = "UPDATE " + Album::Table::Name
            + " SET nb_discs = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, nbDiscs, m_id ) == false )
        return false;
    m_nbDiscs = nbDiscs;
    return true;
}

unsigned int Album::duration() const
{
    return m_duration;
}

bool Album::isUnknownAlbum() const
{
    return m_title.empty();
}

ArtistPtr Album::albumArtist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    if ( m_albumArtist == nullptr )
        m_albumArtist = Artist::fetch( m_ml, m_artistId );
    return m_albumArtist;
}

bool Album::setAlbumArtist( std::shared_ptr<Artist> artist )
{
    if ( m_artistId == artist->id() )
        return true;
    if ( artist->id() == 0 )
        return false;
    static const std::string req = "UPDATE " + Table::Name + " SET "
            "artist_id = ? WHERE id_album = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artist->id(), m_id ) == false )
        return false;
    if ( m_artistId != 0 )
    {
        if ( m_albumArtist == nullptr )
            albumArtist();
        m_albumArtist->updateNbAlbum( -1 );
    }
    m_artistId = artist->id();
    m_albumArtist = artist;
    artist->updateNbAlbum( 1 );
    static const std::string ftsReq = "UPDATE " + Table::Name + "Fts SET "
            " artist = ? WHERE rowid = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), ftsReq, artist->name(), m_id );
    return true;
}

Query<IArtist> Album::artists( const QueryParameters* params ) const
{
    std::string req = "FROM " + Artist::Table::Name + " art "
            "INNER JOIN " + AlbumTrack::Table::Name + " att "
                "ON att.artist_id = art.id_artist "
            "WHERE att.album_id = ?";
    if ( params != nullptr && ( params->sort != SortingCriteria::Alpha &&
                                params->sort != SortingCriteria::Default ) )
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
    std::string orderBy = "GROUP BY art.id_artist ORDER BY art.name";
    if ( params != nullptr && params->desc == true )
        orderBy += " DESC";
    return make_query<Artist, IArtist>( m_ml, "art.*", std::move( req ),
                                        std::move( orderBy ), m_id );
}

void Album::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " +
            Table::Name +
            "("
                "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
                "title TEXT COLLATE NOCASE,"
                "artist_id UNSIGNED INTEGER,"
                "release_year UNSIGNED INTEGER,"
                "short_summary TEXT,"
                "thumbnail_id UNSIGNED INT,"
                // Number of tracks in this album, regardless of their presence
                // state.
                "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
                "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
                "nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,"
                // The album presence state, which is the number of present tracks
                // in this album
                "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
                    "CHECK(is_present <= nb_tracks),"
                "FOREIGN KEY( artist_id ) REFERENCES " + Artist::Table::Name
                + "(id_artist) ON DELETE CASCADE,"
                "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
                + "(id_thumbnail)"
            ")";
    const std::string vtableReq = "CREATE VIRTUAL TABLE IF NOT EXISTS "
                + Table::Name + "Fts USING FTS3("
                "title,"
                "artist"
            ")";

    sqlite::Tools::executeRequest( dbConnection, req );
    sqlite::Tools::executeRequest( dbConnection, vtableReq );
}

void Album::createTriggers( sqlite::Connection* dbConnection )
{
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS album_artist_id_idx ON " +
            Table::Name + "(artist_id)";
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_album_present AFTER UPDATE OF "
            "is_present ON " + Media::Table::Name +
            " WHEN new.subtype = " +
                std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                                    IMedia::SubType::AlbumTrack ) ) +
            " BEGIN "
            " UPDATE " + Table::Name + " SET is_present=is_present + "
                "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                "WHERE id_album = (SELECT album_id FROM " + AlbumTrack::Table::Name + " "
                    "WHERE media_id = new.id_media"
                ");"
            " END";
    static const std::string deleteTriggerReq = "CREATE TRIGGER IF NOT EXISTS delete_album_track AFTER DELETE ON "
             + AlbumTrack::Table::Name +
            " BEGIN "
            " UPDATE " + Table::Name +
            " SET"
                " nb_tracks = nb_tracks - 1,"
                " is_present = is_present - 1,"
                " duration = duration - old.duration"
                " WHERE id_album = old.album_id;"
            " DELETE FROM " + Table::Name +
                " WHERE id_album=old.album_id AND nb_tracks = 0;"
            " END";
    static const std::string updateAddTrackTriggerReq = "CREATE TRIGGER IF NOT EXISTS add_album_track"
            " AFTER INSERT ON " + AlbumTrack::Table::Name +
            " BEGIN"
            " UPDATE " + Table::Name +
            " SET duration = duration + new.duration,"
            " nb_tracks = nb_tracks + 1,"
            " is_present = is_present + 1"
            " WHERE id_album = new.album_id;"
            " END";
    static const std::string vtriggerInsert = "CREATE TRIGGER IF NOT EXISTS insert_album_fts AFTER INSERT ON "
            + Table::Name +
            // Skip unknown albums
            " WHEN new.title IS NOT NULL"
            " BEGIN"
            " INSERT INTO " + Table::Name + "Fts(rowid, title) VALUES(new.id_album, new.title);"
            " END";
    static const std::string vtriggerDelete = "CREATE TRIGGER IF NOT EXISTS delete_album_fts BEFORE DELETE ON "
            + Table::Name +
            // Unknown album probably won't be deleted, but better safe than sorry
            " WHEN old.title IS NOT NULL"
            " BEGIN"
            " DELETE FROM " + Table::Name + "Fts WHERE rowid = old.id_album;"
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
    static const std::string req = "INSERT INTO " + Table::Name +
            "(id_album, title, thumbnail_id) VALUES(NULL, ?, ?)";
    if ( insert( ml, album, req, title, sqlite::ForeignKey( thumbnailId ) ) == false )
        return nullptr;
    return album;
}

std::shared_ptr<Album> Album::createUnknownAlbum( MediaLibraryPtr ml, const Artist* artist )
{
    auto album = std::make_shared<Album>( ml, artist );
    static const std::string req = "INSERT INTO " + Table::Name +
            "(id_album, artist_id) VALUES(NULL, ?)";
    if ( insert( ml, album, req, artist->id() ) == false )
        return nullptr;
    return album;
}

Query<IAlbum> Album::search( MediaLibraryPtr ml, const std::string& pattern,
                             const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + Table::Name + "Fts WHERE " +
            Table::Name + "Fts MATCH '*' || ? || '*')"
            "AND is_present != 0";
    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      orderBy( params ), pattern );
}

Query<IAlbum> Album::searchFromArtist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + Table::Name + "Fts WHERE " +
            Table::Name + "Fts MATCH '*' || ? || '*')"
            "AND is_present != 0 "
            "AND artist_id = ?";
    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      orderBy( params ), pattern, artistId );
}

Query<IAlbum> Album::fromArtist( MediaLibraryPtr ml, int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
                    "INNER JOIN " + AlbumTrack::Table::Name + " att "
                        "ON att.album_id = alb.id_album "
                    "INNER JOIN " + Media::Table::Name + " m "
                        "ON att.media_id = m.id_media "
                    "WHERE (att.artist_id = ? OR alb.artist_id = ?) "
                        "AND m.is_present != 0 ";
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
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (ReleaseDate)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::ReleaseDate:
        // When listing albums of an artist, default order is by descending year (with album title
        // discrimination in case 2+ albums went out the same year)
        // This leads to DESC being used for "non-desc" case
        if ( desc == true )
            groupAndOrder += "release_year, title";
        else
            groupAndOrder += "release_year DESC, title";
        break;
    }

    return make_query<Album, IAlbum>( ml, "*", std::move( req ),
                                      std::move( groupAndOrder ),
                                      artistId, artistId );
}

Query<IAlbum> Album::fromGenre( MediaLibraryPtr ml, int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
            "INNER JOIN " + AlbumTrack::Table::Name + " att ON att.album_id = alb.id_album "
            "WHERE att.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY att.album_id" + orderBy( params );
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      std::move( groupAndOrderBy ), genreId );
}

Query<IAlbum> Album::searchFromGenre( MediaLibraryPtr ml, const std::string& pattern,
                                      int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
            "INNER JOIN " + AlbumTrack::Table::Name + " att ON att.album_id = alb.id_album "
            "WHERE id_album IN "
            "(SELECT rowid FROM " + Table::Name + "Fts WHERE " +
            Table::Name + "Fts MATCH '*' || ? || '*')"
            "AND att.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY att.album_id" + orderBy( params );
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      std::move( groupAndOrderBy ), pattern,
                                      genreId );
}

Query<IAlbum> Album::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    std::string countReq = "SELECT COUNT(*) FROM " + Table::Name + " WHERE "
            "is_present != 0";
    std::string req = "SELECT alb.* FROM " + Table::Name + " alb ";
    if ( sort == SortingCriteria::Artist )
    {
        req += "INNER JOIN " + Artist::Table::Name + " art ON alb.artist_id = art.id_artist "
               "WHERE alb.is_present != 0 ";
        req += "ORDER BY art.name ";
        if ( desc == true )
            req += "DESC ";
        req += ", alb.title";
    }
    else if ( sort == SortingCriteria::PlayCount )
    {
        req += "INNER JOIN " + AlbumTrack::Table::Name + " t ON alb.id_album = t.album_id "
               "INNER JOIN " + Media::Table::Name + " m ON t.media_id = m.id_media "
               "WHERE alb.is_present != 0 ";
        req += "GROUP BY id_album "
               "ORDER BY SUM(m.play_count) ";
        if ( desc == false )
            req += "DESC "; // Most played first by default
        req += ", alb.title";
    }
    else
    {
        req += " WHERE is_present != 0";
        req += orderBy( params );
    }
    return make_query_with_count<Album, IAlbum>( ml, std::move( countReq ),
                                                 std::move( req ) );
}

}
