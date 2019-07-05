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
#include "Device.h"
#include "File.h"
#include "Folder.h"
#include "Playlist.h"
#include "Media.h"
#include "parser/Task.h"
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
{
    row >> m_id
        >> m_step
        >> m_retryCount
        >> m_mrl
        >> m_fileType
        >> m_fileId
        >> m_parentFolderId
        >> m_parentPlaylistId
        >> m_parentPlaylistIndex
        >> m_isRefresh;
}

Task::Task( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
            std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs,
            IFile::Type fileType,
            std::shared_ptr<Playlist> parentPlaylist,
            unsigned int parentPlaylistIndex )
    : m_ml( ml )
    , m_mrl( std::move( mrl ) )
    , m_fileType( fileType )
    , m_parentFolderId( parentFolder->id() )
    , m_parentPlaylistId( parentPlaylist != nullptr ? parentPlaylist->id() : 0 )
    , m_parentPlaylistIndex( parentPlaylistIndex )
    , m_fileFs( std::move( fileFs ) )
    , m_parentFolder( std::move( parentFolder ) )
    , m_parentFolderFs( std::move( parentFolderFs ) )
    , m_parentPlaylist( std::move( parentPlaylist ) )
{
}

Task::Task( MediaLibraryPtr ml, std::shared_ptr<File> file,
            std::shared_ptr<fs::IFile> fileFs )
    : m_ml( ml )
    , m_mrl( file->mrl() )
    , m_fileType( file->type() )
    , m_fileId( file->id() )
    , m_isRefresh( true )
    , m_file( std::move( file ) )
    , m_fileFs( std::move( fileFs ) )
{
}

Task::Task( std::string mrl, IFile::Type fileType, unsigned int playlistIndex )
    : m_mrl( std::move( mrl ) )
    , m_fileType( fileType )
    , m_parentPlaylistIndex( playlistIndex )
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
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_step, m_id );
}

bool Task::decrementRetryCount()
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = retry_count - 1 WHERE id_task = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
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
    return ( static_cast<uint8_t>( m_step ) & static_cast<uint8_t>( step ) ) != 0;
}

void Task::startParserStep()
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = retry_count + 1 WHERE id_task = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
}

int64_t Task::id() const
{
    return m_id;
}

bool Task::restoreLinkedEntities()
{
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
        catch ( const fs::DeviceRemovedException& )
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
            setMrl( mrl );
    }

    // Now we always have a valid MRL, but we might not have a fileId
    // In any case, we need to fetch the corresponding FS entities

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
    catch ( const std::system_error& ex )
    {
        LOG_ERROR( "Failed to restore task: ", ex.what() );
        return false;
    }

    try
    {
        m_fileFs = m_parentFolderFs->file( mrl );
    }
    catch ( const fs::DeviceRemovedException& )
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
    if ( m_parentPlaylistId != 0 )
    {
        m_parentPlaylist = Playlist::fetch( m_ml, m_parentPlaylistId );
        if ( m_parentPlaylist == nullptr )
        {
            LOG_ERROR( "Failed to restore parent playlist #", m_parentPlaylistId );
            return false;
        }
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

void Task::createTable( sqlite::Connection* dbConnection )
{
    std::string reqs[] = {
        #include "database/tables/Task_v18.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

void Task::resetRetryCount( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = 0 WHERE step & ? != ?";
    sqlite::Tools::executeUpdate( ml->getConn(), req, Step::Completed,
                                  Step::Completed);
}

void Task::resetParsing( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "retry_count = 0, step = ?";
    sqlite::Tools::executeUpdate( ml->getConn(), req, Step::None );
}

std::vector<std::shared_ptr<Task>> Task::fetchUncompleted( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + Task::Table::Name + " t"
        " LEFT JOIN " + File::Table::Name + " f ON f.id_file = t.file_id"
        " LEFT JOIN " + Folder::Table::Name + " fol ON f.folder_id = fol.id_folder"
        " LEFT JOIN " + Device::Table::Name + " d ON d.id_device = fol.device_id"
        " WHERE step & ? != ? AND retry_count < 3 AND (d.is_present != 0 OR "
        " t.file_id IS NULL)";
    return Task::fetchAll<Task>( ml, req, Step::Completed,
                                 Step::Completed );
}

std::shared_ptr<Task>
Task::create( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> parentFolder, std::shared_ptr<fs::IDirectory> parentFolderFs,
              IFile::Type fileType,
              std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist )
{
    auto parentFolderId = parentFolder->id();
    auto parentPlaylistId = parentPlaylist.first != nullptr ? parentPlaylist.first->id() : 0;
    auto parentPlaylistIndex = parentPlaylist.second;

    std::shared_ptr<Task> self = std::make_shared<Task>( ml, std::move( mrl ), std::move( fileFs ),
        std::move( parentFolder ), std::move( parentFolderFs ), fileType,
        std::move( parentPlaylist.first ), parentPlaylist.second );
    const std::string req = "INSERT INTO " + Task::Table::Name +
        "(mrl, file_type, parent_folder_id, parent_playlist_id, "
        "parent_playlist_index, is_refresh) "
        "VALUES(?, ?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, self->mrl(), fileType, parentFolderId,
                 sqlite::ForeignKey( parentPlaylistId ),
                 parentPlaylistIndex, false ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Task>
Task::createRefreshTask( MediaLibraryPtr ml, std::shared_ptr<File> file,
                         std::shared_ptr<fs::IFile> fileFs )
{
    auto parentFolderId = file->folderId();
    auto self = std::make_shared<Task>( ml, std::move( file ), std::move( fileFs ) );
    const std::string req = "INSERT INTO " + Task::Table::Name +
            "(mrl, file_type, file_id, parent_folder_id, is_refresh) "
            "VALUES(?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, self->mrl(), self->file()->type(),
                 self->file()->id(), parentFolderId, true ) == false )
        return nullptr;
    return self;
}

void Task::removePlaylistContentTasks( MediaLibraryPtr ml, int64_t playlistId )
{
    const std::string req = "DELETE FROM " + Task::Table::Name + " "
            "WHERE parent_playlist_id = ? AND step & ? = ?";
    sqlite::Tools::executeDelete( ml->getConn(), req, playlistId,
                                  Step::Completed, Step::Completed );
}

void Task::recoverUnscannedFiles( MediaLibraryPtr ml )
{
    static const std::string req = "INSERT INTO " + Task::Table::Name +
            "(file_id, parent_folder_id)"
            " SELECT id_file, folder_id FROM " + File::Table::Name +
            " f LEFT JOIN " + Task::Table::Name + " t"
            " ON t.file_id = f.id_file WHERE t.file_id IS NULL"
            " AND f.folder_id IS NOT NULL";
    sqlite::Tools::executeInsert( ml->getConn(), req );
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

PlaylistPtr Task::parentPlaylist()
{
    return m_parentPlaylist;
}

unsigned int Task::parentPlaylistIndex() const
{
    return m_parentPlaylistIndex;
}

bool Task::isRefresh() const
{
    return m_isRefresh;
}

}

}
