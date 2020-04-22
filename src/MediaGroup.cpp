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
    , m_name( row.extract<decltype(m_name)>() )
    , m_nbVideo( row.extract<decltype(m_nbVideo)>() )
    , m_nbAudio( row.extract<decltype(m_nbAudio)>() )
    , m_nbUnknown( row.extract<decltype(m_nbUnknown)>() )
    , m_duration( row.extract<decltype(m_duration)>() )
    , m_creationDate( row.extract<decltype(m_creationDate)>() )
    , m_lastModificationDate( row.extract<decltype(m_lastModificationDate)>() )
    , m_userInteracted( row.extract<decltype(m_userInteracted)>() )
    , m_forcedSingleton( row.extract<decltype(m_forcedSingleton)>() )
{
    assert( row.hasRemainingColumns() == false );
}

MediaGroup::MediaGroup( MediaLibraryPtr ml, std::string name, bool userInitiated,
                        bool isForcedSingleton )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( std::move( name ) )
    , m_nbVideo( 0 )
    , m_nbAudio( 0 )
    , m_nbUnknown( 0 )
    , m_duration( 0 )
    , m_creationDate( time( nullptr ) )
    , m_lastModificationDate( m_creationDate )
    , m_userInteracted( userInitiated )
    , m_forcedSingleton( isForcedSingleton )
{
}

MediaGroup::MediaGroup( MediaLibraryPtr ml )
    : m_ml( ml )
    , m_id( 0 )
    , m_nbVideo( 0 )
    , m_nbAudio( 0 )
    , m_nbUnknown( 0 )
    , m_duration( 0 )
    , m_creationDate( time( nullptr ) )
    , m_lastModificationDate( m_creationDate )
    , m_userInteracted( true )
    , m_forcedSingleton( false )
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

int64_t MediaGroup::duration() const
{
    return m_duration;
}

time_t MediaGroup::creationDate() const
{
    return m_creationDate;
}

time_t MediaGroup::lastModificationDate() const
{
    return m_lastModificationDate;
}

bool MediaGroup::userInteracted() const
{
    return m_userInteracted;
}

bool MediaGroup::add( IMedia& media )
{
    return add( media, false );
}

bool MediaGroup::add( int64_t mediaId )
{
    return add( mediaId, false );
}

bool MediaGroup::add( IMedia& media, bool initForceSingleton )
{
    if ( add( media.id(), initForceSingleton ) == false )
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
    if ( media.duration() > 0 )
        m_duration += media.duration();
    auto& m = static_cast<Media&>( media );
    m.setMediaGroupId( m_id );
    return true;
}

bool MediaGroup::add( int64_t mediaId, bool initForceSingleton )
{
    std::unique_ptr<sqlite::Transaction> t;
    if ( m_forcedSingleton == true && initForceSingleton == false &&
         sqlite::Transaction::transactionInProgress() == false )
        t = m_ml->getConn()->newTransaction();
    if ( Media::setMediaGroup( m_ml, mediaId, m_id ) == false )
        return false;
    if ( m_forcedSingleton == true && initForceSingleton == false )
    {
        const std::string req = "UPDATE " + Table::Name +
                " SET forced_singleton = 0 WHERE id_group = ?";
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id ) == false )
            return false;
        m_forcedSingleton = false;
    }
    if ( t != nullptr )
        t->commit();
    m_lastModificationDate = time( nullptr );
    return Media::setMediaGroup( m_ml, mediaId, m_id );
}

bool MediaGroup::remove( IMedia& media )
{
    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = m_ml->getConn()->newTransaction();

    auto group = MediaGroup::create( m_ml, media.title(), false, true );
    if ( group == nullptr )
        return false;
    auto res = group->add( media, true );
    if ( res == false )
        return false;

    if ( t != nullptr )
        t->commit();

    auto& m = static_cast<Media&>( media );
    m.setMediaGroupId( group->id() );

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
    if ( media.duration() > 0 )
        m_duration -= media.duration();
    return true;
}

