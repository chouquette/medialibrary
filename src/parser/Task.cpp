/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 - 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *          Alexandre Fernandez <nerf@boboop.fr>
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

#include "Task.h"

#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/Errors.h"
#include "Device.h"
#include "File.h"
#include "Folder.h"
#include "Playlist.h"
#include "Media.h"
#include "parser/Task.h"
#include "parser/Parser.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/Strings.h"

#include <algorithm>

namespace medialibrary
{

namespace parser
{

const std::string Task::Table::Name = "Task";
const std::string Task::Table::PrimaryKeyColumn = "id_task";
int64_t parser::Task::* const Task::Table::PrimaryKey = &parser::Task::m_id;

Task::Task( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_step( row.extract<decltype(m_step)>() )
    , m_retryCount( row.extract<decltype(m_retryCount)>() )
    , m_type( row.extract<decltype(m_type)>() )
    , m_mrl( row.extract<decltype(m_mrl)>() )
    , m_fileType( row.extract<decltype(m_fileType)>() )
    , m_fileId( row.extract<decltype(m_fileId)>() )
    , m_parentFolderId( row.extract<decltype(m_parentFolderId)>() )
    , m_linkToId( row.extract<decltype(m_linkToId)>() )
    , m_linkToType( row.extract<decltype(m_linkToType)>() )
    , m_linkExtra( row.extract<decltype(m_linkExtra)>() )
{
    assert( row.hasRemainingColumns() == false );
}

Task::Task( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
            std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs,
            IFile::Type fileType )
    : m_ml( ml )
    , m_type( Type::Creation )
    , m_mrl( std::move( mrl ) )
    , m_fileType( fileType )
    , m_parentFolderId( parentFolder->id() )
    , m_fileFs( std::move( fileFs ) )
    , m_parentFolder( std::move( parentFolder ) )
    , m_parentFolderFs( std::move( parentFolderFs ) )
{
}

Task::Task( MediaLibraryPtr ml, std::shared_ptr<File> file,
            std::shared_ptr<fs::IFile> fileFs, std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs )
    : m_ml( ml )
    , m_type( Type::Refresh )
    , m_mrl( file->mrl() )
    , m_fileType( file->type() )
    , m_fileId( file->id() )
    , m_file( std::move( file ) )
    , m_fileFs( std::move( fileFs ) )
    , m_parentFolder( std::move( parentFolder ) )
    , m_parentFolderFs( std::move( parentFolderFs ) )
{
}

Task::Task( MediaLibraryPtr ml, std::string mrl, int64_t linkToId,
            Task::LinkType linkToType, int64_t linkExtra )
    : m_ml( ml )
    , m_type( Type::Link )
    , m_mrl( std::move( mrl ) )
    , m_linkToId( linkToId )
    , m_linkToType( linkToType )
    , m_linkExtra( linkExtra )
{
}

Task::Task( MediaLibraryPtr ml, std::string mrl , IFile::Type fileType )
    : m_ml( ml )
    , m_type( Type::Restore )
    , m_mrl( std::move( mrl ) )
    , m_fileType( fileType )
{
}

Task::Task( std::string mrl, IFile::Type fileType, unsigned int playlistIndex )
    : m_type( Type::Temporary )
    , m_mrl( std::move( mrl ) )
    , m_fileType( fileType )
    , m_linkToType( LinkType::Playlist )
    , m_linkExtra( playlistIndex )
{
}

void Task::markStepCompleted( Step stepCompleted )
{
    m_step = static_cast<Step>( static_cast<uint8_t>( m_step ) |
                                      static_cast<uint8_t>( stepCompleted ) );
}

bool Task::saveParserStep()
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET step = ?, "
            "retry_count = 0 WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_step, m_id ) == false )
        return false;
    m_retryCount = 0;
    return true;
}

bool Task::decrementRetryCount()
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = retry_count - 1 WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id ) == false )
        return false;
    --m_retryCount;
    return true;
}

