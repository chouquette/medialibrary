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

#include "MediaGroup.h"
#include "Media.h"
#include "database/SqliteQuery.h"
#include "medialibrary/IMediaLibrary.h"
#include "utils/ModificationsNotifier.h"

#include <cstring>

namespace medialibrary
{

const std::string MediaGroup::Table::Name = "MediaGroup";
const std::string MediaGroup::Table::PrimaryKeyColumn = "id_group";
int64_t MediaGroup::*const MediaGroup::Table::PrimaryKey = &MediaGroup::m_id;

const std::string MediaGroup::FtsTable::Name = "MediaGroupFts";

MediaGroup::MediaGroup( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_parentId( row.extract<decltype(m_parentId)>() )
    , m_name( row.extract<decltype(m_name)>() )
    , m_nbVideo( row.extract<decltype(m_nbVideo)>() )
    , m_nbAudio( row.extract<decltype(m_nbAudio)>() )
    , m_nbUnknown( row.extract<decltype(m_nbUnknown)>() )
{
    assert( row.hasRemainingColumns() == false );
}

MediaGroup::MediaGroup(MediaLibraryPtr ml, int64_t parentId, std::string name )
    : m_ml( ml )
    , m_id( 0 )
    , m_parentId( parentId )
    , m_name( std::move( name ) )
    , m_nbVideo( 0 )
    , m_nbAudio( 0 )
    , m_nbUnknown( 0 )
{
}

int64_t MediaGroup::id() const
{
    return m_id;
}

const std::string& MediaGroup::name() const
{
    return m_name;
}

uint32_t MediaGroup::nbMedia() const
{
    return m_nbVideo + m_nbAudio + m_nbUnknown;
}

uint32_t MediaGroup::nbVideo() const
{
    return m_nbVideo;
}

uint32_t MediaGroup::nbAudio() const
{
    return m_nbAudio;
}

uint32_t MediaGroup::nbUnknown() const
{
    return m_nbUnknown;
}

bool MediaGroup::add( IMedia& media )
{
    if ( media.addToGroup( m_id ) == false )
        return false;
    switch ( media.type() )
    {
        case IMedia::Type::Audio:
            ++m_nbAudio;
            break;
        case IMedia::Type::Video:
            ++m_nbVideo;
            break;
        case IMedia::Type::Unknown:
            ++m_nbUnknown;
            break;
    }
    return true;
}

bool MediaGroup::add( int64_t mediaId )
{
    return Media::setMediaGroup( m_ml, mediaId, m_id );
}

bool MediaGroup::remove( IMedia& media )
{
    if ( media.removeFromGroup() == false )
        return false;
    switch ( media.type() )
    {
        case IMedia::Type::Audio:
            --m_nbAudio;
            break;
        case IMedia::Type::Video:
            --m_nbVideo;
            break;
        case IMedia::Type::Unknown:
            --m_nbUnknown;
            break;
    }
    return true;
}

bool MediaGroup::remove( int64_t mediaId )
{
    return Media::setMediaGroup( m_ml, mediaId, 0 );
}

MediaGroupPtr MediaGroup::createSubgroup( const std::string& name )
{
    return create( m_ml, m_id, name );
}

Query<IMediaGroup> MediaGroup::subgroups( const QueryParameters* params ) const
{
    const std::string req = "FROM " + Table::Name + " mg WHERE parent_id = ?";
    return make_query<MediaGroup, IMediaGroup>( m_ml, "mg.*", req,
                                                orderBy( params ), m_id );
}

bool MediaGroup::isSubgroup() const
{
    return m_parentId != 0;
}

MediaGroupPtr MediaGroup::parent() const
{
    return fetch( m_ml, m_parentId );
}

Query<IMedia> MediaGroup::media( IMedia::Type mediaType, const QueryParameters* params)
{
    return Media::fromMediaGroup( m_ml, m_id, mediaType, params );
}

Query<IMedia> MediaGroup::searchMedia(const std::string& pattern, IMedia::Type mediaType,
                                       const QueryParameters* params )
{
    return Media::searchFromMediaGroup( m_ml, m_id, mediaType, pattern, params );
}

std::string MediaGroup::path() const
{
    auto res = name();
    if ( isSubgroup() == false )
        return res;
    auto p = parent();
    while ( p )
    {
        res = p->name() + '/' + res;
        if ( p->isSubgroup() == false )
            break;
        p = p->parent();
    }
    return res;
}

bool MediaGroup::rename( std::string name )
{
    if ( name.empty() == true )
        return false;
    if ( name == m_name )
        return true;
    const std::string req = "UPDATE " + Table::Name +
            " SET name = ? WHERE id_group = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
        return false;
    m_name = std::move( name );
    return true;
}

bool MediaGroup::destroy()
{
    return DatabaseHelpers<MediaGroup>::destroy( m_ml, m_id );
}

std::shared_ptr<MediaGroup> MediaGroup::create( MediaLibraryPtr ml,
                                                int64_t parentId, std::string name )
{
    static const std::string req = "INSERT INTO " + Table::Name +
            "(parent_id, name) VALUES(?, ?)";
    auto self = std::make_shared<MediaGroup>( ml, parentId, std::move( name ) );
    if ( insert( ml, self, req, sqlite::ForeignKey{ parentId }, self->name() ) == false )
        return nullptr;
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaGroupCreation( self );
    return self;
}

std::shared_ptr<MediaGroup> MediaGroup::fetchByName( MediaLibraryPtr ml,
                                             const std::string& name )
{
    static const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE name = ?";
    return fetch( ml, req, name );
}

std::vector<std::shared_ptr<MediaGroup>>
MediaGroup::fetchMatching( MediaLibraryPtr ml, const std::string& prefix )
{
    static const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE SUBSTR(name, 1, ?) = ? COLLATE NOCASE";
    return fetchAll<MediaGroup>( ml, req, prefix.length(), prefix );
}

Query<IMediaGroup> MediaGroup::listAll( MediaLibraryPtr ml,
                                        const QueryParameters* params )
{
    const std::string req = "FROM " + Table::Name + " mg WHERE parent_id IS NULL";
    return make_query<MediaGroup, IMediaGroup>( ml, "mg.*", req, orderBy( params ) );
}

Query<IMediaGroup> MediaGroup::search( MediaLibraryPtr ml, const std::string& pattern,
                                       const QueryParameters* params )
{
    const std::string req = "FROM " + Table::Name + " mg"
            " WHERE id_group IN (SELECT rowid FROM " + FtsTable::Name +
                " WHERE " + FtsTable::Name + " MATCH ?)";
    return make_query<MediaGroup, IMediaGroup>( ml, "mg.*", req, orderBy( params ),
                                                sqlite::Tools::sanitizePattern( pattern ) );
}

void MediaGroup::createTable(sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( FtsTable::Name, Settings::DbModelVersion ) );
}

void MediaGroup::createTriggers( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::InsertFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DeleteFts, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::IncrementNbMediaOnGroupChange, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DecrementNbMediaOnGroupChange, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DecrementNbMediaOnDeletion, Settings::DbModelVersion ) );
}

