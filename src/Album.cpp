/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
#include "Artist.h"
#include "Genre.h"
#include "Media.h"
#include "Thumbnail.h"
#include "utils/Enums.h"
#include "Deprecated.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string Album::Table::Name = "Album";
const std::string Album::Table::PrimaryKeyColumn = "id_album";
int64_t Album::* const Album::Table::PrimaryKey = &Album::m_id;
const std::string Album::FtsTable::Name = "AlbumFts";

Album::Album(MediaLibraryPtr ml, sqlite::Row& row)
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_title( row.extract<decltype(m_title)>() )
    , m_artistId( row.extract<decltype(m_artistId)>() )
    , m_releaseYear( row.extract<decltype(m_releaseYear)>() )
    , m_shortSummary( row.extract<decltype(m_shortSummary)>() )
    , m_nbTracks( row.extract<decltype(m_nbTracks)>() )
    , m_duration( row.extract<decltype(m_duration)>() )
    , m_nbDiscs( row.extract<decltype(m_nbDiscs)>() )
    , m_nbPresentTracks( row.extract<decltype(m_nbPresentTracks)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Album::Album( MediaLibraryPtr ml, std::string title )
    : m_ml( ml )
    , m_id( 0 )
    , m_title( std::move( title ) )
    , m_artistId( 0 )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_duration( 0 )
    , m_nbDiscs( 1 )
    , m_nbPresentTracks( 0 )
{
}

Album::Album( MediaLibraryPtr ml, const Artist* artist )
    : m_ml( ml )
    , m_id( 0 )
    , m_artistId( artist->id() )
    , m_releaseYear( ~0u )
    , m_nbTracks( 0 )
    , m_duration( 0 )
    , m_nbDiscs( 1 )
    , m_nbPresentTracks( 0 )
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

ThumbnailStatus Album::thumbnailStatus( ThumbnailSizeType sizeType ) const
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return ThumbnailStatus::Missing;
    return t->status();
}

const std::string& Album::thumbnailMrl( ThumbnailSizeType sizeType ) const
{
    const auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return Thumbnail::EmptyMrl;
    return t->mrl();
}

std::shared_ptr<Thumbnail> Album::thumbnail( ThumbnailSizeType sizeType ) const
{
    auto idx = Thumbnail::SizeToInt( sizeType );
    if ( m_thumbnails[idx] == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, Thumbnail::EntityType::Album,
                                           m_id, sizeType );
        if ( thumbnail == nullptr )
            return nullptr;
        m_thumbnails[idx] = std::move( thumbnail );
    }
    return m_thumbnails[idx];
}

bool Album::shouldUpdateThumbnail( const Thumbnail& currentThumbnail )
{
    /*
     * We want to update an album thumbnail.
     *
     * If we inherited the thumbnail from a media, we need to insert a new record
     * as we won't want to update the source media's thumbnail.
     *
     * If the cover comes from an album file, we can update it, and let the
     * thumbnails that were based on this album be updated as well
     *
     * In other cases, let's just insert a new thumbnail.
     */
    switch ( currentThumbnail.origin() )
    {
        case Thumbnail::Origin::Media:
            return false;
        case Thumbnail::Origin::CoverFile:
            return true;
        default:
            return false;
    }
}

bool Album::setThumbnail( std::shared_ptr<Thumbnail> newThumbnail )
{
    assert( newThumbnail != nullptr );
    auto currentThumbnail = thumbnail( newThumbnail->sizeType() );
    auto thumbnailIdx = Thumbnail::SizeToInt( newThumbnail->sizeType() );
    currentThumbnail = Thumbnail::updateOrReplace( m_ml, currentThumbnail,
                                                   std::move( newThumbnail ),
                                                   Album::shouldUpdateThumbnail,
                                                   m_id, Thumbnail::EntityType::Album );
    auto res = currentThumbnail != nullptr;
    m_thumbnails[thumbnailIdx] = std::move( currentThumbnail );
    return res;
}

