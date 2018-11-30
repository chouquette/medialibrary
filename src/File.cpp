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
#include "utils/Filename.h"

namespace medialibrary
{

const std::string File::Table::Name = "File";
const std::string File::Table::PrimaryKeyColumn = "id_file";
int64_t File::* const File::Table::PrimaryKey = &File::m_id;

File::File( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.extract<decltype(m_id)>() )
    , m_mediaId( row.extract<decltype(m_mediaId)>() )
    , m_playlistId( row.extract<decltype(m_playlistId)>() )
    , m_mrl( row.extract<decltype(m_mrl)>() )
    , m_type( row.extract<decltype(m_type)>() )
    , m_lastModificationDate( row.extract<decltype(m_lastModificationDate)>() )
    , m_size( row.extract<decltype(m_size)>() )
    , m_folderId( row.extract<decltype(m_folderId)>() )
    , m_isRemovable( row.extract<decltype(m_isRemovable)>() )
    , m_isExternal( row.extract<decltype(m_isExternal)>() )
{
}

File::File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type,
            const fs::IFile& file, int64_t folderId, bool isRemovable )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    , m_mrl( isRemovable == true ? file.name() : file.mrl() )
    , m_type( type )
    , m_lastModificationDate( file.lastModificationDate() )
    , m_size( file.size() )
    , m_folderId( folderId )
    , m_isRemovable( isRemovable )
    , m_isExternal( false )
{
    assert( ( mediaId == 0 && playlistId != 0 ) || ( mediaId != 0 && playlistId == 0 ) );
}

File::File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, IFile::Type type,
            const std::string& mrl )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    , m_mrl( mrl )
    , m_type( type )
    , m_lastModificationDate( 0 )
    , m_size( 0 )
    , m_folderId( 0 )
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

    if ( m_fullPath.empty() == false )
        return m_fullPath;
    auto folder = Folder::fetch( m_ml, m_folderId );
    if ( folder == nullptr )
        return m_mrl;
    m_fullPath = folder->mrl() + m_mrl;
    return m_fullPath;
}

const std::string& File::rawMrl() const
{
    return m_mrl;
}