void MediaGroup::createIndexes( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ParentId, Settings::DbModelVersion ) );
}

std::string MediaGroup::schema( const std::string& name, uint32_t dbModel )
{
    assert( dbModel >= 24 );
    if ( name == FtsTable::Name )
    {
        return "CREATE VIRTUAL TABLE " + FtsTable::Name +
                   " USING FTS3(name)";
    }
    assert( name == Table::Name );
    auto req = "CREATE TABLE " + Table::Name +
    "("
        "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
        "parent_id INTEGER,"
        "name TEXT COLLATE NOCASE,"
        "nb_video UNSIGNED INTEGER DEFAULT 0,"
        "nb_audio UNSIGNED INTEGER DEFAULT 0,"
        "nb_unknown UNSIGNED INTEGER DEFAULT 0,"

        "FOREIGN KEY(parent_id) REFERENCES " + Table::Name +
            "(id_group) ON DELETE CASCADE";
    if ( dbModel < 25 )
    {
        req += ",UNIQUE(parent_id, name) ON CONFLICT FAIL";
    }
    req += ")";
    return req;
}

std::string MediaGroup::trigger( MediaGroup::Triggers t, uint32_t dbModel )
{
    assert( dbModel >= 24 );
    switch ( t )
    {
        case Triggers::InsertFts:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER INSERT ON " + Table::Name +
                    " BEGIN"
                    " INSERT INTO " + FtsTable::Name + "(rowid, name)"
                        " VALUES(new.rowid, new.name);"
                    " END";
        case Triggers::DeleteFts:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER DELETE ON " + Table::Name +
                   " BEGIN"
                   " DELETE FROM " + FtsTable::Name +
                       " WHERE rowid = old.id_group;"
                   " END";
        case Triggers::IncrementNbMediaOnGroupChange:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                    " WHEN new.group_id IS NOT NULL AND"
                        " (old.type != new.type OR IFNULL(old.group_id, 0) != new.group_id)"
                    " BEGIN"
                    " UPDATE " + Table::Name + " SET"
                        " nb_video = nb_video + "
                            "(CASE new.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Video ) ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_audio = nb_audio + "
                            "(CASE new.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Audio ) ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_unknown = nb_unknown + "
                            "(CASE new.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Unknown ) ) +
                                " THEN 1 ELSE 0 END)"
                    " WHERE id_group = new.group_id;"
                    " END";
        case Triggers::DecrementNbMediaOnGroupChange:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                    " WHEN old.group_id IS NOT NULL AND"
                        "(old.type != new.type OR old.group_id != IFNULL(new.group_id, 0))"
                    " BEGIN"
                    " UPDATE " + Table::Name + " SET"
                        " nb_video = nb_video - "
                            "(CASE old.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Video ) ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_audio = nb_audio - "
                            "(CASE old.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Audio ) ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_unknown = nb_unknown - "
                            "(CASE old.type WHEN " +
                                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                    IMedia::Type::Unknown ) ) +
                                " THEN 1 ELSE 0 END)"
                    " WHERE id_group = old.group_id;"
                    " END";
        case Triggers::DecrementNbMediaOnDeletion:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER DELETE ON " + Media::Table::Name +
                   " WHEN old.group_id IS NOT NULL"
                   " BEGIN"
                   " UPDATE " + Table::Name + " SET"
                       " nb_video = nb_video - "
                           "(CASE old.type WHEN " +
                               std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                   IMedia::Type::Video ) ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_audio = nb_audio - "
                           "(CASE old.type WHEN " +
                               std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                   IMedia::Type::Audio ) ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_unknown = nb_unknown - "
                           "(CASE old.type WHEN " +
                               std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                                   IMedia::Type::Unknown ) ) +
                               " THEN 1 ELSE 0 END)"
                   " WHERE id_group = old.group_id;"
                   " END";
        default:
            assert( !"Invalid trigger" );
    }
    return "<invalid request>";
}