bool MediaGroup::remove( int64_t mediaId )
{
    auto media = Media::fetch( m_ml, mediaId );
    if ( media == nullptr )
        return false;
    return remove( *media );
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

bool MediaGroup::rename( std::string name )
{
    return rename( std::move( name ), true );
}

bool MediaGroup::rename( std::string name, bool userInitiated )
{
    if ( name.empty() == true )
        return false;
    if ( m_forcedSingleton == true )
        return false;
    if ( name == m_name )
        return true;
    /* No need to update the user_interacted column if it's already set to false */
    if ( userInitiated == false || m_userInteracted == true )
    {
        const std::string req = "UPDATE " + Table::Name +
                " SET name = ?, last_modification_date = strftime('%s')"
                " WHERE id_group = ?";
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
            return false;
    }
    else
    {
        const std::string req = "UPDATE " + Table::Name +
                " SET name = ?, last_modification_date = strftime('%s'),"
                " user_interacted = true WHERE id_group = ?";
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, name, m_id ) == false )
            return false;
        m_userInteracted = true;
    }
    m_lastModificationDate = time( nullptr );
    m_name = std::move( name );
    return true;
}

bool MediaGroup::isForcedSingleton() const
{
    return m_forcedSingleton;
}

bool MediaGroup::destroy()
{
    if ( m_forcedSingleton == true )
        return false;
    auto t = m_ml->getConn()->newTransaction();
    auto content = media( IMedia::Type::Unknown, nullptr )->all();
    for ( auto& m : content )
    {
        if ( remove( *m ) == false )
            return false;
    }
    // Let the empty group be removed by the DeleteEmptyGroups trigger
    t->commit();
    return true;
}

std::shared_ptr<MediaGroup> MediaGroup::create( MediaLibraryPtr ml,
                                                std::string name,
                                                bool userInitiated,
                                                bool isForcedSingleton )
{
    static const std::string req = "INSERT INTO " + Table::Name +
            "(name, user_interacted, forced_singleton, creation_date, last_modification_date) "
            "VALUES(?, ?, ?, ?, ?)";
    auto self = std::make_shared<MediaGroup>( ml, std::move( name ),
                                              userInitiated, isForcedSingleton );
    if ( insert( ml, self, req, self->name(), userInitiated,
                 isForcedSingleton, self->creationDate(),
                 self->lastModificationDate() ) == false )
        return nullptr;
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaGroupCreation( self );
    return self;
}

std::shared_ptr<MediaGroup> MediaGroup::create( MediaLibraryPtr ml,
                                                const std::vector<int64_t>& mediaIds )
{
    static const std::string req = "INSERT INTO " + Table::Name +
            "(user_interacted, forced_singleton, creation_date, last_modification_date) "
            "VALUES(?, ?, ?, ?)";
    auto self = std::make_shared<MediaGroup>( ml );
    if ( insert( ml, self, req, true, false, self->creationDate(),
                 self->lastModificationDate() ) == false )
        return nullptr;
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaGroupCreation( self );
    for ( const auto mId : mediaIds )
    {
        auto media = ml->media( mId );
        if ( media == nullptr )
        {
            /*
             * Hope it was a sporadic failure and attempt to link the media
             * anyway. The counters won't be up to date but the media will be linked.
             */
            self->add( mId );
        }
        else
            self->add( *media );
    }
    return self;
}

std::vector<std::shared_ptr<MediaGroup>>
MediaGroup::fetchMatching( MediaLibraryPtr ml, const std::string& prefix )
{
    if ( prefix.length() < AutomaticGroupPrefixSize )
        return {};
    static const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE forced_singleton = 0"
            " AND SUBSTR(name, 1, ?) = ? COLLATE NOCASE";
    return fetchAll<MediaGroup>( ml, req, prefix.length(), prefix );
}

