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

#include "Genre.h"

#include "Album.h"
#include "Artist.h"
#include "Media.h"
#include "database/SqliteQuery.h"
#include "utils/Enums.h"
#include "utils/ModificationsNotifier.h"
#include "Deprecated.h"

namespace medialibrary
{

const std::string Genre::Table::Name = "Genre";
const std::string Genre::Table::PrimaryKeyColumn = "id_genre";
int64_t Genre::* const Genre::Table::PrimaryKey = &Genre::m_id;
const std::string Genre::FtsTable::Name = "GenreFts";

Genre::Genre( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_nbTracks( row.extract<decltype(m_nbTracks)>() )
    , m_nbPresentTracks( row.extract<decltype(m_nbPresentTracks)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Genre::Genre( MediaLibraryPtr ml, std::string name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( std::move( name ) )
    , m_nbTracks( 0 )
    , m_nbPresentTracks( 0 )
{
}

int64_t Genre::id() const
{
    return m_id;
}

const std::string& Genre::name() const
{
    return m_name;
}

uint32_t Genre::nbTracks() const
{
    return m_nbTracks;
}

uint32_t Genre::nbPresentTracks() const
{
    return m_nbPresentTracks;
}

bool Genre::updateNbTracks( int increment )
{
    static const std::string req = "UPDATE " + Table::Name +
            " SET nb_tracks = nb_tracks + ?1,"
            " is_present = is_present + ?1 WHERE id_genre = ?2";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, increment, m_id ) == false )
        return false;
    m_nbTracks += increment;
    m_nbPresentTracks += increment;
    return true;
}

Query<IArtist> Genre::artists( const QueryParameters* params ) const
{
    std::string req = "FROM " + Artist::Table::Name + " a "
            "INNER JOIN " + Media::Table::Name + " m ON m.artist_id = a.id_artist "
            "WHERE m.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY m.artist_id ORDER BY a.name";
    if ( params != nullptr )
    {
        if ( params->sort != SortingCriteria::Default && params->sort != SortingCriteria::Alpha )
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
        if ( params->desc == true )
            groupAndOrderBy += " DESC";
    }
    return make_query<Artist, IArtist>( m_ml, "a.*", std::move( req ),
                                        std::move( groupAndOrderBy ), m_id );
}

Query<IArtist> Genre::searchArtists( const std::string& pattern,
                                    const QueryParameters* params ) const
{
    return Artist::searchByGenre( m_ml, pattern, params, m_id );
}

Query<IMedia> Genre::tracks( TracksIncluded included,
                             const QueryParameters* params ) const
{
    return Media::tracksFromGenre( m_ml, m_id, included, params );
}

Query<IMedia> Genre::searchTracks( const std::string& pattern, const QueryParameters* params ) const
{
    return Media::searchGenreTracks( m_ml, pattern, m_id, params );
}

Query<IAlbum> Genre::albums( const QueryParameters* params ) const
{
    return Album::fromGenre( m_ml, m_id, params );
}

Query<IAlbum> Genre::searchAlbums( const std::string& pattern,
                                   const QueryParameters* params ) const
{
    return Album::searchFromGenre( m_ml, pattern, m_id, params );
}

const std::string&Genre::thumbnailMrl( ThumbnailSizeType sizeType ) const
{
    const auto t = thumbnail( sizeType );
    if ( t == nullptr )
        return Thumbnail::EmptyMrl;
    return t->mrl();
}

bool Genre::hasThumbnail( ThumbnailSizeType sizeType ) const
{
    if ( m_thumbnails[Thumbnail::SizeToInt( sizeType )] != nullptr )
        return true;
    return thumbnail( sizeType ) != nullptr;
}

bool Genre::shouldUpdateThumbnail( const Thumbnail& oldThumbnail )
{
    return oldThumbnail.isShared() == false;
}

bool Genre::setThumbnail( const std::string& mrl, ThumbnailSizeType sizeType,
                          bool takeOwnership )
{
    auto thumbnailIdx = Thumbnail::SizeToInt( sizeType );
    auto currentThumbnail = thumbnail( sizeType );
    auto newThumbnail = std::make_shared<Thumbnail>( m_ml, mrl,
                            Thumbnail::Origin::UserProvided, sizeType, false );
    currentThumbnail = Thumbnail::updateOrReplace( m_ml, currentThumbnail,
                                                   newThumbnail,
                                                   Genre::shouldUpdateThumbnail,
                                                   m_id, Thumbnail::EntityType::Genre );
    if ( currentThumbnail == nullptr )
        return false;
    m_thumbnails[thumbnailIdx] = std::move( currentThumbnail );
    if ( takeOwnership == true )
        m_thumbnails[thumbnailIdx]->relocate();
    auto notifier = m_ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyGenreModification( m_id );
    return true;
}

std::shared_ptr<Thumbnail> Genre::thumbnail( ThumbnailSizeType sizeType ) const
{
    auto idx = Thumbnail::SizeToInt( sizeType );
    if ( m_thumbnails[idx] == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, Thumbnail::EntityType::Genre,
                                           m_id, sizeType );
        if ( thumbnail == nullptr )
            return nullptr;
        m_thumbnails[idx] = std::move( thumbnail );
    }
    return m_thumbnails[idx];
}

void Genre::createTable( sqlite::Connection* dbConn )
{
    const std::string reqs[] = {
        schema( Table::Name, Settings::DbModelVersion ),
        schema( FtsTable::Name, Settings::DbModelVersion ),
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
}

void Genre::createTriggers( sqlite::Connection* dbConn )
{
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::InsertFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::DeleteFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::UpdateOnTrackDelete, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::UpdateIsPresent, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConn,
                                   trigger( Triggers::DeleteEmpty, Settings::DbModelVersion ) );
}

std::string Genre::schema( const std::string& tableName, uint32_t dbModel )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
               " USING FTS3(name)";
    }
    assert( tableName == Table::Name );
    if ( dbModel < 30 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_genre INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
            "nb_tracks INTEGER NOT NULL DEFAULT 0"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_genre INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
        "nb_tracks INTEGER NOT NULL DEFAULT 0,"
        "is_present INTEGER NOT NULL DEFAULT 0 "
            "CHECK(is_present <= nb_tracks)"
    ")";
}

