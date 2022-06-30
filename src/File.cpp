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

#include "File.h"

#include "Media.h"
#include "Folder.h"
#include "Playlist.h"
#include "Subscription.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/Errors.h"

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
    , m_isNetwork( row.extract<decltype(m_isNetwork)>() )
    , m_subscriptionId( row.hasRemainingColumns() ? row.extract<decltype(m_subscriptionId)>() : 0 )
    , m_insertionDate( row.hasRemainingColumns() ? row.extract<decltype(m_insertionDate)>() : 0 )
{
    assert( row.hasRemainingColumns() == false );
}

File::File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type,
            const fs::IFile& file, int64_t folderId, bool isRemovable,
            time_t insertionDate )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    /*
     * We don't expect a subscription with an actual file on the file system
     * at least for now.
     */
    , m_mrl( isRemovable == true ? file.name() : file.mrl() )
    , m_type( type )
    , m_lastModificationDate( file.lastModificationDate() )
    , m_size( file.size() )
    , m_folderId( folderId )
    , m_isRemovable( isRemovable )
    , m_isExternal( false )
    , m_isNetwork( file.isNetwork() )
    , m_subscriptionId( 0 )
    , m_insertionDate( insertionDate )
{
    assert( ( mediaId == 0 && playlistId != 0 ) || ( mediaId != 0 && playlistId == 0 ) );
}

File::File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId,
            int64_t subscriptionId, IFile::Type type, const std::string& mrl,
            int64_t fileSize, time_t insertionDate )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_playlistId( playlistId )
    , m_mrl( mrl )
    , m_type( type )
    , m_lastModificationDate( 0 )
    , m_size( fileSize )
    , m_folderId( 0 )
    , m_isRemovable( false )
    , m_isExternal( true )
    , m_isNetwork( utils::url::schemeIs( "file://", mrl ) == false )
    , m_subscriptionId( subscriptionId )
    , m_insertionDate( insertionDate )
    , m_fullPath( mrl )
{
    assert( ( mediaId == 0 && playlistId != 0 && subscriptionId == 0 ) ||
            ( mediaId != 0 && playlistId == 0 && subscriptionId == 0 ) ||
            ( mediaId == 0 && playlistId == 0 && subscriptionId != 0 ) );
}

int64_t File::id() const
{
    return m_id;
}

const std::string& File::mrl() const
{
    if ( m_isRemovable == false )
        return m_mrl;

    // If the file is removable, then it needs to have a parent folder
    assert( m_folderId != 0 );

    if ( m_fullPath.empty() == false )
        return m_fullPath;
    auto folder = Folder::fetch( m_ml, m_folderId );
    if ( folder == nullptr )
    {
        assert( !"Can't find the folder for an existing file" );
        return m_mrl;
    }
    m_fullPath = folder->mrl() + m_mrl;
    return m_fullPath;
}

const std::string& File::rawMrl() const
{
    return m_mrl;
}

void File::setMrl( std::string mrl )
{
    if ( m_mrl == mrl )
        return;
    if ( setMrl( m_ml, mrl, m_id ) == false )
        return;
    m_mrl = std::move( mrl );
}

bool File::setMrl( MediaLibraryPtr ml, const std::string& mrl, int64_t fileId )
{
    const static std::string req = "UPDATE " + File::Table::Name + " SET "
            "mrl = ? WHERE id_file = ?";
    return sqlite::Tools::executeUpdate( ml->getConn(), req, mrl, fileId );
}

IFile::Type File::type() const
{
    return m_type;
}

time_t File::lastModificationDate() const
{
    return m_lastModificationDate;
}

uint64_t File::size() const
{
    return m_size;
}

bool File::isExternal() const
{
    return m_isExternal;
}

bool File::updateFsInfo( time_t newLastModificationDate, uint64_t newSize )
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

bool File::isNetwork() const
{
    return m_isNetwork;
}

bool File::isMain() const
{
    return m_type == Type::Main ||
           m_type == Type::Cache;
}

time_t File::insertionDate() const
{
    return m_insertionDate;
}

std::shared_ptr<Media> File::media() const
{
    if ( m_mediaId == 0 )
        return nullptr;
    auto media = m_media.lock();
    if ( media == nullptr )
    {
        media = Media::fetch( m_ml, m_mediaId );
        m_media = media;
    }
    return media;
}

int64_t File::mediaId() const
{
    return m_mediaId;
}

