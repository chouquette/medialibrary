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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"

namespace medialibrary
{

const std::string Artist::Table::Name = "Artist";
const std::string Artist::Table::PrimaryKeyColumn = "id_artist";
int64_t Artist::*const Artist::Table::PrimaryKey = &Artist::m_id;
const std::string Artist::FtsTable::Name = "ArtistFts";
const std::string Artist::MediaRelationTable::Name = "MediaArtistRelation";

Artist::Artist( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_shortBio( row.extract<decltype(m_shortBio)>() )
    , m_nbAlbums( row.extract<decltype(m_nbAlbums)>() )
    , m_nbTracks( row.extract<decltype(m_nbTracks)>() )
    , m_mbId( row.extract<decltype(m_mbId)>() )
    , m_isPresent( row.extract<decltype(m_isPresent)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Artist::Artist( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_nbAlbums( 0 )
    , m_nbTracks( 0 )
    , m_isPresent( true )
{
}

int64_t Artist::id() const
{
    return m_id;
}

const std::string& Artist::name() const
{
    return m_name;
}

const std::string& Artist::shortBio() const
{
    return m_shortBio;
}

bool Artist::setShortBio(const std::string& shortBio)
{
    static const std::string req = "UPDATE " + Artist::Table::Name
            + " SET shortbio = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, shortBio, m_id ) == false )
        return false;
    m_shortBio = shortBio;
    return true;
}

Query<IAlbum> Artist::albums( const QueryParameters* params ) const
{
    return Album::fromArtist( m_ml, m_id, params );
}

Query<IAlbum> Artist::searchAlbums( const std::string& pattern,
                                    const QueryParameters* params ) const
{
    return Album::searchFromArtist( m_ml, pattern, m_id, params );
}

Query<IMedia> Artist::tracks( const QueryParameters* params ) const
{
    SortingCriteria sort = params != nullptr ? params->sort : SortingCriteria::Default;
    bool desc = params != nullptr ? params->desc : false;
    std::string req = "FROM " + Media::Table::Name + " med "
        "INNER JOIN " + MediaRelationTable::Name + " mar "
            "ON mar.media_id = med.id_media ";
    if ( sort != SortingCriteria::Duration &&
         sort != SortingCriteria::InsertionDate &&
         sort != SortingCriteria::ReleaseDate &&
         sort != SortingCriteria::Alpha )
    {
        req += "INNER JOIN AlbumTrack atr ON atr.media_id = med.id_media "
               "INNER JOIN Album alb ON alb.id_album = atr.album_id ";
    }
    req += "WHERE mar.artist_id = ? ";

    req += "AND med.is_present != 0";
    std::string orderBy = "ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::Duration:
        orderBy += "med.duration";
        break;
    case SortingCriteria::InsertionDate:
        orderBy += "med.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        orderBy += "med.release_date";
        break;
    case SortingCriteria::Alpha:
        orderBy += "med.title";
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Album)" );
        /* fall-through */
    case SortingCriteria::Album:
    case SortingCriteria::Default:
        if ( desc == true )
            orderBy += "alb.title DESC, atr.disc_number, atr.track_number";
        else
            orderBy += "alb.title, atr.disc_number, atr.track_number";
        break;
    }

    if ( desc == true && sort != SortingCriteria::Album )
        orderBy += " DESC";
    return make_query<Media, IMedia>( m_ml, "med.*", std::move( req ),
                                      std::move( orderBy ), m_id );
}

Query<IMedia> Artist::searchTracks( const std::string& pattern, const QueryParameters* params ) const
{
    return Media::searchArtistTracks( m_ml, pattern, m_id, params );
}

bool Artist::addMedia( Media& media )
{
    static const std::string req = "INSERT INTO " + MediaRelationTable::Name +
            " VALUES(?, ?)";
    return sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(), m_id ) != 0;
}

ThumbnailStatus Artist::thumbnailStatus( ThumbnailSizeType sizeType ) const
{
    auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return ThumbnailStatus::Missing;
    return t->status();
}

const std::string& Artist::thumbnailMrl( ThumbnailSizeType sizeType ) const
{
    const auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return Thumbnail::EmptyMrl;
    return t->mrl();
}

std::shared_ptr<Thumbnail> Artist::thumbnail( ThumbnailSizeType sizeType ) const
{
    auto idx = Thumbnail::SizeToInt( sizeType );
    if ( m_thumbnails[idx] == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, Thumbnail::EntityType::Artist,
                                           m_id, sizeType );
        if ( thumbnail == nullptr )
            return nullptr;
        m_thumbnails[idx] = std::move( thumbnail );
    }
    return m_thumbnails[idx];
}

