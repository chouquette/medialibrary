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
#include "utils/Strings.h"
#include "utils/Enums.h"

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
    , m_nbSeen( row.extract<decltype(m_nbSeen)>() )
    , m_nbExternal( row.extract<decltype(m_nbExternal)>() )
    , m_nbPresentVideo( row.extract<decltype(m_nbPresentVideo)>() )
    , m_nbPresentAudio( row.extract<decltype(m_nbPresentAudio)>() )
    , m_nbPresentUnknown( row.extract<decltype(m_nbPresentUnknown)>() )
    , m_nbPresentSeen( row.extract<decltype(m_nbPresentSeen)>() )
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
    , m_nbSeen( 0 )
    , m_nbExternal( 0 )
    , m_nbPresentVideo( 0 )
    , m_nbPresentAudio( 0 )
    , m_nbPresentUnknown( 0 )
    , m_nbPresentSeen( 0 )
    , m_duration( 0 )
    , m_creationDate( time( nullptr ) )
    , m_lastModificationDate( m_creationDate )
    , m_userInteracted( userInitiated )
    , m_forcedSingleton( isForcedSingleton )
{
}

MediaGroup::MediaGroup( MediaLibraryPtr ml , std::string name )
    : m_ml( ml )
    , m_id( 0 )
    , m_name( std::move( name ) )
    , m_nbVideo( 0 )
    , m_nbAudio( 0 )
    , m_nbUnknown( 0 )
    , m_nbSeen( 0 )
    , m_nbExternal( 0 )
    , m_nbPresentVideo( 0 )
    , m_nbPresentAudio( 0 )
    , m_nbPresentUnknown( 0 )
    , m_nbPresentSeen( 0 )
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

uint32_t MediaGroup::nbPresentMedia() const
{
    return m_nbPresentVideo + m_nbPresentAudio + m_nbPresentUnknown;
}

uint32_t MediaGroup::nbTotalMedia() const
{
    return m_nbVideo + m_nbAudio + m_nbUnknown;
}

uint32_t MediaGroup::nbPresentVideo() const
{
    return m_nbPresentVideo;
}

uint32_t MediaGroup::nbPresentAudio() const
{
    return m_nbPresentAudio;
}

uint32_t MediaGroup::nbPresentUnknown() const
{
    return m_nbPresentUnknown;
}