std::string Album::addRequestJoin( const QueryParameters* params,
        bool media  )
{
    auto sort = params != nullptr ? params->sort : SortingCriteria::Alpha;
    bool artist = false;

    switch( sort )
    {
        case SortingCriteria::ReleaseDate:
        case SortingCriteria::Duration:
        case SortingCriteria::TrackNumber:
            /* No other tables required for this criterias */
            break;
        case SortingCriteria::PlayCount:
        case SortingCriteria::InsertionDate:
            media = true;
            break;
        case SortingCriteria::Artist:
            /* Different case than default, but same tables in the end */
            /* fall-through */
        case SortingCriteria::Alpha:
        case SortingCriteria::Default:
        default:
            // In case of identical album name
            // sort should continue with artist name
            artist = true;
            break;
    }
    std::string req;
    if ( artist == true )
        req += "LEFT JOIN " + Artist::Table::Name + " art ON alb.artist_id = art.id_artist ";
    if ( media == true )
        req += "INNER JOIN " + Media::Table::Name + " m ON m.album_id = alb.id_album ";
    return req;
}

std::string Album::orderTracksBy( const QueryParameters* params = nullptr )
{
    std::string req = " ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::TrackId;
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
    case SortingCriteria::TrackId:
    case SortingCriteria::Default:
        if ( desc == true )
            req += "med.disc_number DESC, med.track_number DESC, med.filename";
        else
            req += "med.disc_number, med.track_number, med.filename";
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
    case SortingCriteria::Artist:
        req += "art.name";
        if ( desc == true )
            req += " DESC";
        req += ", alb.title ";
        break;
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
    case SortingCriteria::PlayCount:
        /* This voluntarily overrides the initial "ORDER BY" in req, since
         * we need the GROUP BY first */
        req = "GROUP BY alb.id_album "
              "ORDER BY SUM(m.play_count) ";
        if ( desc == false )
            req += "DESC "; // Most played first by default
        req += ", alb.title";
        break;
    case SortingCriteria::InsertionDate:
        req = "GROUP BY alb.id_album "
              "ORDER BY MIN(m.insertion_date) ";
        if ( desc == true )
            req += "DESC ";
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::Alpha:
        req += "title";
        if ( desc == true )
            req += " DESC";
        req += ", art.name";
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
        " WHERE med.album_id = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND med.is_present != 0";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id );
}

Query<IMedia> Album::tracks( GenrePtr genre, const QueryParameters* params ) const
{
    if ( genre == nullptr )
        return {};
    std::string req = "FROM " + Media::Table::Name + " med "
            " WHERE med.album_id = ?"
            " AND med.genre_id = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND med.is_present != 0";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      orderTracksBy( params ), m_id, genre->id() );
}

std::vector<MediaPtr> Album::cachedTracks() const
{
    if ( m_tracks.empty() == true )
        m_tracks = tracks( nullptr )->all();
    return m_tracks;
}

Query<IMedia> Album::searchTracks( const std::string& pattern,
                                   const QueryParameters* params ) const
{
    return Media::searchAlbumTracks( m_ml, pattern, m_id, params );
}

bool Album::addTrack( std::shared_ptr<Media> media, unsigned int trackNb,
                      unsigned int discNumber, int64_t artistId, Genre* genre )
{
    /*
     * The transaction should already exist, but in order to avoid sprinkling
     * all the tests with a transaction block, we just create one here. It's
     * likely to be a noop transaction outside of the tests but it doesn't matter
     * much.
     */
    auto t = m_ml->getConn()->newTransaction();
    static const std::string req = "UPDATE " + Table::Name + " SET "
        "nb_tracks = nb_tracks + 1, is_present = is_present + 1,"
        "duration = duration + ? "
        "WHERE id_album = ?";
    auto duration = media->duration() >= 0 ? media->duration() : 0;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, duration,
                                       m_id ) == false )
        return false;
    if ( media->markAsAlbumTrack( m_id, trackNb, discNumber, artistId, genre ) == false )
        return false;
    if ( genre != nullptr && genre->updateNbTracks( 1 ) == false )
        return false;

    m_nbTracks++;
    assert( media->isPresent() );
    ++m_nbPresentTracks;
    m_duration += duration;
    // Don't assume we have always have a valid value in m_tracks.
    // While it's ok to assume that if we are currently parsing the album, we
    // have a valid cache tracks, this isn't true when restarting an interrupted parsing.
    // The nbTracks value will be correct however. If it's equal to one, it means we're inserting
    // the first track in this album
    if ( ( m_tracks.empty() == true && m_nbTracks == 1 ) ||
         ( m_tracks.empty() == false && m_nbTracks > 1 ) )
        m_tracks.push_back( std::move( media ) );
    t->commit();
    return true;
}