bool Artist::shouldUpdateThumbnail( Thumbnail& currentThumbnail,
                                    Thumbnail::Origin newOrigin )
{
    switch( currentThumbnail.origin() )
    {
        case Thumbnail::Origin::Artist:
        {
            // The previous thumbnail was based on an artist.
            // AlbumArtist has a higher priority over this, otherwise reject
            // anything that is not used provided.
            return newOrigin != Thumbnail::Origin::AlbumArtist &&
                   newOrigin != Thumbnail::Origin::UserProvided;
        }
        case Thumbnail::Origin::AlbumArtist:
        {
            // This is the highest ranking for an artist thumbnail, so
            // we only accept to override it with a user provided thumbnail
            return newOrigin == Thumbnail::Origin::UserProvided;
        }
        case Thumbnail::Origin::Media:
        {
            // The thumbnail was determined by an embeded artwork, we can
            // accept pretty much anything else
            return newOrigin != Thumbnail::Origin::Media;
        }
        case Thumbnail::Origin::CoverFile:
        {
            // The previous thumbnail came from an album cover file.
            // Accept only thumbnails from artists & user provided
            return newOrigin == Thumbnail::Origin::Artist ||
                   newOrigin == Thumbnail::Origin::AlbumArtist ||
                   newOrigin == Thumbnail::Origin::UserProvided;
        }
        case Thumbnail::Origin::UserProvided:
            return true;
        default:
            assert( !"Unreachable" );
            return false;
    }
}

bool Artist::setThumbnail( std::shared_ptr<Thumbnail> newThumbnail,
                           Thumbnail::Origin origin )
{
    assert( newThumbnail != nullptr );

    auto thumbnailIdx = Thumbnail::SizeToInt( newThumbnail->sizeType() );
    auto currentThumbnail = thumbnail( newThumbnail->sizeType() );
    if ( currentThumbnail != nullptr &&
         shouldUpdateThumbnail( *newThumbnail, origin ) == false )
            return true;
    currentThumbnail = Thumbnail::updateOrReplace( m_ml, currentThumbnail,
                                                   newThumbnail, m_id,
                                                   Thumbnail::EntityType::Artist );
    auto res = currentThumbnail != nullptr;
    m_thumbnails[thumbnailIdx] = std::move( currentThumbnail );
    return res;
}

bool Artist::setThumbnail( const std::string& thumbnailMrl, ThumbnailSizeType sizeType )
{
    return setThumbnail( std::make_shared<Thumbnail>( m_ml,
                thumbnailMrl, Thumbnail::Origin::UserProvided, sizeType, false ),
                Thumbnail::Origin::UserProvided );
}

std::shared_ptr<Album> Artist::unknownAlbum()
{
    static const std::string req = "SELECT * FROM " + Album::Table::Name +
                        " WHERE artist_id = ? AND title IS NULL";
    auto album = Album::fetch( m_ml, req, m_id );
    if ( album == nullptr )
    {
        album = Album::createUnknownAlbum( m_ml, this );
        if ( album == nullptr )
            return nullptr;
        m_nbAlbums = 1;
    }
    return album;
}

const std::string& Artist::musicBrainzId() const
{
    return m_mbId;
}