Query<IMediaGroup> MediaGroup::listAll( MediaLibraryPtr ml,
                                        const QueryParameters* params )
{
    const std::string req = "FROM " + Table::Name + " mg";
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
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::DeleteEmptyGroups, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::RenameForcedSingleton, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateDurationOnMediaChange, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateDurationOnMediaDeletion, Settings::DbModelVersion ) );
}

void MediaGroup::createIndexes( sqlite::Connection* connection )
{
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::ForcedSingleton, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::Duration, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::CreationDate, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   index( Indexes::LastModificationDate, Settings::DbModelVersion ) );
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
    if ( dbModel == 24 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
            "parent_id INTEGER,"
            "name TEXT COLLATE NOCASE,"
            "nb_video UNSIGNED INTEGER DEFAULT 0,"
            "nb_audio UNSIGNED INTEGER DEFAULT 0,"
            "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
            "FOREIGN KEY(parent_id) REFERENCES " + Table::Name +
                "(id_group) ON DELETE CASCADE,"
            "UNIQUE(parent_id, name) ON CONFLICT FAIL"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT COLLATE NOCASE,"
        "nb_video UNSIGNED INTEGER DEFAULT 0,"
        "nb_audio UNSIGNED INTEGER DEFAULT 0,"
        "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
        "duration INTEGER DEFAULT 0,"
        "creation_date INTEGER NOT NULL,"
        "last_modification_date INTEGER NOT NULL,"
        "user_interacted BOOLEAN,"
        "forced_singleton BOOLEAN"
    ")";
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
                                " THEN 1 ELSE 0 END),"
                        " last_modification_date = strftime('%s')"
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
                                " THEN 1 ELSE 0 END),"
                        " last_modification_date = strftime('%s')"
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
                               " THEN 1 ELSE 0 END),"
                       " last_modification_date = strftime('%s')"
                   " WHERE id_group = old.group_id;"
                   " END";
        case Triggers::DeleteEmptyGroups:
            assert( dbModel >= 25 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF nb_video, nb_audio, nb_unknown"
                       " ON " + Table::Name +
                   " WHEN new.nb_video = 0 AND new.nb_audio = 0 AND new.nb_unknown = 0"
                   " BEGIN"
                   " DELETE FROM " + Table::Name + " WHERE id_group = new.id_group;"
                   " END";
        case Triggers::RenameForcedSingleton:
            assert( dbModel >= 25 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF title ON " + Media::Table::Name +
                   " WHEN new.group_id IS NOT NULL"
                   " BEGIN"
                       " UPDATE " + Table::Name + " SET name = new.title"
                           " WHERE id_group = new.group_id"
                           " AND forced_singleton != 0;"
                   " END";
        case Triggers::UpdateDurationOnMediaChange:
            assert( dbModel >= 25 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF duration, group_id ON " + Media::Table::Name +
                   " BEGIN"
                       " UPDATE " + Table::Name +
                           " SET duration = duration - max(old.duration, 0)"
                           " WHERE id_group = old.group_id;"
                       " UPDATE " + Table::Name +
                           " SET duration = duration + max(new.duration, 0)"
                           " WHERE id_group = new.group_id;"
                   " END";
        case Triggers::UpdateDurationOnMediaDeletion:
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER DELETE ON " + Media::Table::Name +
                   " WHEN old.group_id IS NOT NULL AND old.duration > 0"
                   " BEGIN"
                       " UPDATE " + Table::Name +
                           " SET duration = duration - old.duration"
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
        case Triggers::DeleteEmptyGroups:
            assert( dbModel >= 25 );
            return "media_group_delete_empty_group";
        case Triggers::RenameForcedSingleton:
            assert( dbModel >= 25 );
            return "media_group_rename_forced_singleton";
        case Triggers::UpdateDurationOnMediaChange:
            assert( dbModel >= 25 );
            return "media_group_update_duration_on_media_change";
        case Triggers::UpdateDurationOnMediaDeletion:
            assert( dbModel >= 25 );
            return "media_group_update_duration_on_media_deletion";
        default:
            assert( !"Invalid trigger" );
    }
    return "<invalid request>";
}

std::string MediaGroup::index( Indexes i, uint32_t dbModel )
{
    switch ( i )
    {
        case Indexes::ParentId:
            assert( dbModel == 24 );
            return "CREATE INDEX " + indexName( i, dbModel ) +
                   " ON " + Table::Name + "(parent_id)";
        case Indexes::ForcedSingleton:
            assert( dbModel >= 25 );
            return "CREATE INDEX " + indexName( i, dbModel ) +
                   " ON " + Table::Name + "(forced_singleton)";
        case Indexes::Duration:
            assert( dbModel >= 25 );
            return "CREATE INDEX " + indexName( i, dbModel ) +
                   " ON " + Table::Name + "(duration)";
        case Indexes::CreationDate:
            assert( dbModel >= 25 );
            return "CREATE INDEX " + indexName( i, dbModel ) +
                   " ON " + Table::Name + "(creation_date)";
        case Indexes::LastModificationDate:
            assert( dbModel >= 25 );
            return "CREATE INDEX " + indexName( i, dbModel ) +
                   " ON " + Table::Name + "(last_modification_date)";
    }
    return "<invalid request>";
}

std::string MediaGroup::indexName( Indexes i, uint32_t dbModel )
{
    switch ( i )
    {
        case Indexes::ParentId:
            assert( dbModel == 24 );
            return "media_group_parent_id_idx";
        case Indexes::ForcedSingleton:
            assert( dbModel >= 25 );
            return "media_group_forced_singleton";
        case Indexes::Duration:
            assert( dbModel >= 25 );
            return "media_group_duration";
        case Indexes::CreationDate:
            assert( dbModel >= 25 );
            return "media_group_creation_date";
        case Indexes::LastModificationDate:
            assert( dbModel >= 25 );
            return "media_group_last_modification_date";
    }
    return "<invalid request>";
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

    auto check = []( sqlite::Connection* dbConn, Triggers t ) {
        return sqlite::Tools::checkTriggerStatement( dbConn,
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    auto checkIndex = []( sqlite::Connection* dbConn, Indexes i ) {
        return sqlite::Tools::checkIndexStatement( dbConn,
                                    index( i, Settings::DbModelVersion ),
                                    indexName( i, Settings::DbModelVersion ) );
    };

    return check( ml->getConn(), Triggers::InsertFts ) &&
            check( ml->getConn(), Triggers::DeleteFts ) &&
            check( ml->getConn(), Triggers::IncrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            check( ml->getConn(), Triggers::DecrementNbMediaOnGroupChange ) &&
            checkIndex( ml->getConn(), Indexes::ForcedSingleton ) &&
            checkIndex( ml->getConn(), Indexes::Duration ) &&
            checkIndex( ml->getConn(), Indexes::CreationDate ) &&
            checkIndex( ml->getConn(), Indexes::LastModificationDate );
}

bool MediaGroup::assignToGroup( MediaLibraryPtr ml, Media& m )
{
    assert( m.groupId() == 0 );
    auto title = m.title();
    auto p = prefix( title );
    auto groups = MediaGroup::fetchMatching( ml, p );
    if ( groups.empty() == true )
    {
        if ( strncasecmp( title.c_str(), "the ", 4 ) == 0 )
            title = title.substr( 4 );
        auto group = create( ml, std::move( title ), false, false );
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
    if ( target->userInteracted() == false &&
         target->rename( longestPattern, false ) == false )
    {
        return false;
    }
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
        case SortingCriteria::Duration:
            req += "mg.duration";
            break;
        case SortingCriteria::InsertionDate:
            req += "mg.creation_date";
            break;
        case SortingCriteria::LastModificationDate:
            req += "mg.last_modification_date";
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

}
