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
#include "Playlist.h"

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
        >> m_playlistId
        >> m_mrl
        >> m_type
        >> m_lastModificationDate
        >> m_size
        >> m_folderId
        >> m_isPresent
        >> m_isRemovable
        >> m_isExternal;
}

File::File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type, const fs::IFile& file, int64_t folderId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    , m_mrl( isRemovable == true ? file.name() : file.mrl() )
    , m_type( type )
    , m_lastModificationDate( file.lastModificationDate() )
    , m_size( file.size() )
    , m_folderId( folderId )
    , m_isPresent( true )
    , m_isRemovable( isRemovable )
    , m_isExternal( false )
{
    assert( ( mediaId == 0 && playlistId != 0 ) || ( mediaId != 0 && playlistId == 0 ) );
}

File::File(MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, IFile::Type type, const std::string& mrl )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    , m_mrl( mrl )
    , m_type( type )
    , m_lastModificationDate( 0 )
    , m_size( 0 )
    , m_folderId( 0 )
    , m_isPresent( true )
    , m_isRemovable( false )
    , m_isExternal( true )
    , m_fullPath( mrl )
{
    assert( ( mediaId == 0 && playlistId != 0 ) || ( mediaId != 0 && playlistId == 0 ) );
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

void File::setMrl( const std::string& mrl )
{
    if ( m_mrl == mrl )
        return;
    const static std::string req = "UPDATE " + policy::FileTable::Name + " SET "
            "mrl = ? WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, mrl, m_id ) == false )
        return;
    m_mrl = mrl;
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

void File::createTable( sqlite::Connection* dbConnection )
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FileTable::Name + "("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "media_id UNSIGNED INT DEFAULT NULL,"
            "playlist_id UNSIGNED INT DEFAULT NULL,"
            "mrl TEXT,"
            "type UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INT,"
            "size UNSIGNED INT,"
            "folder_id UNSIGNED INTEGER,"
            "is_present BOOLEAN NOT NULL DEFAULT 1,"
            "is_removable BOOLEAN NOT NULL,"
            "is_external BOOLEAN NOT NULL,"
            "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name
            + "(id_media) ON DELETE CASCADE,"
            "FOREIGN KEY (playlist_id) REFERENCES " + policy::PlaylistTable::Name
            + "(id_playlist) ON DELETE CASCADE,"
            "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name
            + "(id_folder) ON DELETE CASCADE,"
            "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL"
        ")";

    sqlite::Tools::executeRequest( dbConnection, req );
}

void File::createTriggers(sqlite::Connection* dbConnection)
{
    std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_folder_present AFTER UPDATE OF is_present ON "
            + policy::FolderTable::Name +
            " BEGIN"
            " UPDATE " + policy::FileTable::Name + " SET is_present = new.is_present WHERE folder_id = new.id_folder;"
            " END";
    std::string mediaIndexReq = "CREATE INDEX IF NOT EXISTS file_media_id_index ON " +
            policy::FileTable::Name + "(media_id)";
    std::string folderIndexReq = "CREATE INDEX IF NOT EXISTS file_folder_id_index ON " +
            policy::FileTable::Name + "(folder_id)";
    sqlite::Tools::executeRequest( dbConnection, triggerReq );
    sqlite::Tools::executeRequest( dbConnection, mediaIndexReq );
    sqlite::Tools::executeRequest( dbConnection, folderIndexReq );
}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& fileFs,
                                             int64_t folderId, bool isRemovable )
{
    assert( mediaId > 0 );
    auto self = std::make_shared<File>( ml, mediaId, 0, type, fileFs, folderId, isRemovable );
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
    assert( mediaId > 0 );
    // Sqlite won't ensure uniqueness for (folder_id, mrl) when folder_id is null, so we have to ensure
    // of it ourselves
    static const std::string existingReq = "SELECT * FROM " + policy::FileTable::Name +
            " WHERE folder_id IS NULL AND mrl = ?";
    auto existing = fetch( ml, existingReq, mrl );
    if ( existing != nullptr )
        return nullptr;

    auto self = std::make_shared<File>( ml, mediaId, 0, type, mrl );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(media_id, mrl, type, folder_id, is_removable, is_external) VALUES(?, ?, ?, NULL, 0, 1)";

    if ( insert( ml, self, req, mediaId, mrl, type ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<File> File::createFromPlaylist( MediaLibraryPtr ml, int64_t playlistId, const fs::IFile& fileFs,
                                                int64_t folderId, bool isRemovable )
{
    assert( playlistId > 0 );
    const auto type = IFile::Type::Playlist;
    auto self = std::make_shared<File>( ml, 0, playlistId, type , fileFs, folderId, isRemovable );
    static const std::string req = "INSERT INTO " + policy::FileTable::Name +
            "(playlist_id, mrl, type, folder_id, last_modification_date, size, is_removable, is_external) VALUES(?, ?, ?, ?, ?, ?, ?, 0)";

    if ( insert( ml, self, req, playlistId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                 self->m_lastModificationDate, self->m_size, isRemovable ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
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


}