std::string MediaGroup::triggerName(MediaGroup::Triggers t, uint32_t dbModel)
{
    assert( dbModel >= 24 );
    switch ( t )
    {
        case Triggers::InsertFts:
            return "media_group_insert_fts";
        case Triggers::DeleteFts:
            return "media_group_delete_fts";
        case Triggers::IncrementNbMediaOnGroupChange:
            return "media_group_increment_nb_media";
        case Triggers::DecrementNbMediaOnGroupChange:
            return "media_group_decrement_nb_media";
        case Triggers::DecrementNbMediaOnDeletion:
            return "media_group_decrement_nb_media_on_deletion";
        default:
            assert( !"Invalid trigger" );
    }
    return "<invalid request>";
}

std::string MediaGroup::index( Indexes i, uint32_t dbModel )
{
    assert( i == Indexes::ParentId );
    assert( dbModel >= 24 );
    return "CREATE INDEX " + indexName( i, dbModel ) +
           " ON " + Table::Name + "(parent_id)";
}

std::string MediaGroup::indexName( Indexes i, uint32_t dbModel )
{
    assert( i == Indexes::ParentId );
    assert( dbModel >= 24 );
    return "media_group_parent_id_idx";
}

bool MediaGroup::checkDbModel( MediaLibraryPtr ml )
{
    if ( sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false )
        return false;

    if ( sqlite::Tools::checkIndexStatement( ml->getConn(),
            index( Indexes::ParentId, Settings::DbModelVersion ),
            indexName( Indexes::ParentId, Settings::DbModelVersion ) ) == false )
        return false;

    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    return check( ml->getConn(), Triggers::InsertFts ) &&
            check( ml->getConn(), Triggers::DeleteFts ) &&
            check( ml->getConn(), Triggers::IncrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange );
}