bool File::setMediaId( int64_t mediaId )
{
    if ( mediaId == m_mediaId )
        return true;
    const std::string req = "UPDATE " + Table::Name + " SET media_id = ? "
            "WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, mediaId, m_id ) == false )
        return false;
    m_mediaId = mediaId;
    return true;
}

bool File::setPlaylistId( int64_t playlistId )
{
    if ( playlistId == m_playlistId )
        return true;
    const std::string req = "UPDATE " + Table::Name + " SET media_id = NULL "
            ", playlist_id = ? WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, playlistId, m_id ) == false )
        return false;
    m_playlistId = playlistId;
    return true;

}

bool File::destroy()
{
    return DatabaseHelpers::destroy( m_ml, m_id );
}

int64_t File::folderId() const
{
    return m_folderId;
}

bool File::update( const fs::IFile& fileFs, int64_t folderId, bool isRemovable )
{
    const std::string req = "UPDATE " + Table::Name + " SET "
        "mrl = ?, last_modification_date = ?, size = ?, folder_id = ?, "
        "is_removable = ?, is_external = ?, is_network = ? WHERE id_file = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
            isRemovable == true ? fileFs.name() : fileFs.mrl(),
            fileFs.lastModificationDate(), fileFs.size(), folderId, isRemovable,
            false, fileFs.isNetwork(), m_id ) == false )
        return false;
    m_mrl = isRemovable == true ? fileFs.name() : fileFs.mrl();
    m_fullPath = fileFs.mrl();
    m_lastModificationDate = fileFs.lastModificationDate();
    m_size = fileFs.size();
    m_folderId = folderId;
    m_isRemovable = isRemovable;
    m_isExternal = false;
    m_isNetwork = fileFs.isNetwork();
    return true;
}

bool File::convertToExternal()
{
    const std::string req = "UPDATE " + Table::Name + " SET "
        "mrl = ?, folder_id = NULL, is_removable = 0, is_external = 1 WHERE id_file = ?";
    return sqlite::Tools::executeUpdate( m_ml->getConn(), req, mrl(), m_id );
}

FilePtr File::cache( const std::string& mrl )
{
    if ( utils::url::schemeIs( "file://", mrl ) == false )
        return nullptr;
    LOG_DEBUG( "Marking ", mrl, " as a cached MRL for file #", m_id );
    return File::createFromExternalMedia( m_ml, m_mediaId, Type::Cache, mrl,
                                          m_size, time(nullptr) );
}

void File::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   schema( Table::Name, Settings::DbModelVersion ) );
}

void File::createIndexes( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::MediaId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::FolderId, Settings::DbModelVersion ) );
    sqlite::Tools::executeRequest( dbConnection,
                                   index( Indexes::PlaylistId, Settings::DbModelVersion ) );
}

std::string File::schema( const std::string& tableName, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( tableName );

    assert( tableName == Table::Name );
    if ( dbModel < 37 )
    {
        return "CREATE TABLE " + Table::Name +
        "("
            "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
            "media_id UNSIGNED INT DEFAULT NULL,"
            "playlist_id UNSIGNED INT DEFAULT NULL,"
            "mrl TEXT,"
            "type UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INT,"
            "size UNSIGNED INT,"
            "folder_id UNSIGNED INTEGER,"
            "is_removable BOOLEAN NOT NULL,"
            "is_external BOOLEAN NOT NULL,"
            "is_network BOOLEAN NOT NULL,"

            "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
            + "(id_media) ON DELETE CASCADE,"

            "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name
            + "(id_playlist) ON DELETE CASCADE,"

            "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
            + "(id_folder) ON DELETE CASCADE,"

            "UNIQUE(mrl,folder_id) ON CONFLICT FAIL"
        ")";
    }
    return "CREATE TABLE " + Table::Name +
    "("
        "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
        "media_id UNSIGNED INT DEFAULT NULL,"
        "playlist_id UNSIGNED INT DEFAULT NULL,"
        "mrl TEXT,"
        "type UNSIGNED INTEGER,"
        "last_modification_date UNSIGNED INT,"
        "size UNSIGNED INT,"
        "folder_id UNSIGNED INTEGER,"
        "is_removable BOOLEAN NOT NULL,"
        "is_external BOOLEAN NOT NULL,"
        "is_network BOOLEAN NOT NULL,"
        "subscription_id UNSIGNED INTEGER UNIQUE,"
        "insertion_date UNSIGNED INTEGER,"

        "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
        + "(id_media) ON DELETE CASCADE,"

        "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name
        + "(id_playlist) ON DELETE CASCADE,"

        "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name
        + "(id_folder) ON DELETE CASCADE,"

        "FOREIGN KEY(subscription_id) REFERENCES " + Subscription::Table::Name
        + "(id_subscription) ON DELETE CASCADE,"

        "UNIQUE(mrl,folder_id) ON CONFLICT FAIL"
    ")";
}

