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
#include "AlbumTrack.h"
#include "Artist.h"
#include "Media.h"
#include "database/SqliteQuery.h"

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
{
    assert( row.hasRemainingColumns() == false );
}

Genre::Genre( MediaLibraryPtr ml, const std::string& name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( name )
    , m_nbTracks( 0 )
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

void Genre::updateCachedNbTracks( int increment )
{
    m_nbTracks += increment;
}

Query<IArtist> Genre::artists( const QueryParameters* params ) const
{
    std::string req = "FROM " + Artist::Table::Name + " a "
            "INNER JOIN " + AlbumTrack::Table::Name + " att ON att.artist_id = a.id_artist "
            "WHERE att.genre_id = ?";
    std::string groupAndOrderBy = "GROUP BY att.artist_id ORDER BY a.name";
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

Query<IMedia> Genre::tracks( bool withThumbnail, const QueryParameters* params ) const
{
    return AlbumTrack::fromGenre( m_ml, m_id, withThumbnail, params );
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
    const std::string vtableInsertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_genre_fts"
            " AFTER INSERT ON " + Genre::Table::Name +
            " BEGIN"
            " INSERT INTO " + Genre::FtsTable::Name + "(rowid,name) VALUES(new.id_genre, new.name);"
            " END";
    const std::string vtableDeleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_genre_fts"
            " BEFORE DELETE ON " + Genre::Table::Name +
            " BEGIN"
            " DELETE FROM " + Genre::FtsTable::Name + " WHERE rowid = old.id_genre;"
            " END";
    const std::string onTrackCreated = "CREATE TRIGGER IF NOT EXISTS update_genre_on_new_track"
            " AFTER INSERT ON " + AlbumTrack::Table::Name +
            " WHEN new.genre_id IS NOT NULL"
            " BEGIN"
            " UPDATE " + Genre::Table::Name + " SET nb_tracks = nb_tracks + 1 WHERE id_genre = new.genre_id;"
            " END";
    const std::string onTrackDeleted = "CREATE TRIGGER IF NOT EXISTS update_genre_on_track_deleted"
            " AFTER DELETE ON " + AlbumTrack::Table::Name +
            " WHEN old.genre_id IS NOT NULL"
            " BEGIN"
            " UPDATE " + Genre::Table::Name + " SET nb_tracks = nb_tracks - 1 WHERE id_genre = old.genre_id;"
            " DELETE FROM " + Genre::Table::Name + " WHERE nb_tracks = 0;"
            " END";

    sqlite::Tools::executeRequest( dbConn, vtableInsertTrigger );
    sqlite::Tools::executeRequest( dbConn, vtableDeleteTrigger );
    sqlite::Tools::executeRequest( dbConn, onTrackCreated );
    sqlite::Tools::executeRequest( dbConn, onTrackDeleted );
}

std::string Genre::schema( const std::string& tableName, uint32_t )
{
    if ( tableName == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
               " USING FTS3(name)";
    }
    assert( tableName == Table::Name );
    return "CREATE TABLE " + Table::Name +
    "("
        "id_genre INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
        "nb_tracks INTEGER NOT NULL DEFAULT 0"
    ")";
}

std::shared_ptr<Genre> Genre::create( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "INSERT INTO " + Genre::Table::Name + "(name)"
            "VALUES(?)";
    auto self = std::make_shared<Genre>( ml, name );
    if ( insert( ml, self, req, name ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Genre> Genre::fromName( MediaLibraryPtr ml, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + Genre::Table::Name + " WHERE name = ?";
    return fetch( ml, req, name );
}

Query<IGenre> Genre::search( MediaLibraryPtr ml, const std::string& name,
                             const QueryParameters* params )
{
    std::string req = "FROM " + Genre::Table::Name + " WHERE id_genre IN "
            "(SELECT rowid FROM " + Genre::FtsTable::Name + " "
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
    std::string req = "FROM " + Genre::Table::Name;
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