bool MediaGroup::assignToGroup( MediaLibraryPtr ml, Media& m )
{
    assert( m.groupId() == 0 );
    assert( m.hasBeenGrouped() == false );
    auto title = m.title();
    auto p = prefix( title );
    auto groups = MediaGroup::fetchMatching( ml, p );
    if ( groups.empty() == true )
    {
        if ( strncasecmp( title.c_str(), "the ", 4 ) == 0 )
            title = title.substr( 4 );
        auto group = create( ml, 0, std::move( title ) );
        if ( group == nullptr )
            return false;
        return group->add( m );
    }
    std::string longestPattern;
    std::shared_ptr<MediaGroup> target;
    for ( const auto& group : groups )
    {
        auto match = commonPattern( group->name(), title );
        assert( match.empty() == false );
        if ( match.length() > longestPattern.length() )
        {
            longestPattern = match;
            target = group;
        }
    }
    if ( target == nullptr )
    {
        assert( !"There should have been a match" );
        return false;
    }
    if ( target->rename( longestPattern ) == false )
        return false;
    return target->add( m );
}

std::string MediaGroup::prefix( const std::string& title )
{
    auto offset = 0u;
    if ( strncasecmp( title.c_str(), "the ", 4 ) == 0 )
        offset = 4;
    return title.substr( offset, AutomaticGroupPrefixSize + offset );
}

std::string MediaGroup::commonPattern( const std::string& groupName,
                                       const std::string& newTitle )
{
    auto groupIdx = 0u;
    auto groupBegin = 0u;
    auto titleIdx = 0u;
    if ( strncasecmp( groupName.c_str(), "the ", 4 ) == 0 )
        groupBegin = groupIdx = 4u;
    if ( strncasecmp( newTitle.c_str(), "the ", 4 ) == 0 )
        titleIdx = 4;
    while ( groupIdx < groupName.size() && titleIdx < newTitle.size() &&
            tolower( groupName[groupIdx] ) == tolower( newTitle[titleIdx] ) )
    {
        ++groupIdx;
        ++titleIdx;
    }
    if ( groupIdx - groupBegin < AutomaticGroupPrefixSize )
        return {};
    return groupName.substr( groupBegin, groupIdx - groupBegin );
}

std::string MediaGroup::orderBy(const QueryParameters* params)
{
    std::string req = "ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Alpha;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
        case SortingCriteria::NbAudio:
            req += "mg.nb_audio";
            break;
        case SortingCriteria::NbVideo:
            req += "mg.nb_video";
            break;
        case SortingCriteria::NbMedia:
            req += "mg.nb_audio + mg.nb_video + mg.nb_unknown";
            break;
        default:
            LOG_WARN( "Unsupported sorting criteria for media groups: ",
                      static_cast<std::underlying_type_t<SortingCriteria>>( sort ),
                      ". Falling back to default (Alpha)" );
            /* fall-through */
        case SortingCriteria::Alpha:
            req += "mg.name";
            break;
    }
    if ( desc == true )
        req += " DESC";
    return req;
}

bool MediaGroup::exists(MediaLibraryPtr ml, const std::string& name)
{
    sqlite::Statement stmt{ ml->getConn()->handle(), "SELECT EXISTS("
        "SELECT id_group FROM " + Table::Name +
        " WHERE parent_id IS NULL AND name = ?)"
    };
    stmt.execute( name );
    auto row = stmt.row();
    auto res = row.extract<bool>();
    assert( stmt.row() == nullptr );
    return res;
}

}