bool Task::isCompleted() const
{
    using StepType = typename std::underlying_type<Step>::type;
    return ( static_cast<StepType>( m_step ) &
             static_cast<StepType>( Step::Completed ) ) ==
             static_cast<StepType>( Step::Completed );
}

bool Task::isStepCompleted( Step step ) const
{
    if ( m_type == Type::Link )
    {
        // We always want to return true for any step which isn't the linking step
        // since we don't want to run any service other than the linking one
        return step != Step::Linking;
    }
    return ( static_cast<uint8_t>( m_step ) & static_cast<uint8_t>( step ) ) != 0;
}

void Task::startParserStep()
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = retry_count + 1 WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id ) == false )
        return;
    ++m_retryCount;
}

uint32_t Task::goToNextService()
{
    return ++m_currentService;
}

void Task::resetCurrentService()
{
    m_currentService = 0;
}

unsigned int Task::retryCount() const
{
    return m_retryCount;
}

int64_t Task::id() const
{
    return m_id;
}

bool Task::restoreLinkedEntities( LastTaskInfo& lastTask )
{
    // No need to restore anything for link tasks, they only contain mrls & ids.
    assert( needEntityRestoration() == true );
    LOG_DEBUG("Restoring linked entities of task ", m_id);
    // MRL will be empty if the task has been resumed from unparsed files
    // (during 11 -> 12 migration)
    auto mrl = m_mrl;
    if ( mrl.empty() == true && m_fileId == 0 )
    {
        LOG_WARN( "Aborting & removing file task without mrl nor file id (#", m_id, ')' );
        destroy( m_ml, m_id );
        return false;
    }
    // First of all, we need to know if the file has been created already
    // ie. have we run the MetadataParser service, at least partially
    std::shared_ptr<File> file;
    if ( m_fileId != 0 )
    {
        file = File::fetch( m_ml, m_fileId );
        if ( file == nullptr )
        {
            LOG_WARN( "Failed to restore file associated to the task. Task will "
                      "be dropped" );
            destroy( m_ml, m_id );
            return false;
        }
    }

    // Now we either have a task with an existing file, and we managed to fetch
    // it, or the task was not processed yet, and we don't have a fileId (and
    // therefor no file instance)
    assert( m_fileId == 0 || file != nullptr );

    // In case we have a refresh task, old versions didn't provide the parent
    // id. That is:
    // 0.4.x branch before vlc-android 3.1.5 (excluded)
    // 0.5.x branch before 2019 May 17 (unshipped in releases)
    // However we must have either a file ID (mandatory for a refresh task) or a
    // parent folder ID (mandatory when discovering a file)
    assert( m_fileId != 0 || m_parentFolderId != 0 );

    // Regardless of the stored mrl, always fetch the file from DB and query its
    // mrl. If might have changed in case we're dealing with a removable storage
    if ( file != nullptr )
    {
        try
        {
            mrl = file->mrl();
        }
        catch ( const fs::errors::DeviceRemoved& )
        {
            LOG_WARN( "Postponing rescan of removable file ", file->rawMrl(),
                      " until the device containing it is present again" );
            return false;
        }
        assert( mrl.empty() == false );
        // If we are migrating a task without an mrl, store the mrl for future use
        // In case the mrl changed, update it as well, as the later points of the
        // parsing process will depend on the mrl stored in the item, not the
        // one we now have here.
        if ( m_mrl.empty() == true || m_mrl != mrl )
        {
            // There can be an existing task with the same MRL, in case a task
            // wasn't processed before we re detect a file as new
            // See https://code.videolan.org/videolan/medialibrary/issues/157
            try
            {
                setMrl( mrl );
            }
            catch ( const sqlite::errors::ConstraintUnique& )
            {
                LOG_INFO( "Duplicated task after mrl update, discarding the "
                          "duplicate." );
                destroy( m_ml, m_id );
            }
        }
    }

    // Now we always have a valid MRL, but we might not have a fileId
    // In any case, we need to fetch the corresponding FS entities

    if ( lastTask.parentFolderId == m_parentFolderId && m_parentFolderId != 0 )
    {
        m_parentFolderFs = lastTask.fsDir;
    }
    else
    {
        auto fsFactory = m_ml->fsFactoryForMrl( mrl );
        if ( fsFactory == nullptr )
        {
            LOG_WARN( "No fs factory matched the task mrl (", mrl, "). Postponing" );
            return false;
        }

        try
        {
            m_parentFolderFs = fsFactory->createDirectory( utils::file::directory( mrl ) );
        }
        catch ( const fs::errors::System& ex )
        {
            LOG_ERROR( "Failed to restore task: ", ex.what() );
            return false;
        }
        lastTask.fsDir = m_parentFolderFs;
        lastTask.parentFolderId = m_parentFolderId;
    }

    try
    {
        m_fileFs = m_parentFolderFs->file( mrl );
    }
    catch ( const fs::errors::DeviceRemoved& )
    {
        LOG_WARN( "Failed to restore file on an unmounted device: ", mrl );
        return false;
    }
    catch ( const std::exception& ex )
    {
        // If we never found the file yet, we can delete the task. It will be
        // recreated upon next discovery
        if ( file == nullptr )
        {
            LOG_WARN( "Failed to restore file system instances for mrl ", mrl, "(",
                      ex.what(), ").", " Removing the task until it gets detected again." );
            destroy( m_ml, m_id );
        }
        else
        {
            // Otherwise we need to postpone it, although most likely we will
            // detect that the file is now missing, and we won't try to restore
            // this task until it comes back (since the task restoration request
            // includes the file.is_present flag)
            LOG_WARN( "Failed to restore file system instances for mrl ", mrl, "."
                      " Postponing the task." );
        }
        return false;
    }

    // Handle old refresh tasks without a parent folder id
    if ( m_parentFolderId == 0 )
    {
        assert( m_fileId != 0 && file != nullptr );
        m_parentFolderId = file->folderId();
    }
    m_parentFolder = Folder::fetch( m_ml, m_parentFolderId );
    if ( m_parentFolder == nullptr )
    {
        LOG_ERROR( "Failed to restore parent folder #", m_parentFolderId );
        return false;
    }

    if ( file != nullptr )
    {
        // Don't try to restore the media from a playlist file
        if ( file->isMain() == true )
        {
            m_media = file->media();
            if ( m_media == nullptr )
            {
                LOG_ERROR( "Failed to restore attached media" );
                return false;
            }
        }
        m_file = std::move( file );
    }
    return true;
}