std::string File::index( Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::MediaId:
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                        Table::Name + "(media_id)";
        case Indexes::FolderId:
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                        Table::Name + "(folder_id)";
        case Indexes::PlaylistId:
            assert( dbModel >= 34 );
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                        Table::Name + "(playlist_id)";
        case Indexes::InsertionDate:
            assert( dbModel >= 37 );
            return "CREATE INDEX " + indexName( index, dbModel ) + " ON " +
                    Table::Name + "(insertion_date)";
        default:
            assert( !"Invalid index provided" );
    }
    return "<invalid request>";
}

std::string File::indexName( File::Indexes index, uint32_t dbModel )
{
    UNUSED_IN_RELEASE( dbModel );

    switch ( index )
    {
        case Indexes::MediaId:
            return "file_media_id_index";
        case Indexes::FolderId:
            return "file_folder_id_index";
        case Indexes::PlaylistId:
            assert( dbModel >= 34 );
            return "file_playlist_id_idx";
        case Indexes::InsertionDate:
            assert( dbModel >= 37 );
            return "file_insertion_date_idx";
    }
    return "<invalid trigger>";
}

bool File::checkDbModel( MediaLibraryPtr ml )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );

    return sqlite::Tools::checkTableSchema(
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name ) &&
            sqlite::Tools::checkIndexStatement(
                        index( Indexes::MediaId, Settings::DbModelVersion ),
                        indexName( Indexes::MediaId, Settings::DbModelVersion ) ) &&
            sqlite::Tools::checkIndexStatement(
                        index( Indexes::FolderId, Settings::DbModelVersion ),
                        indexName( Indexes::FolderId, Settings::DbModelVersion ) ) &&
            sqlite::Tools::checkIndexStatement(
                        index( Indexes::PlaylistId, Settings::DbModelVersion ),
                        indexName( Indexes::PlaylistId, Settings::DbModelVersion ) );

}