bool Album::removeTrack( Media& media )
{
    /*
     * Remove references to genre/album/artist from the media table before we
     * start updating album & genre.
     * If we don't do this first, if we're removing the last album track we'd
     * end up with a genre/album being deleted while there's still a foreign key
     * pointing to it
     */
    if ( media.setSubTypeUnknown() == false )
        return false;

    static const std::string req = "UPDATE " + Table::Name + " SET "
        "nb_tracks = nb_tracks - 1, is_present = is_present - 1,"
        "duration = duration - ? "
        "WHERE id_album = ?";
    auto duration = media.duration() >= 0 ? media.duration() : 0;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       duration, m_id ) == false )
        return false;

    auto genre = std::static_pointer_cast<Genre>( media.genre() );
    if ( genre != nullptr && genre->updateNbTracks( -1 ) )
        return false;

    m_duration -= duration;
    m_nbTracks--;
    auto it = std::find_if( begin( m_tracks ), end( m_tracks ), [&media]( MediaPtr& m ) {
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

uint32_t Album::nbPresentTracks() const
{
    return m_nbPresentTracks;
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

int64_t Album::duration() const
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
    m_artistId = artist->id();
    m_albumArtist = std::move( artist );
    static const std::string ftsReq = "UPDATE " + FtsTable::Name + " SET "
            " artist = ? WHERE rowid = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), ftsReq,
                                         m_albumArtist->name(), m_id );
}

Query<IArtist> Album::artists( const QueryParameters* params ) const
{
    std::string req = "FROM " + Artist::Table::Name + " art "
            "INNER JOIN " + Media::Table::Name + " m "
                "ON m.artist_id = art.id_artist "
            "WHERE m.album_id = ?";
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
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Album::createTriggers( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::IsPresent, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteTrack, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::InsertFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteEmpty, Settings::DbModelVersion ) );
}

void Album::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::ArtistId, Settings::DbModelVersion ) );
}

std::string Album::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == Table::Name )
    {
        if ( dbModel <= 16 )
        {
            return "CREATE TABLE " + Table::Name +
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
                "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name
                + "(id_artist) ON DELETE CASCADE,"
                "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
                + "(id_thumbnail)"
            ")";
        }
        return "CREATE TABLE " + Album::Table::Name +
        "("
            "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT COLLATE NOCASE,"
            "artist_id UNSIGNED INTEGER,"
            "release_year UNSIGNED INTEGER,"
            "short_summary TEXT,"
            // Number of tracks in this album, regardless of their presence
            // state.
            "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
            "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
            "nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,"
            // The album presence state, which is the number of present tracks
            // in this album
            "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
                "CHECK(is_present <= nb_tracks),"
            "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name
            + "(id_artist) ON DELETE CASCADE"
        ")";
    }
    else if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + Album::FtsTable::Name +
                " USING FTS3(title,artist)";
    }
    assert( !"Invalid table name provided" );
    return "<not a valid request>";
}

