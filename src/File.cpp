/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "File.h"

#include "Media.h"
#include "Folder.h"

namespace medialibrary
{

const std::string policy::FileTable::Name = "File";
const std::string policy::FileTable::PrimaryKeyColumn = "id_file";
int64_t File::* const policy::FileTable::PrimaryKey = &File::m_id;

File::File( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    row >> m_id
        >> m_mediaId
        >> m_mrl
        >> m_type
        >> m_lastModificationDate
        >> m_size
        >> m_parserSteps
        >> m_folderId
        >> m_isPresent
        >> m_isRemovable;
}

File::File( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& file, int64_t folderId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_mrl( isRemovable == true ? file.name() : file.fullPath() )
    , m_type( type )
    , m_lastModificationDate( file.lastModificationDate() )
    , m_size( file.size() )
    , m_parserSteps( ParserStep::None )
    , m_folderId( folderId )
    , m_isPresent( true )
    , m_isRemovable( isRemovable )
{
}

int64_t File::id() const
{
    return m_id;
}

const std::string& File::mrl() const
{
    if ( m_isRemovable == false )
        return m_mrl;

    auto lock = m_fullPath.lock();
    if ( m_fullPath.isCached() )
        return m_fullPath;
    auto folder = Folder::fetch( m_ml, m_folderId );
    if ( folder == nullptr )
        return m_mrl;
    m_fullPath = folder->path() + m_mrl;
    return m_fullPath;
}

IFile::Type File::type() const
{
    return m_type;
}

unsigned int File::lastModificationDate() const
{
    return m_lastModificationDate;
}

unsigned int File::size() const
{
    return m_size;
}

void File::markStepCompleted( ParserStep step )
{
    m_parserSteps = static_cast<ParserStep>( static_cast<uint8_t>( m_parserSteps ) |
                                             static_cast<uint8_t>( step ) );
}

bool File::saveParserStep()
{
    static const std::string req = "UPDATE " + policy::FileTable::Name + " SET parser_step = ? WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_parserSteps, m_id ) == false )
        return false;
    return true;
}

File::ParserStep File::parserStep() const
{
    return m_parserSteps;
}

std::shared_ptr<Media> File::media() const
{
    auto lock = m_media.lock();
    if ( m_media.isCached() == false )
    {
        m_media = Media::fetch( m_ml, m_mediaId );
    }
    return m_media.get().lock();
}

bool File::destroy()
{
    return DatabaseHelpers::destroy( m_ml, m_id );
}

bool File::createTable( DBConnection dbConnection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FileTable::Name + "("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "media_id INT NOT NULL,"
            "mrl TEXT,"
            "type UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INT,"
            "size UNSIGNED INT,"
            "parser_step INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name
            + "(id_media) ON DELETE CASCADE,"
            "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name
            + "(id_folder) ON DELETE CASCADE,"
            "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL"
        ")";
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_folder_present AFTER UPDATE OF is_present ON "
            + policy::FolderTable::Name +
            " BEGIN"
            " UPDATE " + policy::FileTable::Name + " SET is_present = new.is_present WHERE folder_id = new.id_folder;"
            " END";
    std::string indexReq = "CREATE INDEX IF NOT EXISTS file_media_id_index ON " +
            policy::FileTable::Name + "(media_id)";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, indexReq );
}

std::shared_ptr<File> File::create( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& fileFs, int64_t folderId, bool isRemovable )
{
    auto self = std::make_shared<File>( ml, mediaId, type, fileFs, folderId, isRemovable );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(media_id, mrl, type, folder_id, last_modification_date, size, is_removable) VALUES(?, ?, ?, ?, ?, ?, ?)";

    if ( insert( ml, self, req, mediaId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_size, isRemovable ) == false )
        return nullptr;
    self->m_fullPath = fileFs.fullPath();
    return self;
}

std::shared_ptr<File> File::fromPath( MediaLibraryPtr ml, const std::string& path )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +  " WHERE mrl = ?";
    auto file = fetch( ml, req, path );
    if ( file == nullptr )
        return nullptr;
    // safety checks: since this only works for files on non removable devices, isPresent must be true
    // and isRemovable must be false
    assert( file->m_isPresent == true );
    assert( file->m_isRemovable == false );
    return file;
}

std::shared_ptr<File> File::fromFileName( MediaLibraryPtr ml, const std::string& fileName, int64_t folderId )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +  " WHERE mrl = ? "
            "AND folder_id = ?";
    auto file = fetch( ml, req, fileName, folderId );
    if ( file == nullptr )
        return nullptr;
    assert( file->m_isRemovable == true );
    return file;
}

}