uint32_t MediaGroup::nbPresentSeen() const
{
    return m_nbPresentSeen;
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

uint32_t MediaGroup::nbSeen() const
{
    return m_nbSeen;
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
    if ( media.isDiscoveredMedia() == true )
    {
        switch ( media.type() )
        {
            case IMedia::Type::Audio:
                ++m_nbAudio;
                ++m_nbPresentAudio;
                break;
            case IMedia::Type::Video:
                ++m_nbVideo;
                ++m_nbPresentVideo;
                break;
            case IMedia::Type::Unknown:
                ++m_nbUnknown;
                ++m_nbPresentUnknown;
                break;
        }
    }
    else
        ++m_nbExternal;
    if ( media.duration() > 0 )
        m_duration += media.duration();
    if ( media.playCount() > 0 )
    {
        ++m_nbSeen;
        ++m_nbPresentSeen;
    }
    auto& m = static_cast<Media&>( media );
    m.setMediaGroupId( m_id );
    return true;
}

bool MediaGroup::add( int64_t mediaId, bool initForceSingleton )
{
    std::unique_ptr<sqlite::Transaction> t;
    if ( m_forcedSingleton == true && initForceSingleton == false )
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
    auto t = m_ml->getConn()->newTransaction();

    auto group = MediaGroup::create( m_ml, media.title(), false, true );
    if ( group == nullptr )
        return false;
    auto res = group->add( media, true );
    if ( res == false )
        return false;

    t->commit();

    auto& m = static_cast<Media&>( media );
    m.setMediaGroupId( group->id() );

    if ( media.isDiscoveredMedia() == true )
    {
        switch ( media.type() )
        {
            case IMedia::Type::Audio:
                --m_nbPresentAudio;
                --m_nbAudio;
                break;
            case IMedia::Type::Video:
                --m_nbPresentVideo;
                --m_nbVideo;
                break;
            case IMedia::Type::Unknown:
                --m_nbPresentUnknown;
                --m_nbUnknown;
                break;
        }
    }
    else
        --m_nbExternal;
    if ( media.duration() > 0 )
        m_duration -= media.duration();
    if ( media.playCount() > 0 )
    {
        --m_nbSeen;
        --m_nbPresentSeen;
    }
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

uint32_t MediaGroup::nbExternalMedia() const
{
    return m_nbExternal;
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
    std::vector<MediaPtr> media;
    std::string name;
    for ( const auto mId : mediaIds )
    {
        auto m = ml->media( mId );
        if ( m == nullptr )
            continue;
        if ( media.empty() == true )
        {
            /*
             * Only assign the media title for the first media. If at a later
             * point there is no match, we will empty 'name', and we'd end up
             * reseting it to an arbitrary media title if we'd only check if
             * 'name' was empty before assigning it
             */
            assert( name.empty() == true );
            name = m->title();
        }
        else
            name = commonPattern( name, m->title() );
        media.push_back( std::move( m ) );
    }
    if ( media.empty() == true )
        return nullptr;
    static const std::string req = "INSERT INTO " + Table::Name +
            "(name, user_interacted, forced_singleton, creation_date, last_modification_date) "
            "VALUES(?, ?, ?, ?, ?)";
    auto self = std::make_shared<MediaGroup>( ml, std::move( name ) );
    if ( insert( ml, self, req, self->name(), true, false, self->creationDate(),
                 self->lastModificationDate() ) == false )
        return nullptr;
    auto notifier = ml->getNotifier();
    if ( notifier != nullptr )
        notifier->notifyMediaGroupCreation( self );
    for ( const auto& m : media )
    {
        self->add( *m );
    }
    return self;
}

std::vector<std::shared_ptr<MediaGroup>>
MediaGroup::fetchMatching( MediaLibraryPtr ml, const std::string& prefix )
{
    if ( prefix.length() < AutomaticGroupPrefixSize )
        return {};
    auto nbChar = utils::str::utf8::nbChars( prefix );
    static const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE forced_singleton = 0"
            " AND SUBSTR(name, 1, ?) = ? COLLATE NOCASE";
    return fetchAll<MediaGroup>( ml, req, nbChar, prefix );
}

Query<IMediaGroup> MediaGroup::listAll( MediaLibraryPtr ml, IMedia::Type mediaType,
                                        const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " mg ";
    switch ( mediaType )
    {
        case IMedia::Type::Unknown:
        {
            if ( params == nullptr || params->includeMissing == false )
                req += "WHERE nb_present_video > 0 OR nb_present_audio > 0 OR nb_present_unknown > 0";
            else
                req += "WHERE nb_video > 0 OR nb_audio > 0 OR nb_unknown > 0";
            break;
        }
        case IMedia::Type::Audio:
        {
            if ( params == nullptr || params->includeMissing == false )
                req += "WHERE nb_present_audio > 0";
            else
                req += "WHERE nb_audio > 0";
            break;
        }
        case IMedia::Type::Video:
        {
            if ( params == nullptr || params->includeMissing == false )
                req += "WHERE nb_present_video > 0";
            else
                req += "WHERE nb_video > 0";
            break;
        }
    }
    return make_query<MediaGroup, IMediaGroup>( ml, "mg.*", req, orderBy( params ) ).build();
}

Query<IMediaGroup> MediaGroup::search( MediaLibraryPtr ml, const std::string& pattern,
                                       const QueryParameters* params )
{
    std::string req = "FROM " + Table::Name + " mg"
            " WHERE id_group IN (SELECT rowid FROM " + FtsTable::Name +
                " WHERE " + FtsTable::Name + " MATCH ?)";
    if ( params == nullptr || params->includeMissing == false )
        req += " AND nb_present_video > 0 OR nb_present_audio > 0 OR nb_present_unknown > 0";
    else
        req += " AND nb_video > 0 OR nb_audio > 0 OR nb_unknown > 0";
    return make_query<MediaGroup, IMediaGroup>( ml, "mg.*", req, orderBy( params ),
                                                sqlite::Tools::sanitizePattern( pattern ) ).build();
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
                                   trigger( Triggers::UpdateNbMediaPerType, Settings::DbModelVersion ) );
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
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateMediaCountOnPresenceChange, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( connection,
                                   trigger( Triggers::UpdateNbMediaOnImportTypeChange, Settings::DbModelVersion ) );
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
    if ( dbModel == 25 )
    {
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
    if ( dbModel < 30 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE,"
            // Nb media per type, accounting for their presence.
            "nb_video UNSIGNED INTEGER DEFAULT 0,"
            "nb_audio UNSIGNED INTEGER DEFAULT 0,"
            "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
            // Total number of media, regardless of presence
            "nb_media UNSIGNED INTEGER DEFAULT 0,"
            "duration INTEGER DEFAULT 0,"
            "creation_date INTEGER NOT NULL,"
            "last_modification_date INTEGER NOT NULL,"
            "user_interacted BOOLEAN,"
            "forced_singleton BOOLEAN"
        ")";
    }
    if ( dbModel == 30 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE,"
            // Total number of media, regardless of presence
            "nb_video UNSIGNED INTEGER DEFAULT 0,"
            "nb_audio UNSIGNED INTEGER DEFAULT 0,"
            "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
            // Nb media per type, accounting for their presence.
            "nb_present_video UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_video <= nb_video),"
            "nb_present_audio UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_audio <= nb_audio),"
            "nb_present_unknown UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_unknown <= nb_unknown),"
            "duration INTEGER DEFAULT 0,"
            "creation_date INTEGER NOT NULL,"
            "last_modification_date INTEGER NOT NULL,"
            "user_interacted BOOLEAN,"
            "forced_singleton BOOLEAN"
        ")";
    }
    if ( dbModel < 33 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT COLLATE NOCASE,"
            // Total number of media, regardless of presence
            "nb_video UNSIGNED INTEGER DEFAULT 0,"
            "nb_audio UNSIGNED INTEGER DEFAULT 0,"
            "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
            // The number of media that were assigned to this group
            // before they were converted to external media
            "nb_external UNSIGNED INTEGER DEFAULT 0,"
            // Nb media per type, accounting for their presence.
            "nb_present_video UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_video <= nb_video),"
            "nb_present_audio UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_audio <= nb_audio),"
            "nb_present_unknown UNSIGNED INTEGER DEFAULT 0 "
                "CHECK(nb_present_unknown <= nb_unknown),"
            "duration INTEGER DEFAULT 0,"
            "creation_date INTEGER NOT NULL,"
            "last_modification_date INTEGER NOT NULL,"
            "user_interacted BOOLEAN,"
            "forced_singleton BOOLEAN"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT COLLATE NOCASE,"
        // Total number of media, regardless of presence
        "nb_video UNSIGNED INTEGER DEFAULT 0,"
        "nb_audio UNSIGNED INTEGER DEFAULT 0,"
        "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
        // Number of seen media, regardless of presence
        "nb_seen UNSIGNED INTEGER DEFAULT 0,"
        // The number of media that were assigned to this group
        // before they were converted to external media
        "nb_external UNSIGNED INTEGER DEFAULT 0,"
        // Nb media per type, accounting for their presence.
        "nb_present_video UNSIGNED INTEGER DEFAULT 0 "
            "CHECK(nb_present_video <= nb_video),"
        "nb_present_audio UNSIGNED INTEGER DEFAULT 0 "
            "CHECK(nb_present_audio <= nb_audio),"
        "nb_present_unknown UNSIGNED INTEGER DEFAULT 0 "
            "CHECK(nb_present_unknown <= nb_unknown),"
        // Number of seen media, acccounting for their presence
        "nb_present_seen UNSIGNED INTEGER DEFAULT 0 "
            "CHECK(nb_present_seen <= nb_seen),"
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
        {
            assert( dbModel < 26 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                    " WHEN new.group_id IS NOT NULL AND"
                        " (old.type != new.type OR IFNULL(old.group_id, 0) != new.group_id)"
                    " BEGIN"
                    " UPDATE " + Table::Name + " SET"
                        " nb_video = nb_video + "
                            "(CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Video ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_audio = nb_audio + "
                            "(CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Audio ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_unknown = nb_unknown + "
                            "(CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Unknown ) +
                                " THEN 1 ELSE 0 END),"
                        " last_modification_date = strftime('%s')"
                    " WHERE id_group = new.group_id;"
                    " END";
        }
        case Triggers::DecrementNbMediaOnGroupChange:
        {
            assert( dbModel < 26 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                    " WHEN old.group_id IS NOT NULL AND"
                        "(old.type != new.type OR old.group_id != IFNULL(new.group_id, 0))"
                    " BEGIN"
                    " UPDATE " + Table::Name + " SET"
                        " nb_video = nb_video - "
                            "(CASE old.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Video ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_audio = nb_audio - "
                            "(CASE old.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Audio ) +
                                " THEN 1 ELSE 0 END),"
                        " nb_unknown = nb_unknown - "
                            "(CASE old.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Unknown ) +
                                " THEN 1 ELSE 0 END),"
                        " last_modification_date = strftime('%s')"
                    " WHERE id_group = old.group_id;"
                    " END";
        }
        case Triggers::UpdateNbMediaPerType:
        {
            assert( dbModel >= 26 );
            if ( dbModel < 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                        " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                            " WHEN (IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) OR"
                            " old.type != new.type)"
                            " AND new.is_present != 0"
                        " BEGIN"
                        // Handle increment
                        " UPDATE " + Table::Name + " SET"
                            " nb_video = nb_video + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_audio = nb_audio + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_unknown = nb_unknown + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                    " THEN 1 ELSE 0 END),"
                            " last_modification_date = strftime('%s')"
                        " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                        // Handle decrement
                        " UPDATE " + Table::Name + " SET"
                            " nb_video = nb_video - "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_audio = nb_audio - "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_unknown = nb_unknown - "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                    " THEN 1 ELSE 0 END),"
                            " last_modification_date = strftime('%s')"
                        " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                        " END";
            }
            if ( dbModel == 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                        " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                            " WHEN (IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) OR"
                            " old.type != new.type)"
                        " BEGIN"
                        // Handle increment
                        " UPDATE " + Table::Name + " SET"
                            " nb_video = nb_video + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_present_video = nb_present_video + "
                                "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE new.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Video ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " nb_audio = nb_audio + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_present_audio = nb_present_audio + "
                                "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE new.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Audio ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " nb_unknown = nb_unknown + "
                                "(CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_present_unknown = nb_present_unknown + "
                                "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE new.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Unknown ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " last_modification_date = strftime('%s')"
                        " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                        // Handle decrement
                        " UPDATE " + Table::Name + " SET"
                            " nb_present_video = nb_present_video - "
                                "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE old.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Video ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " nb_video = nb_video - "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_present_audio = nb_present_audio - "
                                "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE old.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Audio ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " nb_audio = nb_audio + "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                    " THEN 1 ELSE 0 END),"
                            " nb_present_unknown = nb_present_unknown - "
                                "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                    "(CASE old.type WHEN " +
                                        utils::enum_to_string( IMedia::Type::Unknown ) +
                                        " THEN 1 ELSE 0 END) END),"
                            " nb_unknown = nb_unknown - "
                                "(CASE old.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                    " THEN 1 ELSE 0 END),"
                            " last_modification_date = strftime('%s')"
                        " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                        " END";
            }
            if ( dbModel < 33 )
            {
            /*
             * Changes since V30:
             * - We now only monitor internal media
             * - Fix invalid decrement of nb_audio
             */
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF type, group_id ON " + Media::Table::Name +
                           " WHEN (IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) OR"
                           " old.type != new.type) AND"
                           " new.import_type = " +
                                utils::enum_to_string( Media::ImportType::Internal ) +
                       " BEGIN"
                       // Handle increment
                       " UPDATE " + Table::Name + " SET"
                           " nb_video = nb_video + "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_video = nb_present_video + "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_audio = nb_audio + "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_audio = nb_present_audio + "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Audio ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_unknown = nb_unknown + "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_unknown = nb_present_unknown + "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Unknown ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " last_modification_date = strftime('%s')"
                       " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                       // Handle decrement
                       " UPDATE " + Table::Name + " SET"
                           " nb_present_video = nb_present_video - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_video = nb_video - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_audio = nb_present_audio - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Audio ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_audio = nb_audio - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_unknown = nb_present_unknown - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Unknown ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_unknown = nb_unknown - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END),"
                           " last_modification_date = strftime('%s')"
                       " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                       " END";
            }
            /**
             * Change since V31:
             * - Update nb_seen & nb_present_seen
             */
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF type, play_count, group_id ON " + Media::Table::Name +
                       " WHEN (IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) OR"
                       " old.type != new.type OR new.play_count != old.play_count) AND"
                       " new.import_type = " +
                            utils::enum_to_string( Media::ImportType::Internal ) +
                   " BEGIN"
                   // Handle increment
                   " UPDATE " + Table::Name + " SET"
                       " nb_video = nb_video + "
                           "(CASE new.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Video ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_present_video = nb_present_video + "
                           "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_audio = nb_audio + "
                           "(CASE new.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Audio ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_seen = nb_seen + IIF(new.play_count > 0, 1, 0),"
                       " nb_present_audio = nb_present_audio + "
                           "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_unknown = nb_unknown + "
                           "(CASE new.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Unknown ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_present_unknown = nb_present_unknown + "
                           "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_present_seen = nb_present_seen +"
                            " IIF(new.play_count > 0 AND new.is_present, 1, 0),"
                       " last_modification_date = strftime('%s')"
                   " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                   // Handle decrement
                   " UPDATE " + Table::Name + " SET"
                       " nb_present_video = nb_present_video - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_video = nb_video - "
                           "(CASE old.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Video )+
                               " THEN 1 ELSE 0 END),"
                       " nb_present_audio = nb_present_audio - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_present_seen = nb_present_seen -"
                            " IIF(old.play_count > 0 AND old.is_present != 0, 1, 0),"
                       " nb_audio = nb_audio - "
                           "(CASE old.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Audio ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_present_unknown = nb_present_unknown - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_unknown = nb_unknown - "
                           "(CASE old.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Unknown ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_seen = nb_seen - IIF(old.play_count > 0, 1, 0),"
                       " last_modification_date = strftime('%s')"
                   " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                   " END";
        }
        case Triggers::DecrementNbMediaOnDeletion:
        {
            if ( dbModel < 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER DELETE ON " + Media::Table::Name +
                       " WHEN old.group_id IS NOT NULL"
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET"
                           " nb_video = nb_video - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_audio = nb_audio - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_unknown = nb_unknown - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_media = nb_media - 1,"
                           " last_modification_date = strftime('%s')"
                       " WHERE id_group = old.group_id;"
                       " END";
            }
            if ( dbModel < 33 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER DELETE ON " + Media::Table::Name +
                       " WHEN old.group_id IS NOT NULL"
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET"
                           " nb_present_video = nb_present_video - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_video = nb_video - "
                               "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END),"
                           " nb_present_audio = nb_present_audio - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Audio ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_audio = nb_audio - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END),"
                           " nb_present_unknown = nb_present_unknown - "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Unknown ) +
                                       " THEN 1 ELSE 0 END) END),"
                           " nb_unknown = nb_unknown - "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END),"
                           " last_modification_date = strftime('%s')"
                       " WHERE id_group = old.group_id;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER DELETE ON " + Media::Table::Name +
                   " WHEN old.group_id IS NOT NULL"
                   " BEGIN"
                   " UPDATE " + Table::Name + " SET"
                       " nb_present_video = nb_present_video - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_video = nb_video - "
                           "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END),"
                       " nb_present_audio = nb_present_audio - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_present_seen = nb_present_seen -"
                           " IIF(old.play_count > 0 AND old.is_present > 0, 1, 0),"
                       " nb_audio = nb_audio - "
                           "(CASE old.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Audio ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_present_unknown = nb_present_unknown - "
                           "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END) END),"
                       " nb_unknown = nb_unknown - "
                           "(CASE old.type WHEN " +
                               utils::enum_to_string( IMedia::Type::Unknown ) +
                               " THEN 1 ELSE 0 END),"
                       " nb_seen = nb_seen - IIF(old.play_count > 0, 1, 0),"
                       " last_modification_date = strftime('%s')"
                   " WHERE id_group = old.group_id;"
                   " END";
        }
        case Triggers::DeleteEmptyGroups:
        {
            assert( dbModel >= 25 );
            if ( dbModel == 25 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF nb_video, nb_audio, nb_unknown"
                           " ON " + Table::Name +
                       " WHEN new.nb_video = 0 AND new.nb_audio = 0 AND new.nb_unknown = 0"
                       " BEGIN"
                       " DELETE FROM " + Table::Name + " WHERE id_group = new.id_group;"
                       " END";
            }
            if ( dbModel < 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF nb_media"
                           " ON " + Table::Name +
                       " WHEN new.nb_media != old.nb_media AND new.nb_media = 0"
                       " BEGIN"
                       " DELETE FROM " + Table::Name + " WHERE id_group = new.id_group;"
                       " END";
            }
            if ( dbModel == 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF nb_video, nb_audio, nb_unknown"
                           " ON " + Table::Name +
                       " WHEN new.nb_video = 0 AND new.nb_audio = 0 AND new.nb_unknown = 0"
                       " BEGIN"
                       " DELETE FROM " + Table::Name + " WHERE id_group = new.id_group;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                    " AFTER UPDATE OF nb_video, nb_audio, nb_unknown, nb_external"
                        " ON " + Table::Name +
                    " WHEN new.nb_video = 0 AND new.nb_audio = 0 AND new.nb_unknown = 0"
                           " AND new.nb_external = 0"
                    " BEGIN"
                    " DELETE FROM " + Table::Name + " WHERE id_group = new.id_group;"
                    " END";
        }
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
        case Triggers::UpdateTotalNbMedia:
        {
            assert( dbModel >= 26 );
            assert( dbModel < 30 );
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF group_id ON " + Media::Table::Name +
                   " WHEN IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0)"
                   " BEGIN"
                       " UPDATE " + Table::Name + " SET nb_media = nb_media - 1"
                            " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                       " UPDATE " + Table::Name + " SET nb_media = nb_media + 1"
                            " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                   " END";
        }
        case Triggers::UpdateMediaCountOnPresenceChange:
        {
            assert( dbModel >= 26 );
            if ( dbModel < 30 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                       " WHEN old.is_present != new.is_present"
                           " AND new.group_id IS NOT NULL"
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET"
                            /* Compute the increment in 2 steps: first set it to 1 if the
                             * media type matches the targeted field, then negate it if
                             * the media went missing
                             */
                            " nb_video = nb_video + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                            " nb_audio = nb_audio + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                            " nb_unknown = nb_unknown + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                            " WHERE id_group = new.group_id;"
                       " END";
            }
            if ( dbModel < 33 )
            {
                return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                       " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                       " WHEN old.is_present != new.is_present"
                           " AND new.group_id IS NOT NULL"
                       " BEGIN"
                       " UPDATE " + Table::Name + " SET"
                            /* Compute the increment in 2 steps: first set it to 1 if the
                             * media type matches the targeted field, then negate it if
                             * the media went missing
                             */
                            " nb_present_video = nb_present_video + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Video ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                            " nb_present_audio = nb_present_audio + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Audio ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                            " nb_present_unknown = nb_present_unknown + "
                                " (CASE new.type WHEN " +
                                    utils::enum_to_string( IMedia::Type::Unknown ) +
                                " THEN 1 ELSE 0 END) *"
                                " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)"
                            " WHERE id_group = new.group_id;"
                       " END";
            }
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF is_present ON " + Media::Table::Name +
                   " WHEN old.is_present != new.is_present"
                       " AND new.group_id IS NOT NULL"
                   " BEGIN"
                   " UPDATE " + Table::Name + " SET"
                        /* Compute the increment in 2 steps: first set it to 1 if the
                         * media type matches the targeted field, then negate it if
                         * the media went missing
                         */
                        " nb_present_video = nb_present_video + "
                            " (CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Video ) +
                            " THEN 1 ELSE 0 END) *"
                            " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                        " nb_present_audio = nb_present_audio + "
                            " (CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Audio ) +
                            " THEN 1 ELSE 0 END) *"
                            " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                        " nb_present_unknown = nb_present_unknown + "
                            " (CASE new.type WHEN " +
                                utils::enum_to_string( IMedia::Type::Unknown ) +
                            " THEN 1 ELSE 0 END) *"
                            " (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END),"
                        " nb_present_seen = nb_present_seen +"
                            " IIF(new.play_count > 0, 1, 0) * IIF(new.is_present != 0, 1, -1)"
                        " WHERE id_group = new.group_id;"
                   " END";
        }
        case Triggers::UpdateNbMediaOnImportTypeChange:
        {
            assert( dbModel >= 31 );
            /*
             * This is basically the same as the UpdateNbMediaPerType trigger but
             * with the operations reversed to decrement when the media switches to
             * external, and increment when switching back to internal, with an extra
             * case to handle the increment/decrement based on the import type
             */
            return "CREATE TRIGGER " + triggerName( t, dbModel ) +
                   " AFTER UPDATE OF group_id, import_type ON " + Media::Table::Name +
                       " WHEN ("
                           " IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) "
                               " AND new.import_type != " +
                                utils::enum_to_string( Media::ImportType::Internal ) +
                       " ) OR new.import_type != old.import_type" +
                   " BEGIN"
                   // Handle increment
                   " UPDATE " + Table::Name + " SET"
                       " nb_video = nb_video + " +
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_present_video = nb_present_video + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_audio = nb_audio + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_present_audio = nb_present_audio + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Audio ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_unknown = nb_unknown + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_present_unknown = nb_present_unknown + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE new.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE new.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Unknown ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_external = nb_external + "
                           "(CASE new.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN 0 ELSE 1 END),"
                       " last_modification_date = strftime('%s')"
                   " WHERE new.group_id IS NOT NULL AND id_group = new.group_id;"
                   // Handle decrement
                   " UPDATE " + Table::Name + " SET"
                       " nb_present_video = nb_present_video - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Video ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_video = nb_video - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Video ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_present_audio = nb_present_audio - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Audio ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_audio = nb_audio - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Audio ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_present_unknown = nb_present_unknown - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                               " THEN "
                               "(CASE old.is_present WHEN 0 THEN 0 ELSE "
                                   "(CASE old.type WHEN " +
                                       utils::enum_to_string( IMedia::Type::Unknown ) +
                                       " THEN 1 ELSE 0 END) END) ELSE 0 END),"
                       " nb_unknown = nb_unknown - "
                           "(CASE old.import_type WHEN " +
                               utils::enum_to_string( Media::ImportType::Internal ) +
                                   " THEN "
                               "(CASE old.type WHEN " +
                                   utils::enum_to_string( IMedia::Type::Unknown ) +
                                   " THEN 1 ELSE 0 END) ELSE 0 END),"
                       " nb_external = nb_external - "
                           "(CASE old.import_type WHEN " +
                                utils::enum_to_string( Media::ImportType::Internal ) +
                                " THEN 0 ELSE 1 END),"
                       " last_modification_date = strftime('%s')"
                   " WHERE old.group_id IS NOT NULL AND id_group = old.group_id;"
                   " END";
        }
    }
    return "<invalid request>";
}

std::string MediaGroup::triggerName(MediaGroup::Triggers t, uint32_t dbModel)
{
    UNUSED_IN_RELEASE( dbModel );

    assert( dbModel >= 24 );
    switch ( t )
    {
        case Triggers::InsertFts:
            return "media_group_insert_fts";
        case Triggers::DeleteFts:
            return "media_group_delete_fts";
        case Triggers::IncrementNbMediaOnGroupChange:
            assert( dbModel < 26 );
            return "media_group_increment_nb_media";
        case Triggers::DecrementNbMediaOnGroupChange:
            assert( dbModel < 26 );
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
        case Triggers::UpdateNbMediaPerType:
            assert( dbModel >= 26 );
            return "media_group_update_nb_media_types";
        case Triggers::UpdateTotalNbMedia:
            assert( dbModel >= 26 );
            assert( dbModel < 30 );
            return "media_group_update_total_nb_media";
        case Triggers::UpdateMediaCountOnPresenceChange:
            assert( dbModel >= 26 );
            return "media_group_update_nb_media_types_presence";
        case Triggers::UpdateNbMediaOnImportTypeChange:
            assert( dbModel >= 31 );
            return "media_group_update_media_count_on_import_type_change";
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
    UNUSED_IN_RELEASE( dbModel );

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
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    if ( sqlite::Tools::checkTableSchema(
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) == false ||
           sqlite::Tools::checkTableSchema(
                                       schema( FtsTable::Name, Settings::DbModelVersion ),
                                       FtsTable::Name ) == false )
        return false;

    auto check = []( Triggers t ) {
        return sqlite::Tools::checkTriggerStatement(
                                    trigger( t, Settings::DbModelVersion ),
                                    triggerName( t, Settings::DbModelVersion ) );
    };
    auto checkIndex = []( Indexes i ) {
        return sqlite::Tools::checkIndexStatement(
                                    index( i, Settings::DbModelVersion ),
                                    indexName( i, Settings::DbModelVersion ) );
    };

    return check( Triggers::InsertFts ) &&
            check( Triggers::DeleteFts ) &&
            check( Triggers::UpdateNbMediaPerType ) &&
            check( Triggers::DecrementNbMediaOnDeletion ) &&
            check( Triggers::DeleteEmptyGroups ) &&
            check( Triggers::RenameForcedSingleton ) &&
            check( Triggers::UpdateDurationOnMediaChange ) &&
            check( Triggers::UpdateDurationOnMediaDeletion ) &&
            check( Triggers::UpdateNbMediaPerType ) &&
            check( Triggers::UpdateMediaCountOnPresenceChange ) &&
            check( Triggers::UpdateNbMediaOnImportTypeChange ) &&
            checkIndex( Indexes::ForcedSingleton ) &&
            checkIndex( Indexes::Duration ) &&
            checkIndex( Indexes::CreationDate ) &&
            checkIndex( Indexes::LastModificationDate );
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
    /* We need to get a number of characters, not bytes */
    auto nbBytes = utils::str::utf8::nbBytes( title, offset, AutomaticGroupPrefixSize );
    return title.substr( offset, nbBytes + offset );
}

std::string MediaGroup::commonPattern( const std::string& groupName,
                                       const std::string& newTitle )
{
    auto groupIdx = 0u;
    auto titleIdx = 0u;
    if ( strncasecmp( groupName.c_str(), "the ", 4 ) == 0 )
        groupIdx = 4u;
    if ( strncasecmp( newTitle.c_str(), "the ", 4 ) == 0 )
        titleIdx = 4;
    return utils::str::utf8::commonPattern( groupName, groupIdx,
                                            newTitle, titleIdx,
                                            AutomaticGroupPrefixSize );
}

std::string MediaGroup::orderBy(const QueryParameters* params)
{
    std::string req = "ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Alpha;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
        case SortingCriteria::NbAudio:
            req += "mg.nb_present_audio";
            break;
        case SortingCriteria::NbVideo:
            req += "mg.nb_present_video";
            break;
        case SortingCriteria::NbMedia:
            req += "mg.nb_present_audio + mg.nb_present_video + mg.nb_present_unknown";
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
        case SortingCriteria::Default:
        case SortingCriteria::Alpha:
            req += "mg.name";
            break;
    }
    if ( desc == true )
        req += " DESC";
    return req;
}

}