void Task::setMrl( std::string newMrl )
{
    if ( m_mrl == newMrl )
        return;
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "mrl = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, newMrl, m_id ) == false )
        return;
    m_mrl = std::move( newMrl );
}

void Task::createTable( sqlite::Connection* dbConnection, uint32_t dbModel )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, dbModel, false ) );
}

void Task::createTriggers( sqlite::Connection* dbConnection, uint32_t dbModel )
{
    if ( dbModel < 18 )
        return;
    sqlite::Tools::executeRequest( dbConnection,
                                   trigger( Triggers::DeletePlaylistLinkingTask, dbModel ) );
}

void Task::createIndex( sqlite::Connection* dbConnection, uint32_t dbModel )
{
    if ( dbModel < 24 )
        return;
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::ParentFolderId, dbModel ) );
}

std::string Task::schema( const std::string& tableName, uint32_t dbModel,
                          bool backup )
{
    assert( tableName == Table::Name );
    if ( dbModel <= 17 )
    {
        assert( backup == false );
        return "CREATE TABLE " + Table::Name +
        "("
            "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
            "step INTEGER NOT NULL DEFAULT 0,"
            "retry_count INTEGER NOT NULL DEFAULT 0,"
            "mrl TEXT,"
            "file_type INTEGER NOT NULL,"
            "file_id UNSIGNED INTEGER,"
            "parent_folder_id UNSIGNED INTEGER,"
            "parent_playlist_id INTEGER,"
            "parent_playlist_index UNSIGNED INTEGER,"
            "is_refresh BOOLEAN NOT NULL DEFAULT 0,"
            "UNIQUE(mrl, parent_playlist_id, is_refresh) ON CONFLICT FAIL,"
            "FOREIGN KEY(parent_folder_id) REFERENCES " + Folder::Table::Name
            + "(id_folder) ON DELETE CASCADE,"
            "FOREIGN KEY(file_id) REFERENCES " + File::Table::Name
            + "(id_file) ON DELETE CASCADE,"
            "FOREIGN KEY(parent_playlist_id) REFERENCES " + Playlist::Table::Name
            + "(id_playlist) ON DELETE CASCADE"
        ")";
    }
    auto req = std::string{ "CREATE " } + (backup ? "TEMPORARY " : "") +
            "TABLE " + Table::Name + (backup ? "_backup" : "" ) +
    "("
        "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
        "step INTEGER NOT NULL DEFAULT 0,"
        "retry_count INTEGER NOT NULL DEFAULT 0,"
        "type INTEGER NOT NULL,"
        "mrl TEXT,"
        "file_type INTEGER NOT NULL,"
        "file_id UNSIGNED INTEGER,"
        "parent_folder_id UNSIGNED INTEGER,";

        // For linking purposes
        // link_to_id needs to be not null, so the unique constraint always take
        // its value into account
        // Starting from model 22, this is true for all link_ fields.

    if ( dbModel >= 22 )
    {
        req += "link_to_id UNSIGNED INTEGER NOT NULL,"
               "link_to_type UNSIGNED INTEGER NOT NULL,"
               "link_extra UNSIGNED INTEGER NOT NULL,"
               "UNIQUE(mrl,type, link_to_id, link_to_type, link_extra) ON CONFLICT FAIL,";
    }
    else if ( dbModel >= 20 )
    {
        req += "link_to_id UNSIGNED INTEGER NOT NULL,"
               "link_to_type UNSIGNED INTEGER,"
               "link_extra UNSIGNED INTEGER,"
               "UNIQUE(mrl,type, link_to_id) ON CONFLICT FAIL,";
    }
    else
    {
        req += "link_to_id UNSIGNED INTEGER, "
               "link_to_type UNSIGNED INTEGER,"
               "link_extra UNSIGNED INTEGER,"
               "UNIQUE(mrl,type) ON CONFLICT FAIL,";
    }
    req += "FOREIGN KEY(parent_folder_id) REFERENCES " + Folder::Table::Name
            + "(id_folder) ON DELETE CASCADE,"
            "FOREIGN KEY(file_id) REFERENCES " + File::Table::Name
            + "(id_file) ON DELETE CASCADE"
    ")";
    return req;
}