bool Artist::setMusicBrainzId( const std::string& mbId )
{
    static const std::string req = "UPDATE " + Artist::Table::Name
            + " SET mb_id = ? WHERE id_artist = ?";
    if ( mbId == m_mbId )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, mbId, m_id ) == false )
        return false;
    m_mbId = mbId;
    return true;
}

unsigned int Artist::nbAlbums() const
{
    return m_nbAlbums;
}

unsigned int Artist::nbTracks() const
{
    return m_nbTracks;
}

void Artist::createTable( sqlite::Connection* dbConnection )
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
        schema( MediaRelationTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Artist::createTriggers( sqlite::Connection* dbConnection, uint32_t dbModelVersion )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::HasTrackPresent, dbModelVersion ) );

    if ( dbModelVersion < 23 )
    {
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::HasAlbumRemaining, dbModelVersion ) );
    }

    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::InsertFts, dbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteFts, dbModelVersion ) );

    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeleteArtistsWithoutTracks, dbModelVersion ) );
    if ( dbModelVersion >= 23 )
    {
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::IncrementNbTracks, dbModelVersion ) );
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::DecrementNbTracks, dbModelVersion ) );
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::UpdateNbAlbums, dbModelVersion ) );
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::DecrementNbAlbums, dbModelVersion ) );
        sqlite::Tools::executeRequest( dbConnection,
                                       trigger( Triggers::IncrementNbAlbums, dbModelVersion ) );
    }
}

std::string Artist::schema( const std::string& tableName, uint32_t dbModelVersion )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
                " USING FTS3(name)";
    }
    else if ( tableName == MediaRelationTable::Name )
    {
        return "CREATE TABLE " + MediaRelationTable::Name +
        "("
            "media_id INTEGER NOT NULL,"
            "artist_id INTEGER,"
            "PRIMARY KEY(media_id,artist_id),"
            "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
            "(id_media) ON DELETE CASCADE,"
            "FOREIGN KEY(artist_id) REFERENCES " + Table::Name + "("
                + Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
        ")";
    }
    assert( tableName == Table::Name );
    if ( dbModelVersion <= 16 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
            "shortbio TEXT,"
            "thumbnail_id TEXT,"
            "nb_albums UNSIGNED INT DEFAULT 0,"
            "nb_tracks UNSIGNED INT DEFAULT 0,"
            "mb_id TEXT,"
            "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
                "CHECK(is_present <= nb_tracks),"
            "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
            + "(id_thumbnail)"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
        "shortbio TEXT,"
        // Contains the number of albums where this artist is the album
        // artist. Some or all albums might not be present.
        "nb_albums UNSIGNED INT DEFAULT 0,"
        // Contains the number of tracks by this artist, regardless of
        // the album they appear on. Some or all tracks might not be present
        "nb_tracks UNSIGNED INT DEFAULT 0,"
        "mb_id TEXT,"
        // A presence flag, that represents the number of present tracks.
        // This is not linked to the album presence at all.
        // An artist of which all tracks are not present is considered
        // not present, even if one of its album contains a present
        // track from another artist (the album will be present, however)
        "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
            "CHECK(is_present <= nb_tracks)"
        ")";
}