void File::setMrl( const std::string& mrl )
{
    if ( m_mrl == mrl )
        return;
    const static std::string req = "UPDATE " + File::Table::Name + " SET "
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

bool File::updateFsInfo( uint32_t newLastModificationDate, uint32_t newSize )
{
    if ( m_lastModificationDate == newLastModificationDate && m_size == newSize )
        return true;
    const std::string req = "UPDATE " + File::Table::Name +
            " SET last_modification_date = ?, size = ? WHERE id_file = ?";
    auto res = sqlite::Tools::executeUpdate( m_ml->getConn(), req,
            newLastModificationDate, newSize, m_id );
    if ( res == true )
    {
        m_lastModificationDate = newLastModificationDate;
        m_size = newSize;
    }
    return res;
}

bool File::isRemovable() const
{
    return m_isRemovable;
}

std::shared_ptr<Media> File::media() const
{
    if ( m_mediaId == 0 )
        return nullptr;
    auto media = m_media.lock();
    if ( media == nullptr )
    {
        media = Media::fetch( m_ml, m_mediaId );
        assert( isDeleted() == true || media != nullptr );
        m_media = media;
    }
    return media;
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
    std::string req {
        #include "database/tables/File_v14.sql"
    };
    sqlite::Tools::executeRequest( dbConnection, req );
}

void File::createTriggers( sqlite::Connection* dbConnection )
{
    const std::string reqs[] = {
        #include "database/tables/File_triggers_v14.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConnection, req );
}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                             Type type, const fs::IFile& fileFs,
                                             int64_t folderId, bool isRemovable )
{
    assert( mediaId > 0 );
    auto self = std::make_shared<File>( ml, mediaId, 0, type, fileFs, folderId, isRemovable );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(media_id, mrl, type, folder_id, last_modification_date, size, "
            "is_removable, is_external) VALUES(?, ?, ?, ?, ?, ?, ?, 0)";

    if ( insert( ml, self, req, mediaId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_size, isRemovable ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
    return self;
}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                             IFile::Type type, const std::string& mrl )
{
    assert( mediaId > 0 );
    // Sqlite won't ensure uniqueness for (folder_id, mrl) when folder_id is null, so we have to ensure
    // of it ourselves
    static const std::string existingReq = "SELECT * FROM " + File::Table::Name +
            " WHERE folder_id IS NULL AND mrl = ?";
    auto existing = fetch( ml, existingReq, mrl );
    if ( existing != nullptr )
        return nullptr;

    auto self = std::make_shared<File>( ml, mediaId, 0, type, mrl );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(media_id, mrl, type, folder_id, is_removable, is_external) "
            "VALUES(?, ?, ?, NULL, 0, 1)";

    if ( insert( ml, self, req, mediaId, mrl, type ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<File> File::createFromPlaylist( MediaLibraryPtr ml, int64_t playlistId,
                                                const fs::IFile& fileFs,
                                                int64_t folderId, bool isRemovable )
{
    assert( playlistId > 0 );
    const auto type = IFile::Type::Playlist;
    auto self = std::make_shared<File>( ml, 0, playlistId, type , fileFs, folderId, isRemovable );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(playlist_id, mrl, type, folder_id, last_modification_date, size, "
            "is_removable, is_external) VALUES(?, ?, ?, ?, ?, ?, ?, 0)";

    if ( insert( ml, self, req, playlistId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                 self->m_lastModificationDate, self->m_size, isRemovable ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
    return self;
}

std::shared_ptr<File> File::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    auto fsFactory = ml->fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
    {
        LOG_WARN( "Failed to create FS factory for path ", mrl );
        return nullptr;
    }
    auto device = fsFactory->createDeviceFromMrl( mrl );
    if ( device == nullptr )
    {
        LOG_WARN( "Failed to create a device associated with mrl ", mrl );
        return nullptr;
    }
    if ( device->isRemovable() == false )
    {
        static const std::string req = "SELECT * FROM " + File::Table::Name +
                " WHERE mrl = ? AND folder_id IS NOT NULL";
        auto file = fetch( ml, req, mrl );
        if ( file == nullptr )
            return nullptr;
        // safety checks: since this only works for files on non removable devices
        // isRemovable must be false
        assert( file->m_isRemovable == false );
        return file;
    }
    auto folder = Folder::fromMrl( ml, utils::file::directory( mrl ) );
    if ( folder == nullptr )
    {
        LOG_WARN( "Failed to find folder containing ", mrl );
        return nullptr;
    }
    if ( folder->isPresent() == false )
    {
        LOG_INFO( "Found a folder containing ", mrl, " but it is not present" );
        return nullptr;
    }
    auto file = fromFileName( ml, utils::file::fileName( mrl ), folder->id() );
    if ( file == nullptr )
    {
        LOG_WARN( "Failed to fetch file for ", mrl, " (device ", device->uuid(), " was ",
                  device->isRemovable() ? "" : "NOT ", "removable)");
    }
    return file;
}

std::shared_ptr<File> File::fromFileName( MediaLibraryPtr ml, const std::string& fileName,
                                          int64_t folderId )
{
    static const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE mrl = ? AND folder_id = ?";
    auto file = fetch( ml, req, fileName, folderId );
    if ( file == nullptr )
        return nullptr;
    assert( file->m_isRemovable == true );
    return file;
}

std::shared_ptr<File> File::fromExternalMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    static const std::string req = "SELECT * FROM " + File::Table::Name +  " WHERE mrl = ? "
            "AND folder_id IS NULL";
    auto file = fetch( ml, req, mrl );
    if ( file == nullptr )
        return nullptr;
    assert( file->m_isExternal == true );
    return file;
}


}
