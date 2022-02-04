/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs
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

#include "SubtitleTrack.h"

#include "Media.h"
#include "database/SqliteQuery.h"
#include "File.h"

namespace medialibrary
{

const std::string SubtitleTrack::Table::Name = "SubtitleTrack";
const std::string SubtitleTrack::Table::PrimaryKeyColumn = "id_track";
int64_t SubtitleTrack::* const SubtitleTrack::Table::PrimaryKey = &SubtitleTrack::m_id;

SubtitleTrack::SubtitleTrack( MediaLibraryPtr, sqlite::Row& row )
    : m_id( row.load<decltype(m_id)>( 0 ) )
    , m_codec( row.load<decltype(m_codec)>( 1 ) )
    , m_language( row.load<decltype(m_language)>( 2 ) )
    , m_description( row.load<decltype(m_description)>( 3 ) )
    , m_encoding( row.load<decltype(m_encoding)>( 4 ) )
    , m_attachedFileId( row.load<decltype(m_attachedFileId)>( 6 ) )
{
    assert( row.nbColumns() == 7 );
}

SubtitleTrack::SubtitleTrack( MediaLibraryPtr, std::string codec,
                              std::string language, std::string description,
                              std::string encoding, int64_t attachedFileId )
    : m_id( 0 )
    , m_codec( std::move( codec ) )
    , m_language( std::move( language ) )
    , m_description( std::move( description ) )
    , m_encoding( std::move( encoding ) )
    , m_attachedFileId( attachedFileId )
{
}

int64_t SubtitleTrack::id() const
{
    return m_id;
}

const std::string&SubtitleTrack::codec() const
{
    return m_codec;
}

const std::string&SubtitleTrack::language() const
{
    return m_language;
}

const std::string&SubtitleTrack::description() const
{
    return m_description;
}

const std::string&SubtitleTrack::encoding() const
{
    return m_encoding;
}

bool SubtitleTrack::isInAttachedFile() const
{
    return m_attachedFileId != 0;
}

void SubtitleTrack::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void SubtitleTrack::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::AttachedFileId, Settings::DbModelVersion ) );
}

std::string SubtitleTrack::schema( const std::string& tableName, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( tableName );

    assert( tableName == Table::Name );
    if ( dbModel < 27 )
    {
        return "CREATE TABLE " + Table::Name +
        "(" +
            Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
            "codec TEXT,"
            "language TEXT,"
            "description TEXT,"
            "encoding TEXT,"
            "media_id UNSIGNED INT,"
            "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
                "(id_media) ON DELETE CASCADE"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "(" +
        Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
        "codec TEXT,"
        "language TEXT,"
        "description TEXT,"
        "encoding TEXT,"
        "media_id UNSIGNED INT,"
        "attached_file_id UNSIGNED INT,"
        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
            "(id_media) ON DELETE CASCADE,"
        "FOREIGN KEY(attached_file_id) REFERENCES " + File::Table::Name +
            "(id_file) ON DELETE CASCADE,"
        "UNIQUE(media_id, attached_file_id) ON CONFLICT FAIL"
    ")";
}

std::string SubtitleTrack::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            return "CREATE INDEX " + indexName( index, dbModel ) +
                   " ON " + Table::Name + "(media_id)";
        case Indexes::AttachedFileId:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON "
                    + Table::Name + "(attached_file_id)";
    }
    return "<invalid request>";
}

std::string SubtitleTrack::indexName( Indexes index, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( index );
    UNUSED_IN_RELEASE( dbModel );

    switch ( index )
    {
        case Indexes::MediaId:
            return "subtitle_track_media_idx";
        case Indexes::AttachedFileId:
            assert( dbModel >= 34 );
            return "subtitle_track_attached_file_idx";
    }
    return "<invalid request>";
}

bool SubtitleTrack::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    auto checkIndex = [ml]( Indexes idx ) {
        return sqlite::Tools::checkIndexStatement( ml->getConn(),
             index( idx, Settings::DbModelVersion ),
             indexName( idx, Settings::DbModelVersion ) );
    };
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
           checkIndex( Indexes::MediaId ) &&
           checkIndex( Indexes::AttachedFileId );
}

std::shared_ptr<SubtitleTrack> SubtitleTrack::create( MediaLibraryPtr ml,
            std::string codec, std::string language, std::string description,
            std::string encoding, int64_t mediaId, int64_t attachedFileId )
{
    const std::string req = "INSERT INTO " + Table::Name + "(codec, language,"
            "description, encoding, media_id, attached_file_id) "
            "VALUES(?, ?, ?, ?, ?, ?)";
    auto track = std::make_shared<SubtitleTrack>( ml, std::move( codec ),
                    std::move( language ), std::move( description ),
                    std::move( encoding ), attachedFileId );
    if ( insert( ml, track, req, track->codec(), track->language(),
                 track->description(), track->encoding(), mediaId,
                 sqlite::ForeignKey{ attachedFileId } ) == false )
        return nullptr;
    return track;
}

bool SubtitleTrack::removeFromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                     bool internalTracksOnly )
{
    std::string req = "DELETE FROM " + Table::Name + " WHERE media_id = ?";
    if ( internalTracksOnly == true )
        req += " AND attached_file_id IS NULL";
    return sqlite::Tools::executeDelete( ml->getConn(), req, mediaId );
}

Query<ISubtitleTrack> SubtitleTrack::fromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                                bool internalTracksOnly )
{
    std::string req = "FROM " + Table::Name + " WHERE media_id = ?";
    if ( internalTracksOnly == true )
        req += " AND attached_file_id IS NULL";
    return make_query<SubtitleTrack, ISubtitleTrack>( ml, "*", req, "", mediaId );
}

}