std::shared_ptr<File> File::createFromMedia( MediaLibraryPtr ml, int64_t mediaId,
                                             Type type, const fs::IFile& fileFs,
                                             int64_t folderId, bool isRemovable,
                                             time_t insertionDate )
{
    assert( mediaId > 0 );
    auto self = std::make_shared<File>( ml, mediaId, 0, type, fileFs, folderId,
                                        isRemovable, insertionDate );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(media_id, mrl, type, folder_id, last_modification_date, size, "
            "is_removable, is_external, is_network, insertion_date) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, 0, ?, ?)";

    if ( insert( ml, self, req, mediaId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                         self->m_lastModificationDate, self->m_size, isRemovable,
                         self->m_isNetwork, insertionDate ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
    return self;
}

std::shared_ptr<File> File::createFromExternalMedia( MediaLibraryPtr ml, int64_t mediaId,
                                                     IFile::Type type, const std::string& mrl,
                                                     int64_t fileSize, time_t insertionDate )
{
    assert( mediaId > 0 );
    // Sqlite won't ensure uniqueness for (folder_id, mrl) when folder_id is null, so we have to ensure
    // of it ourselves
    static const std::string existingReq = "SELECT * FROM " + File::Table::Name +
            " WHERE folder_id IS NULL AND mrl = ?";
    auto existing = fetch( ml, existingReq, mrl );
    if ( existing != nullptr )
        return nullptr;

    auto self = std::make_shared<File>( ml, mediaId, 0, 0, type, mrl, fileSize,
                                        insertionDate );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(media_id, mrl, type, size, folder_id, is_removable, is_external,"
            "is_network, subscription_id, insertion_date) "
            "VALUES(?, ?, ?, ?, NULL, 0, 1, ?, ?, ?)";

    if ( insert( ml, self, req, mediaId, mrl, type, fileSize,
                 self->m_isNetwork, nullptr, insertionDate ) == false )
        return nullptr;
    return self;
}

std::shared_ptr<File> File::createFromPlaylist( MediaLibraryPtr ml, int64_t playlistId,
                                                const fs::IFile& fileFs,
                                                int64_t folderId, bool isRemovable,
                                                time_t insertionDate )
{
    assert( playlistId > 0 );
    const auto type = IFile::Type::Playlist;
    auto self = std::make_shared<File>( ml, 0, playlistId, type , fileFs, folderId,
                                        isRemovable, insertionDate );
    static const std::string req = "INSERT INTO " + File::Table::Name +
            "(playlist_id, mrl, type, folder_id, last_modification_date, size, "
            "is_removable, is_external, is_network, insertion_date) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, 0, ?, ?)";

    if ( insert( ml, self, req, playlistId, self->m_mrl, type, sqlite::ForeignKey( folderId ),
                 self->m_lastModificationDate, self->m_size, isRemovable,
                 self->m_isNetwork, insertionDate ) == false )
        return nullptr;
    self->m_fullPath = fileFs.mrl();
    return self;
}

std::shared_ptr<File> File::createFromSubscription( MediaLibraryPtr ml,
                                                    std::string mrl,
                                                    int64_t subscriptionId )
{
    assert( subscriptionId > 0 );
    auto self = std::make_shared<File>( ml, 0, 0, subscriptionId,
                                        IFile::Type::Subscription, mrl, 0,
                                        time(nullptr) );
    const std::string req = "INSERT INTO " + Table::Name +
            "(mrl, type, is_removable, is_external, is_network, subscription_id) "
            "VALUES(?, ?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, self->mrl(), IFile::Type::Subscription,
                 false, true, false, subscriptionId ) == false )
        return nullptr;
    self->m_fullPath = self->mrl();
    return self;
}

bool File::exists( MediaLibraryPtr ml, const std::string& mrl )
{
    OPEN_READ_CONTEXT( ctx, ml->getConn() );
    sqlite::Statement stmt{ "SELECT EXISTS("
        "SELECT id_file FROM " + Table::Name + " WHERE mrl = ?)"
    };
    stmt.execute( mrl );
    auto row = stmt.row();
    auto res = row.extract<bool>();
    assert( stmt.row() == nullptr );
    return res;
}

std::shared_ptr<File> File::fromMrl( MediaLibraryPtr ml, const std::string& mrl )
{
    /* Be optimistic and attempt to fetch a non-removable file first */
    static const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE mrl = ? AND folder_id IS NOT NULL";
    auto file = fetch( ml, req, mrl );
    if ( file != nullptr )
    {
        // safety checks: since this only works for files on non removable devices
        // isRemovable must be false
        assert( file->m_isRemovable == false );
        return file;
    }

    /*
     * Otherwise, fallback to constructing the mrl based on the device that
     * stores it
     */
    auto folder = Folder::fromMrl( ml, utils::file::directory( mrl ) );
    if ( folder == nullptr )
    {
        LOG_DEBUG( "Failed to find folder containing ", mrl );
        return nullptr;
    }
    file = fromFileName( ml, utils::file::fileName( mrl ), folder->id() );
    if ( file == nullptr )
    {
        LOG_DEBUG( "Failed to fetch file for ", mrl );
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
    std::string scheme;
    try
    {
        scheme = utils::url::scheme( mrl );
    }
    catch ( const fs::errors::UnhandledScheme& )
    {
        return nullptr;
    }
    static const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE mrl = ? AND folder_id IS NULL";
    auto file = fetch( ml, req, mrl );
    if ( file == nullptr )
        return nullptr;
    assert( file->m_isExternal == true );
    return file;
}

std::vector<std::shared_ptr<File>> File::fromParentFolder( MediaLibraryPtr ml,
                                                           int64_t parentFolderId )
{
    static const std::string req = "SELECT * FROM " + File::Table::Name
            + " WHERE folder_id = ?";
    return File::fetchAll<File>( ml, req, parentFolderId );
}

std::vector<std::shared_ptr<File> > File::cachedFiles( MediaLibraryPtr ml )
{
    const std::string req = "SELECT * FROM " + Table::Name +
            " WHERE type = ?";
    return fetchAll<File>( ml, req, Type::Cache );
}

std::string File::cachedFileName( const File& f )
{
    return std::to_string( f.id() ) + "_" +
            utils::url::decode( utils::file::fileName( f.rawMrl() ) );
}


}