std::string Task::trigger(Task::Triggers trigger, uint32_t dbModel )
{
    assert( trigger == Triggers::DeletePlaylistLinkingTask );
    assert( dbModel >= 18 );
    return "CREATE TRIGGER " + triggerName( trigger, dbModel ) +
           " AFTER DELETE ON " + Playlist::Table::Name +
           " BEGIN "
               "DELETE FROM " + Table::Name + " "
                   "WHERE link_to_type = " +
                           std::to_string( static_cast<std::underlying_type_t<IItem::LinkType>>(
                               IItem::LinkType::Playlist ) ) + " "
                       "AND link_to_id = old.id_playlist "
                       "AND type = " +
                           std::to_string( static_cast<std::underlying_type_t<Type>>(
                               Type::Link ) ) + ";"
           "END";

}

std::string Task::triggerName( Triggers trigger, uint32_t dbModel )
{
    assert( trigger == Triggers::DeletePlaylistLinkingTask );
    assert( dbModel >= 18 );
    return "delete_playlist_linking_tasks";
}

std::string Task::index(Task::Indexes index, uint32_t dbModel)
{
    assert( index == Indexes::ParentFolderId );
    assert( dbModel >= 24 );
    return "CREATE INDEX " + indexName( index, dbModel ) +
            " ON " + Table::Name + "(parent_folder_id)";
}