std::string Genre::trigger( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::InsertFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER INSERT ON " + Table::Name +
                   " BEGIN"
                        " INSERT INTO " + FtsTable::Name + "(rowid,name)"
                            " VALUES(new.id_genre, new.name);"
                   " END";
        case Triggers::DeleteFts:
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " BEFORE DELETE ON " + Table::Name +
                   " BEGIN"
                        " DELETE FROM " + FtsTable::Name +
                            " WHERE rowid = old.id_genre;"
                   " END";
        case Triggers::UpdateOnNewTrack:
            assert( dbModel < 34 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                    " AFTER INSERT ON " + AlbumTrack::Table::Name +
                    " WHEN new.genre_id IS NOT NULL"
                    " BEGIN"
                        " UPDATE " + Table::Name +
                            " SET nb_tracks = nb_tracks + 1,"
                                " is_present = is_present + 1"
                                " WHERE id_genre = new.genre_id;"
                    " END";
        case Triggers::UpdateOnTrackDelete:
            if ( dbModel < 34 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER DELETE ON " + AlbumTrack::Table::Name +
                       " WHEN old.genre_id IS NOT NULL"
                       " BEGIN"
                            " UPDATE " + Table::Name +
                                " SET nb_tracks = nb_tracks - 1,"
                                    " is_present = is_present - 1"
                                    " WHERE id_genre = old.genre_id;"
                            " DELETE FROM " + Table::Name +
                                " WHERE nb_tracks = 0;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER DELETE ON " + Media::Table::Name +
                    " WHEN old.subtype = " +
                        utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                   " BEGIN"
                        " UPDATE " + Table::Name +
                            " SET nb_tracks = nb_tracks - 1,"
                                " is_present = is_present - IIF(old.is_present != 0, 1, 0)"
                                " WHERE id_genre = old.genre_id;"
                   " END";
        case Triggers::UpdateIsPresent:
            assert( dbModel >= 30 );
            if ( dbModel < 34 )
            {
                return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                       " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                       " WHEN new.subtype = " +
                            utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                       " AND old.is_present != new.is_present"
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET is_present = is_present + "
                           "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) "
                           "WHERE id_genre = "
                               "(SELECT genre_id FROM " + AlbumTrack::Table::Name +
                                   " WHERE media_id = new.id_media);"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                   " WHEN new.subtype = " +
                        utils::enum_to_string( IMedia::SubType::AlbumTrack ) +
                   " AND old.is_present != new.is_present"
                   " BEGIN"
                   " UPDATE " + Table::Name + " SET is_present = is_present + "
                       "(CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) "
                       "WHERE id_genre = new.genre_id;"
                   " END";
        case Triggers::DeleteEmpty:
        {
            assert( dbModel >= 34 );
            return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
                   " AFTER UPDATE OF nb_tracks ON " + Table::Name +
                   " WHEN new.nb_tracks = 0"
                   " BEGIN"
                        " DELETE FROM " + Table::Name + ";"
                   " END";
        }
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

std::string Genre::triggerName( Triggers trigger, uint32_t dbModel )
{
    switch ( trigger )
    {
        case Triggers::InsertFts:
            return "insert_genre_fts";
        case Triggers::DeleteFts:
            return "delete_genre_fts";
        case Triggers::UpdateOnNewTrack:
            assert( dbModel < 34 );
            return "update_genre_on_new_track";
        case Triggers::UpdateOnTrackDelete:
            if ( dbModel < 34 )
                return "update_genre_on_track_deleted";
            return "genre_update_on_track_deleted";
        case Triggers::UpdateIsPresent:
            assert( dbModel >= 30 );
            return "genre_update_is_present";
        case Triggers::DeleteEmpty:
            assert( dbModel >= 34 );
            return "genre_delete_empty";
        default:
            assert( !"Invalid trigger provided" );
    }
    return "<invalid request>";
}

bool Genre::checkDbModel(MediaLibraryPtr ml)
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false )
        return false;
    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    return check( ml->getConn(), Triggers::InsertFts ) &&
            check( ml->getConn(), Triggers::DeleteFts ) &&
            check( ml->getConn(), Triggers::UpdateOnTrackDelete ) &&
            check( ml->getConn(), Triggers::UpdateIsPresent ) &&
            check( ml->getConn(), Triggers::DeleteEmpty );
}

std::shared_ptr<Genre> Genre::create( MediaLibraryPtr ml, std::string name )
{
    static const std::string req = "INSERT INTO " + Table::Name + "(name)"
            "VALUES(?)";
    auto self = std::make_shared<Genre>( ml, std::move( name ) );
    if ( insert( ml, self, req, self->m_name ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Genre> Genre::fromName( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + Table::Name + " WHERE name = ?";
    return fetch( ml, req, name );
}

Query<IGenre> Genre::search( MediaLibraryPtr ml, const std::string& name,
                             const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " WHERE id_genre IN "
            "(SELECT rowid FROM " + FtsTable::Name + " "
            "WHERE name MATCH ?)";
    std::string orderBy = "ORDER BY name";
    if ( params != nullptr )
    {
        if ( params->sort != SortingCriteria::Default && params->sort != SortingCriteria::Alpha )
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
        if ( params->desc == true )
            orderBy += " DESC";
    }
    return make_query<Genre, IGenre>( ml, "*", std::move( req ),
                                      std::move( orderBy ),
                                      sqlite::Tools::sanitizePattern( name ) );
}

Query<IGenre> Genre::listAll( MediaLibraryPtr ml, const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name;
    std::string orderBy = " ORDER BY name";
    if ( params != nullptr )
    {
        if ( params->sort != SortingCriteria::Default && params->sort != SortingCriteria::Alpha )
            LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Alpha" );
        if ( params->desc == true )
            orderBy += " DESC";
    }
    return make_query<Genre, IGenre>( ml, "*", std::move( req ),
                                      std::move( orderBy ) );
}

}