std::string Album::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::IsPresent:
        {
            if ( dbModel < 23 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                        " WHEN new.subtype = " +
                            utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                        " BEGIN "
                        " UPDATE " + Table::Name + " SET is_present=is_present + "
                            "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                            "WHERE id_album = (SELECT album_id FROM " + AlbumTrack::Table::Name + " "
                                "WHERE media_id = new.id_media"
                            ");"
                        " END";
            }
            else if ( dbModel < 34 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                       " WHEN new.subtype = " +
                           utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                       " AND old.is_present != new.is_present"
                       " BEGIN "
                       " UPDATE " + Table::Name + " SET is_present=is_present + "
                           "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                           "WHERE id_album = (SELECT album_id FROM " + AlbumTrack::Table::Name + " "
                               "WHERE media_id = new.id_media"
                           ");"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                   " WHEN new.subtype = " +
                       utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                   " AND old.is_present != new.is_present"
                   " BEGIN "
                   " UPDATE " + Table::Name + " SET is_present=is_present + "
                       "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                       "WHERE id_album = new.album_id;"
                   " END";
        }
        case Triggers::AddTrack:
        {
            assert( dbModel < 34 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER INSERT ON " + AlbumTrack::Table::Name +
                   " BEGIN"
                   " UPDATE " + Table::Name +
                        " SET duration = duration + new.duration,"
                        " nb_tracks = nb_tracks + 1,"
                        " is_present = is_present + 1"
                        " WHERE id_album = new.album_id;"
                   " END";
        }
        case Triggers::DeleteTrack:
        {
            if ( dbModel < 34 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                            " AFTER DELETE ON " + AlbumTrack::Table::Name +
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
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                        " AFTER DELETE ON " + Media::Table::Name +
                    " WHEN old.subtype = " +
                        utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                   " BEGIN "
                   " UPDATE " + Table::Name +
                        " SET"
                            " nb_tracks = nb_tracks - 1,"
                            " is_present = is_present - 1,"
                            " duration = duration - MAX(old.duration, 0)"
                            " WHERE id_album = old.album_id;"
                   " END";
        }
        case Triggers::InsertFts:
        {
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                    " AFTER INSERT ON " + Table::Name +
                    // Skip unknown albums
                    " WHEN new.title IS NOT NULL"
                    " BEGIN"
                        " INSERT INTO " + FtsTable::Name + "(rowid, title)"
                            " VALUES(new.id_album, new.title);"
                    " END";
        }
        case Triggers::DeleteFts:
        {
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                    " BEFORE DELETE ON " + Table::Name +
                    // Unknown album probably won't be deleted, but better safe than sorry
                    " WHEN old.title IS NOT NULL"
                    " BEGIN"
                        " DELETE FROM " + FtsTable::Name +
                            " WHERE rowid = old.id_album;"
                    " END";
        }
        case Triggers::DeleteEmpty:
        {
            assert( dbModel >= 34 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE OF nb_tracks ON " + Table::Name +
                   " WHEN new.nb_tracks = 0"
                   " BEGIN "
                        " DELETE FROM " + Table::Name +
                            " WHERE id_album=new.id_album;"
                   " END";
        }
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<Invalid request provided>";
}

std::string Album::triggerName( Album::Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::IsPresent:
        {
            if ( dbModel < 23 )
                return "is_album_present";
            return "album_is_present";
        }
        case Triggers::AddTrack:
            assert( dbModel < 34 );
            return "add_album_track";
        case Triggers::DeleteTrack:
            if ( dbModel < 34 )
                return "delete_album_track";
            return "album_delete_track";
        case Triggers::InsertFts:
            return "insert_album_fts";
        case Triggers::DeleteFts:
            return "delete_album_fts";
        case Triggers::DeleteEmpty:
            assert( dbModel >= 34 );
            return "album_delete_empty";
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Album::index( Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::ArtistId );
    return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
            Table::Name + "(artist_id)";
}

std::string Album::indexName( Album::Indexes index, uint32_t )
{
    UNUSED_IN_RELEASE( index );

    assert( index == Indexes::ArtistId );
    return "album_artist_id_idx";
}

bool Album::checkDbModel( MediaLibraryPtr ml )
{
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false )
        return false;

    if ( sqlite::Tools::checkIndexStatement( ml->getConn(),
            index( Indexes::ArtistId, Settings::DbModelVersion ),
            indexName( Indexes::ArtistId, Settings::DbModelVersion ) ) == false )
        return false;

    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    return check( ml->getConn(), Triggers::IsPresent ) &&
            check( ml->getConn(), Triggers::DeleteTrack ) &&
            check( ml->getConn(), Triggers::InsertFts ) &&
            check( ml->getConn(), Triggers::DeleteFts ) &&
            check( ml->getConn(), Triggers::DeleteEmpty );
}

std::shared_ptr<Album> Album::create( MediaLibraryPtr ml, std::string title )
{
    auto album = std::make_shared<Album>( ml, std::move( title ) );
    static const std::string req = "INSERT INTO " + Table::Name +
            "(id_album, title) VALUES(NULL, ?)";
    if ( insert( ml, album, req, album->m_title ) == false )
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
    std::string req = "FROM " + Table::Name + " alb ";
    req += addRequestJoin( params, false );
    req += "WHERE id_album IN "
            "(SELECT rowid FROM " + FtsTable::Name + " WHERE " +
            FtsTable::Name + " MATCH ?)";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND alb.is_present != 0";
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      orderBy( params ),
                                      sqlite::Tools::sanitizePattern( pattern ) );
}

Query<IAlbum> Album::searchFromArtist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb ";
    req += addRequestJoin( params, false );
    req += "WHERE id_album IN "
            "(SELECT rowid FROM " + FtsTable::Name + " WHERE " +
            FtsTable::Name + " MATCH ?)"
            " AND artist_id = ?";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND alb.is_present != 0";
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      orderBy( params ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      artistId );
}

Query<IAlbum> Album::fromArtist( MediaLibraryPtr ml, int64_t artistId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb "
                    "INNER JOIN " + Media::Table::Name + " m "
                        "ON m.album_id = alb.id_album "
                    "WHERE (m.artist_id = ? OR alb.artist_id = ?)";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND m.is_present != 0";
    std::string groupAndOrder = " GROUP BY m.album_id ORDER BY ";
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

    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      std::move( groupAndOrder ),
                                      artistId, artistId );
}

Query<IAlbum> Album::fromGenre( MediaLibraryPtr ml, int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb ";
    req += addRequestJoin( params, true );
    req += "WHERE m.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY m.album_id" + orderBy( params );
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      std::move( groupAndOrderBy ), genreId );
}

