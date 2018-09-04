/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 - 2017 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include <algorithm>

namespace medialibrary
{

namespace parser
{

const std::string Task::Table::Name = "Task";
const std::string Task::Table::PrimaryKeyColumn = "id_task";
int64_t parser::Task::* const Task::Table::PrimaryKey = &parser::Task::m_id;

Task::Task( MediaLibraryPtr ml, sqlite::Row& row )
    : currentService( 0 )
    , m_ml( ml )
{
    std::string mrl;
    unsigned int parentPlaylistIndex;
    bool isRefresh;
    row >> m_id
        >> m_step
        >> m_retryCount
        >> mrl
        >> m_fileId
        >> m_parentFolderId
        >> m_parentPlaylistId
        >> parentPlaylistIndex
        >> isRefresh;
    m_item = Item{ this, std::move( mrl ), parentPlaylistIndex, isRefresh };
}

Task::Task( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
            std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs,
            std::shared_ptr<Playlist> parentPlaylist,
            unsigned int parentPlaylistIndex )
    : currentService( 0 )
    , m_ml( ml )
    , m_step( Step::None )
    , m_fileId( 0 )
    , m_item( this, std::move( fileFs ), std::move( parentFolder ),
              std::move( parentFolderFs ), std::move( parentPlaylist ),
              parentPlaylistIndex, false )
{
}

Task::Task( MediaLibraryPtr ml, std::shared_ptr<File> file,
            std::shared_ptr<fs::IFile> fileFs )
    : currentService( 0 )
    , m_ml( ml )
    , m_step( Step::None )
    , m_fileId( file->id() )
    , m_item( this, std::move( file ), std::move( fileFs ) )
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

bool Task::updateFileId( int64_t fileId )
{
    // When restoring a task, we will invoke ITaskCb::updateFileId while the
    // task already knows the fileId (since we're using it to restore the file instance)
    // In this case, bail out. Otherwise, it is not expected for the task to change
    // its associated file during the processing.
    if ( m_fileId == fileId && fileId != 0 )
        return true ;
    assert( m_fileId == 0 );
    assert( fileId != 0 );
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "file_id = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, fileId, m_id ) == false )
        return false;
    m_fileId = fileId;
    return true;
}

int64_t Task::id() const
{
    return m_id;
}

Task::Item& Task::item()
{
    return m_item;
}

Task::Item::Item( ITaskCb* taskCb, std::string mrl, unsigned int subitemPosition,
                  bool isRefresh )
    : m_taskCb( taskCb )
    , m_mrl( std::move( mrl ) )
    , m_duration( 0 )
    , m_parentPlaylistIndex( subitemPosition )
    , m_isRefresh( isRefresh )
{
}

Task::Item::Item( ITaskCb* taskCb, std::shared_ptr<fs::IFile> fileFs,
                  std::shared_ptr<Folder> parentFolder,
                  std::shared_ptr<fs::IDirectory> parentFolderFs,
                  std::shared_ptr<Playlist> parentPlaylist, unsigned int parentPlaylistIndex,
                  bool isRefresh )
    : m_taskCb( taskCb )
    , m_mrl( fileFs->mrl() )
    , m_duration( 0 )
    , m_fileFs( std::move( fileFs ) )
    , m_parentFolder( std::move( parentFolder ) )
    , m_parentFolderFs( std::move( parentFolderFs ) )
    , m_parentPlaylist( std::move( parentPlaylist ) )
    , m_parentPlaylistIndex( parentPlaylistIndex )
    , m_isRefresh( isRefresh )
{
}

Task::Item::Item( ITaskCb* taskCb, std::shared_ptr<File> file, std::shared_ptr<fs::IFile> fileFs )
    : m_taskCb( taskCb )
    , m_mrl( fileFs->mrl() )
    , m_duration( 0 )
    , m_file( std::move( file ) )
    , m_fileFs( std::move( fileFs ) )
    , m_isRefresh( true )
{
}

std::string Task::Item::meta( Task::Item::Metadata type ) const
{
    auto it = m_metadata.find( type );
    if ( it == end( m_metadata ) )
        return std::string{};
    return it->second;
}

void Task::Item::setMeta( Task::Item::Metadata type, std::string value )
{
    m_metadata[type] = std::move( value );
}

const std::string& Task::Item::mrl() const
{
    return m_mrl;
}

void Task::Item::setMrl( std::string mrl )
{
    m_mrl = std::move( mrl );
}

size_t Task::Item::nbSubItems() const
{
    return m_subItems.size();
}

const IItem& Task::Item::subItem( unsigned int index ) const
{
    return m_subItems[index];
}

IItem& Task::Item::createSubItem( std::string mrl, unsigned int playlistIndex )
{
    m_subItems.emplace_back( nullptr, std::move( mrl ), playlistIndex, false );
    return m_subItems.back();
}

int64_t Task::Item::duration() const
{
    return m_duration;
}

void Task::Item::setDuration( int64_t duration )
{
    m_duration = duration;
}

const std::vector<Task::Item::Track>& Task::Item::tracks() const
{
    return m_tracks;
}

void Task::Item::addTrack(Task::Item::Track t)
{
    m_tracks.emplace_back( std::move( t ) );
}

MediaPtr Task::Item::media()
{
    return m_media;
}

void Task::Item::setMedia( MediaPtr media )
{
    m_media = std::move( media );
}

FilePtr Task::Item::file()
{
    return m_file;
}

bool Task::Item::setFile( FilePtr file)
{
    m_file = std::move( file );
    assert( m_taskCb != nullptr );
    return m_taskCb->updateFileId( m_file->id() );
}

FolderPtr Task::Item::parentFolder()
{
    return m_parentFolder;
}

std::shared_ptr<fs::IFile> Task::Item::fileFs()
{
    return m_fileFs;
}

std::shared_ptr<fs::IDirectory> Task::Item::parentFolderFs()
{
    return m_parentFolderFs;
}

PlaylistPtr Task::Item::parentPlaylist()
{
    return m_parentPlaylist;
}

unsigned int Task::Item::parentPlaylistIndex() const
{
    return m_parentPlaylistIndex;
}

bool Task::Item::isRefresh() const
{
    return m_isRefresh;
}

bool Task::restoreLinkedEntities()
{
    LOG_INFO("Restoring linked entities of task ", m_id);
    // MRL will be empty if the task has been resumed from unparsed files
    // parentFolderId == 0 indicates an external file
    auto mrl = m_item.mrl();
    if ( mrl.empty() == true && m_parentFolderId == 0 )
    {
        LOG_WARN( "Aborting & removing external file task (#", m_id, ')' );
        destroy( m_ml, m_id );
        return false;
    }
    // First of all, we need to know if the file has been created already
    // ie. have we run the MetadataParser service, at least partially
    std::shared_ptr<File> file;
    if ( m_fileId != 0 )
        file = File::fetch( m_ml, m_fileId );

    // We might re-create tasks without mrl to ease the handling of files on
    // external storage.
    if ( mrl.empty() == true )
    {
        // but we expect those to be created from an existing file after a
        // partial/failed migration. If we don't have a file nor an mrl, we
        // can't really process it.
        if ( file == nullptr )
        {
            assert( !"Can't process a file without a file nor an mrl" );
            return false;
        }
        auto folder = Folder::fetch( m_ml, file->folderId() );
        if ( folder == nullptr )
        {
            assert( !"Can't file the folder associated with a file" );
            // If the folder can't be found in db while the file can, it looks
            // a lot like a sporadic failure, since file are deleted through
            // triggers when its parent folder gets deleted. Just postpone this.
            return false;
        }
        if ( folder->isPresent() == false )
        {
            LOG_WARN( "Postponing rescan of removable file ", file->rawMrl(),
                      " until the device containing it is present again" );
            return false;
        }
        setMrl( file->mrl() );
    }
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
        return false;

    std::shared_ptr<fs::IDirectory> parentFolderFs;
    try
    {
        parentFolderFs = fsFactory->createDirectory( utils::file::directory( mrl ) );
    }
    catch ( const std::system_error& ex )
    {
        LOG_ERROR( "Failed to restore task: ", ex.what() );
        return false;
    }

    std::shared_ptr<fs::IFile> fileFs;
    try
    {
        auto files = parentFolderFs->files();
        auto it = std::find_if( begin( files ), end( files ), [&mrl]( std::shared_ptr<fs::IFile> f ) {
            return f->mrl() == mrl;
        });
        if ( it == end( files ) )
        {
            LOG_ERROR( "Failed to restore fs::IFile associated with ", mrl );
            return false;
        }
        fileFs = std::move( *it );
    }
    catch ( const std::system_error& ex )
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

    std::shared_ptr<Folder> parentFolder = Folder::fetch( m_ml, m_parentFolderId );
    if ( parentFolder == nullptr )
    {
        LOG_ERROR( "Failed to restore parent folder #", m_parentFolderId );
        return false;
    }
    std::shared_ptr<Playlist> parentPlaylist;
    if ( m_parentPlaylistId != 0 )
    {
        parentPlaylist = Playlist::fetch( m_ml, m_parentPlaylistId );
        if ( parentPlaylist == nullptr )
        {
            LOG_ERROR( "Failed to restore parent playlist #", m_parentPlaylistId );
            return false;
        }
    }

    m_item = Item{ this, std::move( fileFs ), std::move( parentFolder ),
                   std::move( parentFolderFs ), std::move( parentPlaylist ),
                   m_item.parentPlaylistIndex(), m_item.isRefresh() };
    if ( file != nullptr )
    {
        m_item.setMedia( file->media() );
        m_item.setFile( std::move( file ) );
    }
    return true;
}

void Task::setMrl( std::string newMrl )
{
    if ( m_item.mrl() == newMrl )
        return;
    static const std::string req = "UPDATE " + Task::Table::Name + " SET "
            "mrl = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, newMrl, m_id ) == false )
        return;
    m_item.setMrl( std::move( newMrl ) );
}

void Task::createTable( sqlite::Connection* dbConnection )
{
    std::string reqs[] = {
        #include "database/tables/Task_v14.sql"
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
Task::create( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> parentFolder, std::shared_ptr<fs::IDirectory> parentFolderFs,
              std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist )
{
    auto parentFolderId = parentFolder->id();
    auto parentPlaylistId = parentPlaylist.first != nullptr ? parentPlaylist.first->id() : 0;
    auto parentPlaylistIndex = parentPlaylist.second;

    std::shared_ptr<Task> self = std::make_shared<Task>( ml, std::move( fileFs ),
        std::move( parentFolder ), std::move( parentFolderFs ),
        std::move( parentPlaylist.first ), parentPlaylist.second );
    const std::string req = "INSERT INTO " + Task::Table::Name +
        "(mrl, parent_folder_id, parent_playlist_id, parent_playlist_index, is_refresh) "
        "VALUES(?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, self->m_item.mrl(), parentFolderId,
                 sqlite::ForeignKey( parentPlaylistId ),
                 parentPlaylistIndex, false ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<Task>
Task::create( MediaLibraryPtr ml, std::shared_ptr<File> file,
              std::shared_ptr<fs::IFile> fileFs )
{
    auto self = std::make_shared<Task>( ml, std::move( file ), std::move( fileFs ) );
    const std::string req = "INSERT INTO " + Task::Table::Name +
            "(mrl, file_id, is_refresh) VALUES(?, ?, ?)";
    if ( insert( ml, self, req, self->m_item.mrl(), self->m_item.file()->id(),
                 true ) == false )
        return nullptr;
    return self;
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

}

}
