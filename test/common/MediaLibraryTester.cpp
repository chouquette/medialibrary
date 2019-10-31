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
#include "AlbumTrack.h"
#include "Artist.h"
#include "Device.h"
#include "Playlist.h"
#include "Genre.h"
#include "Media.h"
#include "Folder.h"
#include "Show.h"
#include "mocks/FileSystem.h"
#include "parser/Task.h"


MediaLibraryTester::MediaLibraryTester()
    : dummyDevice( new mock::NoopDevice )
    , dummyDirectory( new mock::NoopDirectory )
{
}

void MediaLibraryTester::removeEntryPoint( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::removeEntryPoint( entryPoint );
}

void MediaLibraryTester::reload()
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::reload();
}

void MediaLibraryTester::reload( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::reload( entryPoint );
}

void MediaLibraryTester::banFolder( const std::string& path )
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::banFolder( path );
}

void MediaLibraryTester::unbanFolder( const std::string& path )
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::unbanFolder( path );
}

void MediaLibraryTester::discover( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        MediaLibrary::discover( entryPoint );
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
        m_fsFactories.clear();
        m_fsFactories.push_back( fsFactory );
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

std::shared_ptr<Device> MediaLibraryTester::addDevice( const std::string& uuid, bool isRemovable )
{
    return Device::create( this, uuid, "file://", isRemovable );
}

void MediaLibraryTester::setFsFactory( std::shared_ptr<fs::IFileSystemFactory> fsf )
{
    fsFactory = fsf;
}

void MediaLibraryTester::deleteTrack( int64_t trackId )
{
    AlbumTrack::destroy( this, trackId );
}

std::shared_ptr<AlbumTrack> MediaLibraryTester::albumTrack( int64_t id )
{
    return AlbumTrack::fetch( this, id );
}

std::vector<MediaPtr> MediaLibraryTester::files()
{
    static const std::string req = "SELECT * FROM " + Media::Table::Name + " WHERE is_present != 0";
    return Media::fetchAll<IMedia>( this, req );
}

std::shared_ptr<Device> MediaLibraryTester::device( const std::string& uuid )
{
    return Device::fromUuid( this, uuid );
}

std::vector<const char*> MediaLibraryTester::getSupportedExtensions() const
{
    std::vector<const char*> res;
    res.reserve( NbSupportedExtensions );
    for ( auto i = 0u; i < NbSupportedExtensions; ++i )
        res.push_back( supportedExtensions[i] );
    return res;
}

void MediaLibraryTester::onDiscoveredFile(std::shared_ptr<fs::IFile> fileFs,
                                          std::shared_ptr<Folder> parentFolder,
                                          std::shared_ptr<fs::IDirectory> parentFolderFs,
                                          IFile::Type fileType,
                                          std::pair<int64_t, int64_t>)
{
    addFile( fileFs, parentFolder, parentFolderFs, fileType, IMedia::Type::Unknown );
}

void MediaLibraryTester::startThumbnailer()
{
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
    return sqlite::Tools::executeUpdate( getConn(), req, mediaId, t );
}

bool MediaLibraryTester::outdateAllExternalMedia()
{
    std::string req = "UPDATE " + Media::Table::Name + " SET real_last_played_date = 1 "
            "WHERE import_type != ?";
    return sqlite::Tools::executeUpdate( getConn(), req, Media::ImportType::Internal );
}

bool MediaLibraryTester::setMediaType(int64_t mediaId, IMedia::Type type)
{
    std::string req = "UPDATE " + Media::Table::Name + " SET type = ? WHERE id_media = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, type, mediaId );
}

bool MediaLibraryTester::setAlbumTrackGenre( int64_t albumTrackId, int64_t genreId )
{
    static const std::string req = "UPDATE " + AlbumTrack::Table::Name
            + " SET genre_id = ? WHERE id_track = ?";
    return sqlite::Tools::executeUpdate( getConn(), req, genreId, albumTrackId );
}

uint32_t MediaLibraryTester::countNbThumbnails()
{
    sqlite::Statement stmt{
        getConn()->handle(),
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
    sqlite::Statement stmt{
        getConn()->handle(),
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
                                 "file://", false );
        if ( device == nullptr )
            return false;
    }
    catch ( const sqlite::errors::ConstraintUnique& )
    {
        // Most test cases call Reload() which will end up calling setupDummyFolder
        // again. We don't want the UNIQUE constraint to terminate the test.
        device = Device::fromUuid( this, mock::FileSystemFactory::NoopDeviceUuid );
        // Let's assume that this folder will be the first create folder
        dummyFolder = Folder::fetch( this, 1 );
        return true;
    }
    dummyFolder = Folder::create( this, "./", 0, *device, *dummyDevice );
    if ( dummyFolder->id() != 1 )
        return false;
    return dummyFolder != nullptr;
}
