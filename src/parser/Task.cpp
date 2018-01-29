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
    row >> m_id
        >> m_step
        >> m_retryCount
        >> mrl
        >> m_fileId
        >> m_parentFolderId
        >> m_parentPlaylistId
        >> parentPlaylistIndex;
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
    , mrl( this->fileFs->mrl() )
    , currentService( 0 )
    , m_ml( ml )
    , m_step( ParserStep::None )
{
}

void Task::markStepCompleted( ParserStep stepCompleted )
{
    m_step = static_cast<ParserStep>( static_cast<uint8_t>( m_step ) |
                                      static_cast<uint8_t>( stepCompleted ) );
}

void Task::markStepUncompleted( ParserStep stepUncompleted )
{
    m_step = static_cast<ParserStep>( static_cast<uint8_t>( m_step ) &
                                      ( ~ static_cast<uint8_t>( stepUncompleted ) ) );
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
    return m_step == ParserStep::Completed;
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

bool Task::restoreLinkedEntities( )
{
    auto fsFactory = m_ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
        return false;

    parentFolderFs = fsFactory->createDirectory( utils::file::directory( mrl ) );
    if ( parentFolderFs == nullptr )
        return false;
    auto files = parentFolderFs->files();
    auto it = std::find_if( begin( files ), end( files ), [this]( std::shared_ptr<fs::IFile> f ) {
        return f->mrl() == mrl;
    });
    if ( it == end( files ) )
    {
        LOG_ERROR( "Failed to restore fs::IFile associated with ", mrl );
        return false;
    }
    fileFs = *it;

    file = File::fetch( m_ml, m_fileId );
    if ( file != nullptr )
        media = file->media();
    parentFolder = Folder::fetch( m_ml, m_parentFolderId );
    if ( m_parentPlaylistId != 0 )
        parentPlaylist = Playlist::fetch( m_ml, m_parentPlaylistId );

    return true;
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
            "retry_count = 0 WHERE step != ? AND is_present != 0";
    sqlite::Tools::executeUpdate( ml->getConn(), req, parser::Task::ParserStep::Completed );
}

void Task::resetParsing( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + policy::TaskTable::Name + " SET "
            "retry_count = 0, step = ?";
    sqlite::Tools::executeUpdate( ml->getConn(), req, parser::Task::ParserStep::None );
}

std::vector<std::shared_ptr<Task>> Task::fetchUnparsed( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::TaskTable::Name + " t"
        " LEFT JOIN " + policy::FileTable::Name + " f ON f.id_file = t.file_id"
        " WHERE step != ? AND retry_count < 3 AND (f.is_present != 0 OR "
        " t.file_id IS NULL)";
    return Task::fetchAll<Task>( ml, req, parser::Task::ParserStep::Completed );
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
    if ( insert( ml, self, req, self->mrl, self->parentFolder->id(), sqlite::ForeignKey(
                 self->parentPlaylist ? self->parentPlaylist->id() : 0 ),
                 self->parentPlaylistIndex ) == false )
        return nullptr;
    return self;
}

}

}