std::string Task::indexName( Task::Indexes index, uint32_t dbModel )
{
    assert( index == Indexes::ParentFolderId );
    assert( dbModel >= 24 );
    return "task_parent_folder_id_idx";
}

bool Task::checkDbModel( MediaLibraryPtr ml )
{
    return sqlite::Tools::checkTableSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion, false ),
                                       Table::Name ) &&
           sqlite::Tools::checkTriggerStatement( ml->getConn(),
                trigger( Triggers::DeletePlaylistLinkingTask, Settings::DbModelVersion ),
                triggerName( Triggers::DeletePlaylistLinkingTask, Settings::DbModelVersion ) ) &&
           sqlite::Tools::checkIndexStatement( ml->getConn(),
                index( Indexes::ParentFolderId, Settings::DbModelVersion ),
                indexName( Indexes::ParentFolderId, Settings::DbModelVersion ) );
}

bool Task::resetRetryCount( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = 0 WHERE step & ?1 != ?1 AND step != ?2";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, Step::Completed,
                                         Step::Linking );
}

bool Task::resetParsing( MediaLibraryPtr ml )
{
    assert( sqlite::Transaction::transactionInProgress() == true );
    static const std::string resetReq = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = 0, step = ?";
    /* We also want to delete the refresh tasks, since we are going to rescan
     * all existing media anyway
     */
    static const std::string deleteRefreshReq = "DELETE FROM " + Task::Table::Name +
            " WHERE type = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), deleteRefreshReq, Type::Refresh ) &&
            sqlite::Tools::executeUpdate( ml->getConn(), resetReq, Step::None );
}

std::vector<std::shared_ptr<Task>> Task::fetchUncompleted( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT t.* FROM " + Task::Table::Name + " t"
        " LEFT JOIN " + File::Table::Name + " f ON f.id_file = t.file_id"
        " LEFT JOIN " + Folder::Table::Name + " fol ON f.folder_id = fol.id_folder"
        " LEFT JOIN " + Device::Table::Name + " d ON d.id_device = fol.device_id"
        " WHERE step & ? != ? AND retry_count <= ? AND (d.is_present != 0 OR "
        " t.file_id IS NULL) ORDER BY parent_folder_id";
    return Task::fetchAll<Task>( ml, req, Step::Completed,
                                 Step::Completed, parser::MaxNbRetries );
}

std::shared_ptr<Task>
Task::create( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> parentFolder, std::shared_ptr<fs::IDirectory> parentFolderFs,
              IFile::Type fileType )
{
    auto parentFolderId = parentFolder->id();

    std::shared_ptr<Task> self = std::make_shared<Task>( ml, std::move( mrl ), std::move( fileFs ),
        std::move( parentFolder ), std::move( parentFolderFs ), fileType );
    const std::string req = "INSERT INTO " + Task::Table::Name +
        "(type, mrl, file_type, parent_folder_id, link_to_id, link_to_type, "
            "link_extra)"
            "VALUES(?, ?, ?, ?, 0, 0, 0)";
    if ( insert( ml, self, req, Type::Creation, self->mrl(), fileType,
                 parentFolderId ) == false )
        return nullptr;
    auto parser = ml->getParser();
    if ( parser != nullptr )
        parser->parse( self );
    return self;
}

