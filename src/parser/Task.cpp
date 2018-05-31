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

#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "File.h"
#include "Folder.h"
#include "Playlist.h"
#include "parser/Task.h"
#include "utils/Filename.h"
#include "utils/Url.h"

namespace medialibrary
{

const std::string policy::TaskTable::Name = "Task";
const std::string policy::TaskTable::PrimaryKeyColumn = "id_task";
int64_t parser::Task::* const policy::TaskTable::PrimaryKey = &parser::Task::m_id;

namespace parser
{

Task::Task( MediaLibraryPtr ml, sqlite::Row& row )
    : currentService( 0 )
    , m_ml( ml )
{
    std::string mrl;
    row >> m_id
        >> m_step
        >> m_retryCount
        >> mrl
        >> m_fileId
        >> m_parentFolderId
        >> m_parentPlaylistId
        >> parentPlaylistIndex;
    m_item = Item{ std::move( mrl ) };
}

Task::Task( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
            std::shared_ptr<Folder> parentFolder,
            std::shared_ptr<fs::IDirectory> parentFolderFs,
            std::shared_ptr<Playlist> parentPlaylist,
            unsigned int parentPlaylistIndex )
    : fileFs( std::move( fileFs ) )
    , parentFolder( std::move( parentFolder ) )
    , parentFolderFs( std::move( parentFolderFs ) )
    , parentPlaylist( std::move( parentPlaylist ) )
    , parentPlaylistIndex( parentPlaylistIndex )
    , currentService( 0 )
    , m_ml( ml )
    , m_step( ParserStep::None )
    , m_fileId( 0 )
    , m_item( this->fileFs->mrl() )
{
}

void Task::markStepCompleted( ParserStep stepCompleted )
{
    m_step = static_cast<ParserStep>( static_cast<uint8_t>( m_step ) |
                                      static_cast<uint8_t>( stepCompleted ) );
}

bool Task::saveParserStep()
{
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET step = ?, "
            "retry_count = 0 WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_step, m_id ) == false )
        return false;
    return true;
}

bool Task::isCompleted() const
{
    using StepType = typename std::underlying_type<ParserStep>::type;
    return ( static_cast<StepType>( m_step ) &
             static_cast<StepType>( ParserStep::Completed ) ) ==
             static_cast<StepType>( ParserStep::Completed );
}

bool Task::isStepCompleted( Task::ParserStep step ) const
{
    return ( static_cast<uint8_t>( m_step ) & static_cast<uint8_t>( step ) ) != 0;
}

void Task::startParserStep()
{
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "retry_count = retry_count + 1 WHERE id_task = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
}

bool Task::updateFileId()
{
    assert( m_fileId == 0 );
    assert( file != nullptr && file->id() != 0 );
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "file_id = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, file->id(), m_id ) == false )
        return false;
    m_fileId = file->id();
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

Task::Item::Item( std::string mrl )
    : m_mrl( std::move( mrl ) )
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

const std::vector<Task::Item>& Task::Item::subItems() const
{
    return m_subItems;
}

void Task::Item::addSubItem( Task::Item item )
{
    m_subItems.emplace_back( std::move( item ) );
}

int64_t Task::Item::duration() const
{
    return m_duration;
}

void Task::Item::setDuration( int64_t duration )
{
    m_duration = duration;
}

bool Task::restoreLinkedEntities()
{
    LOG_INFO("Restoring linked entities of task ", m_id);
    // MRL will be empty if the task has been resumed from unparsed files
    // parentFolderId == 0 indicates an external file
    if ( m_item.mrl().empty() == true && m_parentFolderId == 0 )
    {
        LOG_WARN( "Aborting & removing external file task (#", m_id, ')' );
        destroy( m_ml, m_id );
        return false;
    }
    // First of all, we need to know if the file has been created already
    // ie. have we run the MetadataParser service, at least partially
    if ( m_fileId != 0 )
        file = File::fetch( m_ml, m_fileId );

    // We might re-create tasks without mrl to ease the handling of files on
    // external storage.
    if ( m_item.mrl().empty() == true )
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
    auto fsFactory = m_ml->fsFactoryForMrl( m_item.mrl() );
    if ( fsFactory == nullptr )
        return false;

    try
    {
        parentFolderFs = fsFactory->createDirectory( utils::file::directory( m_item.mrl() ) );
    }
    catch ( const std::system_error& ex )
    {
        LOG_ERROR( "Failed to restore task: ", ex.what() );
        return false;
    }

    try
    {
        auto files = parentFolderFs->files();
        auto it = std::find_if( begin( files ), end( files ), [this]( std::shared_ptr<fs::IFile> f ) {
            return f->mrl() == m_item.mrl();
        });
        if ( it == end( files ) )
        {
            LOG_ERROR( "Failed to restore fs::IFile associated with ", m_item.mrl() );
            return false;
        }
        fileFs = *it;
    }
    catch ( const std::system_error& ex )
    {
        // If we never found the file yet, we can delete the task. It will be
        // recreated upon next discovery
        if ( file == nullptr )
        {
            LOG_WARN( "Failed to restore file system instances for mrl ", m_item.mrl(), "."
                      " Removing the task until it gets detected again." );
            destroy( m_ml, m_id );
        }
        else
        {
            // Otherwise we need to postpone it, although most likely we will
            // detect that the file is now missing, and we won't try to restore
            // this task until it comes back (since the task restoration request
            // includes the file.is_present flag)
            LOG_WARN( "Failed to restore file system instances for mrl ", m_item.mrl(), "."
                      " Postponing the task." );
        }
        return false;
    }

    if ( file != nullptr )
        media = file->media();

    parentFolder = Folder::fetch( m_ml, m_parentFolderId );
    if ( m_parentPlaylistId != 0 )
        parentPlaylist = Playlist::fetch( m_ml, m_parentPlaylistId );

    return true;
}

void Task::setMrl( std::string newMrl )
{
    if ( m_item.mrl() == newMrl )
        return;
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "mrl = ? WHERE id_task = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, newMrl, m_id ) == false )
        return;
    m_item = Item{ std::move( newMrl ) };
}

void Task::createTable( sqlite::Connection* dbConnection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::TaskTable::Name + "("
        "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
        "step INTEGER NOT NULL DEFAULT 0,"
        "retry_count INTEGER NOT NULL DEFAULT 0,"
        "mrl TEXT,"
        "file_id UNSIGNED INTEGER,"
        "parent_folder_id UNSIGNED INTEGER,"
        "parent_playlist_id INTEGER,"
        "parent_playlist_index UNSIGNED INTEGER,"
        "UNIQUE(mrl, parent_playlist_id) ON CONFLICT FAIL,"
        "FOREIGN KEY (parent_folder_id) REFERENCES " + policy::FolderTable::Name
        + "(id_folder) ON DELETE CASCADE,"
        "FOREIGN KEY (file_id) REFERENCES " + policy::FileTable::Name
        + "(id_file) ON DELETE CASCADE,"
        "FOREIGN KEY (parent_playlist_id) REFERENCES " + policy::PlaylistTable::Name
        + "(id_playlist) ON DELETE CASCADE"
    ")";
    sqlite::Tools::executeRequest( dbConnection, req );
}

void Task::resetRetryCount( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "retry_count = 0 WHERE step & ? != ?";
    sqlite::Tools::executeUpdate( ml->getConn(), req, parser::Task::ParserStep::Completed,
                                  parser::Task::ParserStep::Completed);
}

void Task::resetParsing( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "retry_count = 0, step = ?";
    sqlite::Tools::executeUpdate( ml->getConn(), req, parser::Task::ParserStep::None );
}

std::vector<std::shared_ptr<Task>> Task::fetchUncompleted( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::TaskTable::Name + " t"
        " LEFT JOIN " + policy::FileTable::Name + " f ON f.id_file = t.file_id"
        " WHERE step & ? != ? AND retry_count < 3 AND (f.is_present != 0 OR "
        " t.file_id IS NULL)";
    return Task::fetchAll<Task>( ml, req, parser::Task::ParserStep::Completed,
                                 parser::Task::ParserStep::Completed );
}

std::shared_ptr<Task>
Task::create( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> parentFolder, std::shared_ptr<fs::IDirectory> parentFolderFs,
              std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist )
{
    std::shared_ptr<Task> self = std::make_shared<Task>( ml, std::move( fileFs ),
        std::move( parentFolder ), std::move( parentFolderFs ),
        std::move( parentPlaylist.first ), parentPlaylist.second );
    const std::string req = "INSERT INTO " + policy::TaskTable::Name +
        "(mrl, parent_folder_id, parent_playlist_id, parent_playlist_index) "
        "VALUES(?, ?, ?, ?)";
    if ( insert( ml, self, req, self->m_item.mrl(), self->parentFolder->id(), sqlite::ForeignKey(
                 self->parentPlaylist ? self->parentPlaylist->id() : 0 ),
                 self->parentPlaylistIndex ) == false )
        return nullptr;
    return self;
}

void Task::recoverUnscannedFiles( MediaLibraryPtr ml )
{
    static const std::string req = "INSERT INTO " + policy::TaskTable::Name +
            "(file_id, parent_folder_id)"
            " SELECT id_file, folder_id FROM " + policy::FileTable::Name +
            " f LEFT JOIN " + policy::TaskTable::Name + " t"
            " ON t.file_id = f.id_file WHERE t.file_id IS NULL"
            " AND f.folder_id IS NOT NULL";
    sqlite::Tools::executeInsert( ml->getConn(), req );
}

}

}