std::string Artist::trigger( Triggers trigger, uint32_t dbModelVersion )
{
    switch ( trigger )
    {
        case Triggers::HasTrackPresent:
        {
            if ( dbModelVersion < 23 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                       " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                       " WHEN new.subtype = " +
                           std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                                               IMedia::SubType::AlbumTrack ) ) +
                       " BEGIN "
                       " UPDATE " + Table::Name + " SET is_present=is_present + "
                           "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                           "WHERE id_artist = (SELECT artist_id FROM " + AlbumTrack::Table::Name + " "
                               " WHERE media_id = new.id_media "
                           ");"
                       " END";
            }
            return "CREATE TRIGGER "  + triggerName( trigger, dbModelVersion ) +
                   " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                   " WHEN new.subtype = " +
                       std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                                           IMedia::SubType::AlbumTrack ) ) +
                   " AND old.is_present != new.is_present"
                   " BEGIN "
                   " UPDATE " + Table::Name + " SET is_present=is_present + "
                       "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                       "WHERE id_artist = (SELECT artist_id FROM " + AlbumTrack::Table::Name + " "
                           " WHERE media_id = new.id_media "
                       ");"
                   " END";
        }
        case Triggers::HasAlbumRemaining:
        {
            assert( dbModelVersion < 23 );
            // Automatically delete the artists that don't have any albums left,
            // except the 2 special artists.
            // Those are assumed to always exist, and deleting them would cause
            // a constaint violation error when inserting an album with
            // unknown/various artist(s).
            // The alternative would be to always check the special artists for
            // existence, which would be much slower when inserting an unknown
            // artist album
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER DELETE ON " + Album::Table::Name +
                   " WHEN old.artist_id != " + std::to_string( UnknownArtistID ) +
                   " AND  old.artist_id != " + std::to_string( VariousArtistID ) +
                   " BEGIN"
                        " UPDATE " + Table::Name + " SET nb_albums = nb_albums - 1"
                            " WHERE id_artist = old.artist_id;"
                        " DELETE FROM " + Table::Name +
                            " WHERE id_artist = old.artist_id AND nb_albums = 0"
                                   " AND nb_tracks = 0;"
                   " END";
        }
        case Triggers::DeleteArtistsWithoutTracks:
        {
            if ( dbModelVersion >= 8 && dbModelVersion < 23 )
            {
                // Don't create this trigger if the database is about to be migrated.
                // This could make earlier migration fail, and needs to be done when
                // migrating to v7 to v8.
                // While the has_album_remaining trigger now also references the nb_tracks
                // field, it was present from before version 3, so it wouldn't be recreated.
                // As we don't support any model before 3 (or rather we just recreate
                // everything), we don't have to bother here.
                return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                       " AFTER DELETE ON " + AlbumTrack::Table::Name +
                       " WHEN old.artist_id != " + std::to_string( UnknownArtistID ) +
                       " AND  old.artist_id != " + std::to_string( VariousArtistID ) +
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET"
                           " nb_tracks = nb_tracks - 1,"
                           " is_present = is_present - 1"
                           " WHERE id_artist = old.artist_id;"
                       " DELETE FROM " + Table::Name + " WHERE id_artist = old.artist_id "
                            " AND nb_albums = 0 "
                            " AND nb_tracks = 0;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER UPDATE OF nb_tracks, nb_albums ON " + Table::Name +
                   " WHEN new.nb_tracks = 0 AND new.nb_albums = 0"
                       " AND new.id_artist != " + std::to_string( UnknownArtistID ) +
                       " AND new.id_artist != " + std::to_string( VariousArtistID ) +
                   " BEGIN"
                       " DELETE FROM " + Table::Name + " WHERE id_artist = old.id_artist;"
                   " END";
        }
        case Triggers::IncrementNbTracks:
        {
            assert( dbModelVersion >= 23 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER INSERT ON " + MediaRelationTable::Name +
                   " BEGIN"
                       " UPDATE " + Table::Name +
                       " SET nb_tracks = nb_tracks + 1, is_present = is_present + 1"
                           " WHERE id_artist = new.artist_id;"
                   " END";
        }
        case Triggers::DecrementNbTracks:
        {
            assert( dbModelVersion >= 23 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER DELETE ON " + MediaRelationTable::Name +
                   " BEGIN"
                       " UPDATE " + Table::Name +
                       " SET nb_tracks = nb_tracks - 1, is_present = is_present - 1"
                            " WHERE id_artist = old.artist_id;"
                   " END";
        }
        case Triggers::UpdateNbAlbums:
        {
            assert( dbModelVersion >= 23 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER UPDATE OF artist_id ON " + Album::Table::Name +
                   " BEGIN"
                       " UPDATE " + Table::Name + " SET nb_albums = nb_albums + 1"
                           " WHERE id_artist = new.artist_id;"
                       // Even if this is the first update, the old value will be NULL
                       // and won't update anything
                       " UPDATE " + Table::Name + " SET nb_albums = nb_albums - 1"
                           " WHERE id_artist = old.artist_id;"
                   " END";
        }
        case Triggers::DecrementNbAlbums:
        {
            assert( dbModelVersion >= 23 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER DELETE ON " + Album::Table::Name +
                   " BEGIN"
                       " UPDATE " + Table::Name + " SET nb_albums = nb_albums - 1"
                           " WHERE id_artist = old.artist_id;"
                   " END";
        }
        case Triggers::IncrementNbAlbums:
        {
            assert( dbModelVersion >= 23 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER INSERT ON " + Album::Table::Name +
                   " WHEN new.artist_id IS NOT NULL"
                   " BEGIN"
                       " UPDATE " + Table::Name + " SET nb_albums = nb_albums + 1"
                           " WHERE id_artist = new.artist_id;"
                   " END";
        }
        case Triggers::InsertFts:
        {
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " AFTER INSERT ON " + Table::Name +
                   " WHEN new.name IS NOT NULL"
                   " BEGIN"
                        " INSERT INTO " + FtsTable::Name + "(rowid,name)"
                            " VALUES(new.id_artist, new.name);"
                   " END";
        }
        case Triggers::DeleteFts:
        {
            return "CREATE TRIGGER " + triggerName( trigger, dbModelVersion ) +
                   " BEFORE DELETE ON " + Table::Name +
                   " WHEN old.name IS NOT NULL"
                   " BEGIN"
                        " DELETE FROM " + FtsTable::Name +
                            " WHERE rowid=old.id_artist;"
                   " END";
        }

        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Artist::triggerName(Artist::Triggers trigger, uint32_t dbModelVersion)
{
    switch ( trigger )
    {
        case Triggers::HasTrackPresent:
        {
            if ( dbModelVersion < 23 )
                return "has_tracks_present";
            return "artist_has_tracks_present";
        }
        case Triggers::HasAlbumRemaining:
            return "has_album_remaining";
        case Triggers::DeleteArtistsWithoutTracks:
        {
            if ( dbModelVersion >= 8 && dbModelVersion < 23 )
                return "has_track_remaining";
            return "delete_artist_without_tracks";
        }
        case Triggers::IncrementNbTracks:
            return "artist_increment_nb_tracks";
        case Triggers::DecrementNbTracks:
            return "artist_decrement_nb_tracks";
        case Triggers::UpdateNbAlbums:
            return "artist_update_nb_albums";
        case Triggers::DecrementNbAlbums:
            return "artist_decrement_nb_albums";
        case Triggers::IncrementNbAlbums:
            return "artist_increment_nb_albums_unknown_album";
        case Triggers::InsertFts:
            return "insert_artist_fts";
        case Triggers::DeleteFts:
            return "delete_artist_fts";

        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

bool Artist::checkDbModel(MediaLibraryPtr ml)
{
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( MediaRelationTable::Name, Settings::DbModelVersion ),
                                       MediaRelationTable::Name ) == false )
        return false;

    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    return check( ml->getConn(), Triggers::HasTrackPresent ) &&
            check( ml->getConn(), Triggers::DeleteArtistsWithoutTracks ) &&
            check( ml->getConn(), Triggers::IncrementNbTracks ) &&
            check( ml->getConn(), Triggers::DecrementNbTracks ) &&
            check( ml->getConn(), Triggers::UpdateNbAlbums ) &&
            check( ml->getConn(), Triggers::DecrementNbAlbums ) &&
            check( ml->getConn(), Triggers::IncrementNbAlbums ) &&
            check( ml->getConn(), Triggers::InsertFts ) &&
            check( ml->getConn(), Triggers::DeleteFts );
}

bool Artist::createDefaultArtists( sqlite::Connection* dbConnection )
{
    static const std::string req = "INSERT INTO " + Artist::Table::Name +
            "(id_artist) VALUES(?),(?)";
    return sqlite::Tools::executeInsert( dbConnection, req, UnknownArtistID,
                                         VariousArtistID ) != 0;
}

std::shared_ptr<Artist> Artist::create( MediaLibraryPtr ml, const std::string& name )
{
    auto artist = std::make_shared<Artist>( ml, name );
    static const std::string req = "INSERT INTO " + Artist::Table::Name +
            "(id_artist, name) VALUES(NULL, ?)";
    if ( insert( ml, artist, req, name ) == false )
        return nullptr;
    return artist;
}

Query<IArtist> Artist::search( MediaLibraryPtr ml, const std::string& name,
                               ArtistIncluded included, const QueryParameters* params )
{
    std::string req = "FROM " + Artist::Table::Name + " WHERE id_artist IN "
            "(SELECT rowid FROM " + Artist::FtsTable::Name + " WHERE name MATCH ?)"
            "AND is_present != 0";
    // We are searching based on the name, so we're ignoring unknown/various artist
    // This means all artist we find has at least one track associated with it, so
    // we can simply filter out based on the number of associated albums
    if ( included == ArtistIncluded::AlbumArtistOnly )
        req += " AND nb_albums > 0";
    return make_query<Artist, IArtist>( ml, "*", std::move( req ),
                                        sortRequest( params ),
                                        sqlite::Tools::sanitizePattern( name ) );
}

Query<IArtist> Artist::listAll( MediaLibraryPtr ml, ArtistIncluded included,
                                const QueryParameters* params )
{
    std::string req = "FROM " + Artist::Table::Name + " WHERE ";
    if ( included == ArtistIncluded::AlbumArtistOnly )
        req += "nb_albums > 0 AND";

    req += " is_present != 0";
    return make_query<Artist, IArtist>( ml, "*", std::move( req ),
                                        sortRequest( params ) );
}

Query<IArtist> Artist::searchByGenre( MediaLibraryPtr ml, const std::string& pattern,
                                      const QueryParameters* params, int64_t genreId )
{
    std::string req = "FROM " + Artist::Table::Name + " a "
                "INNER JOIN " + AlbumTrack::Table::Name + " att ON att.artist_id = a.id_artist "
                "WHERE id_artist IN "
                    "(SELECT rowid FROM " + Artist::FtsTable::Name + " WHERE name MATCH ?)"
                "AND att.genre_id = ? ";

    std::string groupBy = "GROUP BY att.artist_id "
                          "ORDER BY a.name";
    if ( params != nullptr )
    {
        if ( params->sort != SortingCriteria::Default && params->sort != SortingCriteria::Alpha )
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
        if ( params->desc == true )
            groupBy += " DESC";
    }
    return make_query<Artist, IArtist>( ml, "a.*", std::move( req ),
                                        std::move( groupBy ),
                                        sqlite::Tools::sanitizePattern( pattern ),
                                        genreId );
}

bool Artist::dropMediaArtistRelation( MediaLibraryPtr ml, int64_t mediaId )
{
    const std::string req = "DELETE FROM " + MediaRelationTable::Name +
            " WHERE media_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

std::string Artist::sortRequest( const QueryParameters* params )
{
    std::string req = " ORDER BY name";
    if ( params != nullptr )
    {
        if ( params->sort != SortingCriteria::Default && params->sort != SortingCriteria::Alpha )
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
        if ( params->desc == true )
            req +=  " DESC";
    }
    return req;
}

bool Artist::checkDBConsistency( MediaLibraryPtr ml )
{
    sqlite::Statement stmt{ ml->getConn()->handle(),
                "SELECT nb_tracks, is_present FROM " +
                    Artist::Table::Name
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
