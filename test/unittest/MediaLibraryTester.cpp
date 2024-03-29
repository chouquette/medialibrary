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

#include "MediaLibraryTester.h"

#include "Album.h"
#include "Artist.h"
#include "Device.h"
#include "Playlist.h"
#include "Genre.h"
#include "Media.h"
#include "Folder.h"
#include "Show.h"
#include "mocks/FileSystem.h"
#include "parser/Task.h"


MediaLibraryTester::MediaLibraryTester( const std::string& dbPath,
                                        const std::string& mlFolderPath,
                                        const SetupConfig* cfg )
    : MediaLibrary( dbPath, mlFolderPath, nullptr, cfg )
    , dummyDevice( new mock::NoopDevice )
    , dummyDirectory( new mock::NoopDirectory )
{
}

void MediaLibraryTester::onDbConnectionReady( sqlite::Connection* dbConn )
{
    sqlite::Connection::WeakDbContext ctx{ m_dbConnection.get() };
    auto t = m_dbConnection->newTransaction();
    deleteAllTables( dbConn );
    t->commit();
    m_dbConnection->flushAll();
}

std::shared_ptr<Media> MediaLibraryTester::media( int64_t id )
{
    return std::static_pointer_cast<Media>( MediaLibrary::media( id ) );
}

FolderPtr MediaLibraryTester::folder( const std::string& mrl ) const
{
    return Folder::fromMrl( this, mrl, Folder::BannedType::No );
}