std::shared_ptr<Task>
Task::createRefreshTask( MediaLibraryPtr ml, std::shared_ptr<File> file,
                         std::shared_ptr<fs::IFile> fileFs,
                         std::shared_ptr<Folder> parentFolder,
                         std::shared_ptr<fs::IDirectory> parentFolderFs )
{
    auto parentFolderId = file->folderId();
    auto self = std::make_shared<Task>( ml, std::move( file ), std::move( fileFs ),
                                        std::move( parentFolder ),
                                        std::move( parentFolderFs ) );
    const std::string req = "INSERT INTO " + Task::Table::Name +
            "(type, mrl, file_type, file_id, parent_folder_id, link_to_id, "
            "link_to_type, link_extra)"
            "VALUES(?, ?, ?, ?, ?, 0, 0, 0)";
    if ( insert( ml, self, req, Type::Refresh, self->mrl(), self->file()->type(),
                 self->file()->id(), parentFolderId ) == false )
        return nullptr;
    auto parser = ml->getParser();
    if ( parser != nullptr )
        parser->parse( self );
    return self;
}

std::shared_ptr<Task>
Task::createMediaRefreshTask( MediaLibraryPtr ml, std::shared_ptr<Media> media )
{
    auto files = media->files();
    auto mainFileIt = std::find_if( cbegin( files ), cend( files ),
                                  []( const std::shared_ptr<IFile>& f ) {
        return f->isMain();
    });
    if ( mainFileIt == cend( files ) )
        return nullptr;
    auto mainFile = std::static_pointer_cast<File>( *mainFileIt );
    auto mrl = mainFile->mrl();
    auto fsFactory = ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
        return nullptr;
    auto folder = Folder::fetch( ml, mainFile->folderId() );
    if ( folder == nullptr )
        return nullptr;
    auto folderMrl = utils::file::directory( mrl );
    std::shared_ptr<fs::IDirectory> folderFs;
    std::shared_ptr<fs::IFile> fileFs;
    try
    {
        folderFs = fsFactory->createDirectory( folderMrl );
        fileFs = folderFs->file( mrl );
    }
    catch ( const fs::errors::Exception& ex )
    {
        LOG_INFO( "Failed to create a media restore task: ", ex.what() );
        return nullptr;
    }
    assert( fileFs != nullptr );
    return createRefreshTask( ml, std::move( mainFile ), std::move( fileFs ),
                              std::move( folder ), std::move( folderFs ) );
}

std::shared_ptr<Task> Task::createLinkTask( MediaLibraryPtr ml, std::string mrl,
                                            int64_t linkToId, Task::LinkType linkToType,
                                            int64_t linkToExtra )
{
    auto self = std::make_shared<Task>( ml, std::move( mrl ), linkToId, linkToType,
                                        linkToExtra );
    const std::string req = "INSERT INTO " + Task::Table::Name +
            "(type, mrl, file_type, file_id, parent_folder_id, link_to_id,"
            "link_to_type, link_extra) VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, Type::Link, self->mrl(), IFile::Type::Unknown,
                 nullptr, nullptr, linkToId, linkToType, linkToExtra ) == false )
        return nullptr;
    auto parser = ml->getParser();
    if ( parser != nullptr )
        parser->parse( self );
    return self;
}

std::shared_ptr<Task> Task::createRestoreTask( MediaLibraryPtr ml, std::string mrl,
                                               IFile::Type fileType )
{
    auto self = std::make_shared<Task>( ml, std::move( mrl ), fileType );
    const std::string req = "INSERT INTO " + Table::Name +
            "(type, mrl, file_type, link_to_id, link_to_type, link_extra) "
            "VALUES(?, ?, ?, 0, 0, 0)";
    if ( insert( ml, self, req, Type::Restore, self->mrl(), fileType ) == false )
        return nullptr;
    auto parser = ml->getParser();
    if ( parser != nullptr )
        parser->parse( self );
    return self;
}

bool Task::removePlaylistContentTasks( MediaLibraryPtr ml, int64_t playlistId )
{
    const std::string req = "DELETE FROM " + Task::Table::Name + " "
            "WHERE type = ? AND link_to_type = ? AND link_to_id = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, Task::Type::Link,
                                         LinkType::Playlist, playlistId );
}

