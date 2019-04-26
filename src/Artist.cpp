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

Artist::Artist( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_shortBio( row.extract<decltype(m_shortBio)>() )
    , m_thumbnailId( row.extract<decltype(m_thumbnailId)>() )
    , m_nbAlbums( row.extract<decltype(m_nbAlbums)>() )
    , m_nbTracks( row.extract<decltype(m_nbTracks)>() )
    , m_mbId( row.extract<decltype(m_mbId)>() )
    , m_isPresent( row.extract<decltype(m_isPresent)>() )
{
}

Artist::Artist( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_thumbnailId( 0 )
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
    std::string req = "FROM " + Media::Table::Name + " med ";

    SortingCriteria sort = params != nullptr ? params->sort : SortingCriteria::Default;
    bool desc = params != nullptr ? params->desc : false;
    // Various artist is a special artist that doesn't have tracks per-se.
    // Rather, it's a virtual artist for albums with many artist but no declared
    // album artist. When listing its tracks, we need to list those by albums
    // instead of listing all tracks by this artist, as there will be none.
    if ( m_id != VariousArtistID )
    {
        req += "INNER JOIN MediaArtistRelation mar ON mar.media_id = med.id_media ";
        if ( sort != SortingCriteria::Duration &&
             sort != SortingCriteria::InsertionDate &&
             sort != SortingCriteria::ReleaseDate &&
             sort != SortingCriteria::Alpha )
        {
            req += "INNER JOIN Album alb ON alb.id_album = atr.album_id "
                   "INNER JOIN AlbumTrack atr ON atr.media_id = med.id_media ";
        }
        req += "WHERE mar.artist_id = ? ";
    }
    else
    {
        req += "INNER JOIN AlbumTrack atr ON atr.media_id = med.id_media "
               "INNER JOIN Album alb ON alb.id_album = atr.album_id "
               "WHERE alb.artist_id = ? ";
    }

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
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
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
    static const std::string req = "INSERT INTO MediaArtistRelation VALUES(?, ?)";
    // If track's ID is 0, the request will fail due to table constraints
    sqlite::ForeignKey artistForeignKey( m_id );
    return sqlite::Tools::executeInsert( m_ml->getConn(), req, media.id(), artistForeignKey ) != 0;
}

bool Artist::isThumbnailGenerated() const
{
    return m_thumbnailId != 0;
}

const std::string& Artist::thumbnailMrl() const
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

std::shared_ptr<Thumbnail> Artist::thumbnail()
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

bool Artist::setThumbnail( std::shared_ptr<Thumbnail> thumbnail )
{
    assert( thumbnail != nullptr );

    if ( m_thumbnailId != 0 )
    {
        if ( m_thumbnail == nullptr )
        {
            auto m_thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
            if ( m_thumbnail == nullptr )
                return false;
        }
        if ( m_thumbnail->origin() == Thumbnail::Origin::Artist ||
             m_thumbnail->origin() == Thumbnail::Origin::UserProvided )
            return m_thumbnail->update( thumbnail->mrl(),
                                        thumbnail->origin(),
                                        thumbnail->isGenerated() );
    }
    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = m_ml->getConn()->newTransaction();
    if ( thumbnail->id() == 0 )
    {
        if ( thumbnail->insert() == 0 )
            return false;
    }
    static const std::string req = "UPDATE " + Artist::Table::Name +
            " SET thumbnail_id = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, thumbnail->id(),
                                       m_id ) == false )
        return false;
    if ( t != nullptr )
        t->commit();
    m_thumbnailId = thumbnail->id();
    m_thumbnail = std::move( thumbnail );
    return true;
}

bool Artist::setArtworkMrl( const std::string& thumbnailMrl,
                            Thumbnail::Origin origin, bool isGenerated )
{
    return setThumbnail( std::make_shared<Thumbnail>( m_ml, thumbnailMrl,
                                                      origin, isGenerated ) );
}