FolderPtr MediaLibraryTester::folder( int64_t id ) const
{
    return MediaLibrary::folder( id );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( const std::string& path, IMedia::Type type )
{
    return addFile( std::make_shared<mock::NoopFile>( path ),
                    dummyFolder, dummyDirectory, IFile::Type::Main, type );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( std::shared_ptr<fs::IFile> file, IMedia::Type type )
{
    return addFile( std::move( file ), dummyFolder, dummyDirectory,
                    IFile::Type::Main, type );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( std::shared_ptr<fs::IFile> fileFs,
                                              std::shared_ptr<Folder> parentFolder,
                                              std::shared_ptr<fs::IDirectory> parentFolderFs,
                                              IFile::Type fileType,
                                              IMedia::Type type )
{
    LOG_INFO( "Adding ", fileFs->mrl() );
    auto mptr = Media::create( this, type, parentFolder->deviceId(), parentFolder->id(),
                               fileFs->name(), -1 );
    if ( mptr == nullptr )
    {
        LOG_ERROR( "Failed to add media ", fileFs->mrl(), " to the media library" );
        return nullptr;
    }
    // For now, assume all media are made of a single file
    auto file = mptr->addFile( *fileFs, parentFolder->id(),
                               parentFolderFs->device()->isRemovable(), fileType );
    if ( file == nullptr )
    {
        LOG_ERROR( "Failed to add file ", fileFs->mrl(), " to media #", mptr->id() );
        Media::destroy( this, mptr->id() );
        return nullptr;
    }
    return mptr;
}

void MediaLibraryTester::addLocalFsFactory()
{
    if ( fsFactory != nullptr )
    {
        m_fsHolder.addFsFactory( fsFactory );
    }
    else
    {
        MediaLibrary::addLocalFsFactory();
    }
}

void MediaLibraryTester::deleteAlbum( int64_t albumId )
{
    Album::destroy( this, albumId );
}

std::shared_ptr<Genre> MediaLibraryTester::createGenre( const std::string& name )
{
    return Genre::create( this, name );
}

void MediaLibraryTester::deleteGenre( int64_t genreId )
{
    Genre::destroy( this, genreId );
}

void MediaLibraryTester::deleteArtist( int64_t artistId )
{
    Artist::destroy( this, artistId );
}

void MediaLibraryTester::deleteShow( int64_t showId )
{
    Show::destroy( this, showId );
}

std::shared_ptr<Device> MediaLibraryTester::addDevice( const std::string& uuid,
                                                       const std::string& scheme,
                                                       bool isRemovable )
{
    return Device::create( this, uuid, scheme, isRemovable, false );
}

void MediaLibraryTester::setFsFactory( std::shared_ptr<fs::IFileSystemFactory> fsf )
{
    fsFactory = fsf;
}

std::shared_ptr<Media> MediaLibraryTester::albumTrack( int64_t id )
{
    return Media::fetch( this, id );
}

std::vector<MediaPtr> MediaLibraryTester::files()
{
    static const std::string req = "SELECT * FROM " + Media::Table::Name + " WHERE is_present != 0";
    return Media::fetchAll<IMedia>( this, req );
}

std::shared_ptr<Device> MediaLibraryTester::device( const std::string& uuid,
                                                    const std::string& scheme )
{
    return Device::fromUuid( this, uuid, scheme );
}

void MediaLibraryTester::onDiscoveredFile(std::shared_ptr<fs::IFile> fileFs,
                                          std::shared_ptr<Folder> parentFolder,
                                          std::shared_ptr<fs::IDirectory> parentFolderFs,
                                          IFile::Type fileType)
{
    addFile( fileFs, parentFolder, parentFolderFs, fileType, IMedia::Type::Unknown );
}

void MediaLibraryTester::populateNetworkFsFactories()
{
}

MediaPtr MediaLibraryTester::addMedia( const std::string& mrl, IMedia::Type type )
{
    return addFile( mrl, type );
}

void MediaLibraryTester::deleteMedia( int64_t mediaId )
{
    Media::destroy( this, mediaId );
}

bool MediaLibraryTester::outdateAllDevices()
{
    std::string req = "UPDATE " + Device::Table::Name + " SET last_seen = 1";
    return sqlite::Tools::executeUpdate( getConn(), req );
}

bool MediaLibraryTester::setMediaInsertionDate( int64_t mediaId, time_t t )
{
    std::string req = "UPDATE " + Media::Table::Name + " SET insertion_date = ? "
            "WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, t, mediaId );
}

bool MediaLibraryTester::setMediaLastPlayedDate( int64_t mediaId, time_t lastPlayedDate )
{
    std::string req = "UPDATE " + Media::Table::Name + " SET last_played_date = ? "
            "WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, lastPlayedDate, mediaId );
}

bool MediaLibraryTester::outdateAllExternalMedia()
{
    std::string req = "UPDATE " + Media::Table::Name + " SET last_played_date = 1 "
            "WHERE import_type != ?";
    return sqlite::Tools::executeUpdate( getConn(), req, Media::ImportType::Internal );
}

bool MediaLibraryTester::setMediaType(int64_t mediaId, IMedia::Type type)
{
    std::string req = "UPDATE " + Media::Table::Name + " SET type = ? WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, type, mediaId );
}

uint32_t MediaLibraryTester::countNbThumbnails()
{
    auto ctx = getConn()->acquireReadContext();
    sqlite::Statement stmt{
        "SELECT COUNT(*) FROM " + Thumbnail::Table::Name
    };
    uint32_t res;
    stmt.execute();
    auto row = stmt.row();
    row >> res;
    return res;
}

uint32_t MediaLibraryTester::countNbTasks()
{
    auto ctx = getConn()->acquireReadContext();
    sqlite::Statement stmt{
        "SELECT COUNT(*) FROM " + parser::Task::Table::Name
    };
    uint32_t res;
    stmt.execute();
    auto row = stmt.row();
    row >> res;
    return res;
}

bool MediaLibraryTester::setupDummyFolder()
{
    // We create a dummy device in database, and a dummy folder.
    // This allows us to have a DB setup which is equivalent to an real one
    // File need to have a parent folder to be considered non-external, and a
    // folder needs to have a parent device.
    // However, if we just add a dummy device to DB and be done with it, when
    // the media library refreshes the devices, it will not find the device we
    // inserted and will mark it as missing, which will cause all its media to
    // be marked missing as well, which tends to make the tests fail
    std::shared_ptr<Device> device;
    try
    {
        device = Device::create( this, mock::FileSystemFactory::NoopDeviceUuid,
                                 "file://", false, false );
        if ( device == nullptr )
            return false;
    }
    catch ( const sqlite::errors::ConstraintUnique& )
    {
        // Most test cases call Reload() which will end up calling setupDummyFolder
        // again. We don't want the UNIQUE constraint to terminate the test.
        // Let's assume that this folder will be the first create folder
        dummyFolder = Folder::fetch( this, 1 );
        return true;
    }
    dummyFolder = Folder::create( this, "./", 0, *device, *dummyDevice );
    if ( dummyFolder == nullptr || dummyFolder->id() != 1 )
        return false;
    return true;
}

bool MediaLibraryTester::markMediaAsInternal( int64_t mediaId )
{
    static const std::string req = "UPDATE " + Media::Table::Name +
        " SET import_type = ? WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req,
                                         Media::ImportType::Internal, mediaId );
}

bool MediaLibraryTester::setMediaFolderId( int64_t mediaId, int64_t folderId )
{
    static const std::string req = "UPDATE " + Media::Table::Name +
        " SET folder_id = ? WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, folderId, mediaId );
}

void MediaLibraryTester::deleteAllTables( sqlite::Connection* dbConn )
{
    MediaLibrary::deleteAllTables( dbConn );
}

bool MediaLibraryTester::markMediaAsPublic( int64_t mediaId )
{
    static const std::string req = "UPDATE " + Media::Table::Name +
            " SET is_public = TRUE WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, mediaId );
}