bool Task::removePlaylistContentTasks( MediaLibraryPtr ml )
{
    const std::string req = "DELETE FROM " + Task::Table::Name + " "
            "WHERE type = ? AND link_to_type = ?";
    return sqlite::Tools::executeDelete( ml->getConn(), req, Task::Type::Link,
                                         LinkType::Playlist );
}

/*
 * This is used only by the 11 -> 12 migration, and is refering to an old
 * DB model on purpose
 */
bool Task::recoverUnscannedFiles( MediaLibraryPtr ml )
{
    static const std::string req = "INSERT INTO " + Task::Table::Name +
            "(file_id, parent_folder_id)"
            " SELECT id_file, folder_id FROM " + File::Table::Name +
            " f LEFT JOIN " + Task::Table::Name + " t"
            " ON t.file_id = f.id_file WHERE t.file_id IS NULL"
            " AND f.folder_id IS NOT NULL";
    return sqlite::Tools::executeInsert( ml->getConn(), req );
}

std::string Task::meta( Task::Metadata type ) const
{
    auto it = m_metadata.find( type );
    if ( it == end( m_metadata ) )
        return std::string{};
    return it->second;
}

void Task::setMeta( Task::Metadata type, std::string value )
{
    utils::str::trim( value );
    m_metadata[type] = std::move( value );
}

const std::string& Task::mrl() const
{
    return m_mrl;
}

IFile::Type Task::fileType() const
{
    return m_fileType;
}

size_t Task::nbSubItems() const
{
    return m_subItems.size();
}

const IItem& Task::subItem( unsigned int index ) const
{
    return m_subItems[index];
}

IItem& Task::createSubItem( std::string mrl, unsigned int playlistIndex )
{
    m_subItems.emplace_back( std::move( mrl ), IFile::Type::Main, playlistIndex );
    return m_subItems.back();
}

int64_t Task::duration() const
{
    return m_duration;
}

void Task::setDuration( int64_t duration )
{
    m_duration = duration;
}

const std::vector<Task::Track>& Task::tracks() const
{
    return m_tracks;
}

void Task::addTrack( Task::Track&& t )
{
    m_tracks.emplace_back( std::move( t ) );
}

MediaPtr Task::media()
{
    return m_media;
}

void Task::setMedia( MediaPtr media )
{
    m_media = std::move( media );
}

FilePtr Task::file()
{
    return m_file;
}

bool Task::setFile( FilePtr file )
{
    auto fileId = file->id();
    if ( m_fileId == fileId && m_fileId != 0 )
        return true ;
    assert( m_fileId == 0 );
    assert( fileId != 0 );
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "file_id = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, fileId, m_id ) == false )
        return false;
    m_fileId = fileId;
    m_file = std::move( file );
    return true;
}

FolderPtr Task::parentFolder()
{
    return m_parentFolder;
}

std::shared_ptr<fs::IFile> Task::fileFs()
{
    return m_fileFs;
}

std::shared_ptr<fs::IDirectory> Task::parentFolderFs()
{
    return m_parentFolderFs;
}

bool Task::isRefresh() const
{
    return m_type == Type::Refresh;
}

bool Task::isLinkTask() const
{
    return m_type == Type::Link;
}

bool Task::isRestore() const
{
    return m_type == Type::Restore;
}

IItem::LinkType Task::linkType() const
{
    return m_linkToType;
}

int64_t Task::linkToId() const
{
    return m_linkToId;
}

int64_t Task::linkExtra() const
{
    return m_linkExtra;
}

bool Task::needEntityRestoration() const
{
    if ( isLinkTask() == true || isRestore() == true )
        return false;
    return m_parentFolderFs == nullptr ||
           m_fileFs == nullptr ||
           m_parentFolder == nullptr;
}

}

}