bool Artist::updateNbAlbum( int increment )
{
    assert( increment != 0 );
    assert( increment > 0 || ( increment < 0 && m_nbAlbums >= 1 ) );

    static const std::string req = "UPDATE " + Artist::Table::Name +
            " SET nb_albums = nb_albums + ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, increment, m_id ) == false )
        return false;
    m_nbAlbums += increment;
    return true;
}

bool Artist::updateNbTrack(int increment)
{
    assert( increment != 0 );
    assert( increment > 0 || ( increment < 0 && m_nbTracks >= 1 ) );
    static const std::string req = "UPDATE " + Artist::Table::Name +
            " SET nb_tracks = nb_tracks + ?, is_present = is_present + ?"
            " WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, increment,
                                       increment, m_id ) == false )
        return false;
    m_nbTracks += increment;
    return true;
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
        if ( updateNbAlbum( 1 ) == false )
        {
            Album::destroy( m_ml, album->id() );
            return nullptr;
        }
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
    const std::string req = "CREATE TABLE IF NOT EXISTS " +
            Artist::Table::Name +
            "("
                "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
                "shortbio TEXT,"
                "thumbnail_id TEXT,"
                "nb_albums UNSIGNED INT DEFAULT 0,"
                "nb_tracks UNSIGNED INT DEFAULT 0,"
                "mb_id TEXT,"
                "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0,"
                "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
                + "(id_thumbnail)"
            ")";
    const std::string reqRel = "CREATE TABLE IF NOT EXISTS MediaArtistRelation("
                "media_id INTEGER NOT NULL,"
                "artist_id INTEGER,"
                "PRIMARY KEY (media_id, artist_id),"
                "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
                "(id_media) ON DELETE CASCADE,"
                "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name + "("
                    + Artist::Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    const std::string reqFts = "CREATE VIRTUAL TABLE IF NOT EXISTS " +
                Artist::Table::Name + "Fts USING FTS3("
                "name"
            ")";
    sqlite::Tools::executeRequest( dbConnection, req );
    sqlite::Tools::executeRequest( dbConnection, reqRel );
    sqlite::Tools::executeRequest( dbConnection, reqFts );
}

void Artist::createTriggers( sqlite::Connection* dbConnection, uint32_t dbModelVersion )
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS has_tracks_present AFTER UPDATE OF "
            "is_present ON " + Media::Table::Name +
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
    // Automatically delete the artists that don't have any albums left, except the 2 special artists.
    // Those are assumed to always exist, and deleting them would cause a constaint violation error
    // when inserting an album with unknown/various artist(s).
    // The alternative would be to always check the special artists for existence, which would be much
    // slower when inserting an unknown artist album
    static const std::string autoDeleteAlbumTriggerReq = "CREATE TRIGGER IF NOT EXISTS has_album_remaining"
            " AFTER DELETE ON " + Album::Table::Name +
            " WHEN old.artist_id != " + std::to_string( UnknownArtistID ) +
            " AND  old.artist_id != " + std::to_string( VariousArtistID ) +
            " BEGIN"
            " UPDATE " + Artist::Table::Name + " SET nb_albums = nb_albums - 1 WHERE id_artist = old.artist_id;"
            " DELETE FROM " + Artist::Table::Name + " WHERE id_artist = old.artist_id "
            " AND nb_albums = 0 "
            " AND nb_tracks = 0;"
            " END";

    static const std::string autoDeleteTrackTriggerReq = "CREATE TRIGGER IF NOT EXISTS has_track_remaining"
            " AFTER DELETE ON " + AlbumTrack::Table::Name +
            " WHEN old.artist_id != " + std::to_string( UnknownArtistID ) +
            " AND  old.artist_id != " + std::to_string( VariousArtistID ) +
            " BEGIN"
            " UPDATE " + Artist::Table::Name + " SET"
                " nb_tracks = nb_tracks - 1,"
                " is_present = is_present - 1"
                " WHERE id_artist = old.artist_id;"
            " DELETE FROM " + Artist::Table::Name + " WHERE id_artist = old.artist_id "
            " AND nb_albums = 0 "
            " AND nb_tracks = 0;"
            " END";

    static const std::string ftsInsertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_artist_fts"
            " AFTER INSERT ON " + Artist::Table::Name +
            " WHEN new.name IS NOT NULL"
            " BEGIN"
            " INSERT INTO " + Artist::Table::Name + "Fts(rowid,name) VALUES(new.id_artist, new.name);"
            " END";
    static const std::string ftsDeleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_artist_fts"
            " BEFORE DELETE ON " + Artist::Table::Name +
            " WHEN old.name IS NOT NULL"
            " BEGIN"
            " DELETE FROM " + Artist::Table::Name + "Fts WHERE rowid=old.id_artist;"
            " END";
    sqlite::Tools::executeRequest( dbConnection, triggerReq );
    sqlite::Tools::executeRequest( dbConnection, autoDeleteAlbumTriggerReq );
    // Don't create this trigger if the database is about to be migrated.
    // This could make earlier migration fail, and needs to be done when
    // migrating to v7 to v8.
    // While the has_album_remaining trigger now also references the nb_tracks
    // field, it was present from before version 3, so it wouldn't be recreated.
    // As we don't support any model before 3 (or rather we just recreate
    // everything), we don't have to bother here.
    if ( dbModelVersion >= 8 )
    {
        sqlite::Tools::executeRequest( dbConnection, autoDeleteTrackTriggerReq );
    }
    sqlite::Tools::executeRequest( dbConnection, ftsInsertTrigger );
    sqlite::Tools::executeRequest( dbConnection, ftsDeleteTrigger );
}