Query<IAlbum> Album::searchFromGenre( MediaLibraryPtr ml, const std::string& pattern,
                                      int64_t genreId, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " alb ";
    req += addRequestJoin( params, true );
    req += "WHERE id_album IN "
            "(SELECT rowid FROM " + FtsTable::Name + " WHERE " +
            FtsTable::Name + " MATCH ?)"
            "AND m.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY m.album_id" + orderBy( params );
    return make_query<Album, IAlbum>( ml, "alb.*", std::move( req ),
                                      std::move( groupAndOrderBy ),
                                      sqlite::Tools::sanitizePattern( pattern ),
                                      genreId );
}

Query<IAlbum> Album::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    std::string countReq = "SELECT COUNT(*) FROM " + Table::Name;
    std::string req = "SELECT alb.* FROM " + Table::Name + " alb ";
    req += addRequestJoin( params, false );
    if ( params == nullptr || params->includeMissing == false )
    {
        countReq += " WHERE is_present != 0";
        req += "WHERE alb.is_present != 0 ";
    }
    req += orderBy( params );
    return make_query_with_count<Album, IAlbum>( ml, std::move( countReq ),
                                                 std::move( req ) );
}

bool Album::checkDBConsistency( MediaLibraryPtr ml )
{
    sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT nb_tracks, is_present FROM " +
                    Album::Table::Name
    };
    stmt.execute();
    sqlite::Row row;
    while ( ( row = stmt.row() ) != nullptr )
    {
        auto nbTracks = row.extract<uint32_t>();
        auto isPresent = row.extract<uint32_t>();
        if ( nbTracks != isPresent )
            return false;
    }
    return true;
}

}
