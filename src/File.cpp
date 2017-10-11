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
    bool dummyNbRetries;
    row >> m_id
        >> m_mediaId
        >> m_mrl
        >> m_type
        >> m_lastModificationDate
        >> m_size
        >> m_parserSteps
        >> dummyNbRetries
        >> m_folderId
        >> m_isPresent
        >> m_isRemovable
        >> m_isExternal;
}

File::File( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& file, int64_t folderId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_mrl( isRemovable == true ? file.name() : file.mrl() )
    , m_type( type )
    , m_lastModificationDate( file.lastModificationDate() )
    , m_size( file.size() )
    , m_parserSteps( ParserStep::None )
    , m_folderId( folderId )
    , m_isPresent( true )
    , m_isRemovable( isRemovable )
    , m_isExternal( false )
{
}

File::File(MediaLibraryPtr ml, int64_t mediaId, IFile::Type type, const std::string& mrl )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_mrl( mrl )
    , m_type( type )
    , m_lastModificationDate( 0 )
    , m_size( 0 )
    , m_parserSteps( ParserStep::Completed )
    , m_folderId( 0 )
    , m_isPresent( true )
    , m_isRemovable( false )
    , m_isExternal( true )
    , m_fullPath( mrl )
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
    m_fullPath = folder->mrl() + m_mrl;
    return m_fullPath;
}

IFile::Type File::type() const
{
    return m_type;
}

unsigned int File::lastModificationDate() const
{
    return static_cast<unsigned int>( m_lastModificationDate );
}

unsigned int File::size() const
{
    return m_size;
}

bool File::isExternal() const
{
    return m_isExternal;
}

void File::markStepCompleted( ParserStep step )
{
    m_parserSteps = static_cast<ParserStep>( static_cast<uint8_t>( m_parserSteps ) |
                                             static_cast<uint8_t>( step ) );
}

void File::markStepUncompleted(File::ParserStep step)
{
    m_parserSteps = static_cast<ParserStep>( static_cast<uint8_t>( m_parserSteps ) &
                                             ( ~ static_cast<uint8_t>( step ) ) );
}

bool File::saveParserStep()
{
    static const std::string req = "UPDATE " + policy::FileTable::Name + " SET parser_step = ?, "
            "parser_retries = 0 WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_parserSteps, m_id ) == false )
        return false;
    return true;
}

File::ParserStep File::parserStep() const
{
    return m_parserSteps;
}

void File::startParserStep()
{
    static const std::string req = "UPDATE " + policy::FileTable::Name + " SET "
            "parser_retries = parser_retries + 1 WHERE id_file = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_id );
}

std::shared_ptr<Media> File::media() const
{
    if ( m_mediaId == 0 )
        return nullptr;
    auto lock = m_media.lock();
    if ( m_media.isCached() == false )
    {
        auto media = Media::fetch( m_ml, m_mediaId );
        assert( isDeleted() == true || media != nullptr );
        m_media = media;
    }
    return m_media.get().lock();
}

bool File::destroy()
{
    return DatabaseHelpers::destroy( m_ml, m_id );
}

int64_t File::folderId()
{
    return m_folderId;
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
            "parser_retries INTEGER NOT NULL DEFAULT 0,"
            "folder_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "is_external BOOLEAN NOT NULL,"
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
    std::string mediaIndexReq = "CREATE INDEX IF NOT EXISTS file_media_id_index ON " +
            policy::FileTable::Name + "(media_id)";
    std::string folderIndexReq = "CREATE INDEX IF NOT EXISTS file_folder_id_index ON " +
            policy::FileTable::Name + "(folder_id)";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, mediaIndexReq ) &&
            sqlite::Tools::executeRequest( dbConnection, folderIndexReq );
}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& fileFs,
                                             int64_t folderId, bool isRemovable )
{
    auto self = std::make_shared<File>( ml, mediaId, type, fileFs, folderId, isRemovable );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(media_id, mrl, type, folder_id, last_modification_date, size, is_removable, is_external) VALUES(?, ?, ?, ?, ?, ?, ?, 0)";

    if ( insert( ml, self, req, mediaId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_size, isRemovable ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
    return self;
}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId, IFile::Type type,
                                             const std::string& mrl )
{
    // Sqlite won't ensure uniqueness for (folder_id, mrl) when folder_id is null, so we have to ensure
    // of it ourselves
    static const std::string existingReq = "SELECT * FROM " + policy::FileTable::Name +
            " WHERE folder_id IS NULL AND mrl = ?";
    auto existing = fetch( ml, existingReq, mrl );
    if ( existing != nullptr )
        return nullptr;

    auto self = std::make_shared<File>( ml, mediaId, type, mrl );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(media_id, mrl, type, folder_id, is_removable, is_external) VALUES(?, ?, ?, NULL, 0, 1)";

    if ( insert( ml, self, req, mediaId, mrl, type ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<File> File::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +  " WHERE mrl = ? AND folder_id IS NOT NULL";
    auto file = fetch( ml, req, mrl );
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

std::shared_ptr<File> File::fromExternalMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +  " WHERE mrl = ? "
            "AND folder_id IS NULL";
    auto file = fetch( ml, req, mrl );
    if ( file == nullptr )
        return nullptr;
    assert( file->m_isExternal == true );
    return file;
}

std::vector<std::shared_ptr<File>> File::fetchUnparsed( MediaLibraryPtr ml )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE parser_step != ? AND is_present = 1 AND folder_id IS NOT NULL AND parser_retries < 3";
    return File::fetchAll<File>( ml, req, File::ParserStep::Completed );
}

void File::resetRetryCount( MediaLibraryPtr ml )
{
    static const std::string req = "UPDATE " + policy::FileTable::Name + " SET "
            "parser_retries = 0 WHERE parser_step != ? AND is_present = 1 AND folder_id IS NOT NULL";
    sqlite::Tools::executeUpdate( ml->getConn(), req, ParserStep::Completed );
}

}