bool Artist::createDefaultArtists( sqlite::Connection* dbConnection )
{
    // Don't rely on Artist::create, since we want to insert or do nothing here.
    // This will skip the cache for those new entities, but they will be inserted soon enough anyway.
    static const std::string req = "INSERT OR IGNORE INTO " + Artist::Table::Name +
            "(id_artist) VALUES(?),(?)";
    sqlite::Tools::executeInsert( dbConnection, req, UnknownArtistID,
                                          VariousArtistID );
    // Always return true. The insertion might succeed, but we consider it a failure when 0 row
    // gets inserted, while we are explicitely specifying "OR IGNORE" here.
    return true;
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
                               bool includeAll, const QueryParameters* params )
{
    std::string req = "FROM " + Artist::Table::Name + " WHERE id_artist IN "
            "(SELECT rowid FROM " + Artist::Table::Name + "Fts WHERE name MATCH '*' || ? || '*')"
            "AND is_present != 0";
    // We are searching based on the name, so we're ignoring unknown/various artist
    // This means all artist we find has at least one track associated with it, so
    // we can simply filter out based on the number of associated albums
    if ( includeAll == false )
        req += " AND nb_albums > 0";
    return make_query<Artist, IArtist>( ml, "*", std::move( req ),
                                        sortRequest( params ), name );
}

Query<IArtist> Artist::listAll( MediaLibraryPtr ml, bool includeAll,
                                const QueryParameters* params )
{
    std::string req = "FROM " + Artist::Table::Name + " WHERE ";
    if ( includeAll == true )
        req += "( nb_albums > 0 OR nb_tracks > 0 )";
    else
        req += "nb_albums > 0";

    req += " AND is_present != 0";
    return make_query<Artist, IArtist>( ml, "*", std::move( req ),
                                        sortRequest( params ) );
}

Query<IArtist> Artist::searchByGenre( MediaLibraryPtr ml, const std::string& pattern,
                                      const QueryParameters* params, int64_t genreId )
{
    std::string req = "FROM " + Artist::Table::Name + " a "
                "INNER JOIN " + AlbumTrack::Table::Name + " att ON att.artist_id = a.id_artist "
                "WHERE id_artist IN "
                    "(SELECT rowid FROM " + Artist::Table::Name + "Fts WHERE name MATCH '*' || ? || '*')"
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
                                        std::move( groupBy ), pattern, genreId );
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

}
