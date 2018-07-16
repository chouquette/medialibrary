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


MediaLibraryTester::MediaLibraryTester()
    : dummyDirectory( new mock::NoopDirectory )
    , dummyFolder( std::make_shared<Folder>( nullptr, "./", 0, 0, false ) )
{
}

std::shared_ptr<Media> MediaLibraryTester::media( int64_t id )
{
    return std::static_pointer_cast<Media>( MediaLibrary::media( id ) );
}

std::shared_ptr<Folder> MediaLibraryTester::folder( const std::string& mrl )
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE is_blacklisted = 0 AND is_present != 0";
    auto folders = Folder::DatabaseHelpers::fetchAll<Folder>( this, req );
    for ( auto &f : folders )
    {
        if ( f->mrl() == mrl )
            return f;
    }
    return nullptr;
}

std::shared_ptr<Media> MediaLibraryTester::addFile( const std::string& path )
{
    return addFile( std::make_shared<mock::NoopFile>( path ),
                    dummyFolder, dummyDirectory );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( std::shared_ptr<fs::IFile> file )
{
    return addFile( std::move( file ), dummyFolder, dummyDirectory );
}

std::shared_ptr<Media> MediaLibraryTester::addFile( std::shared_ptr<fs::IFile> fileFs,
                                              std::shared_ptr<Folder> parentFolder,
                                              std::shared_ptr<fs::IDirectory> parentFolderFs )
{
    LOG_INFO( "Adding ", fileFs->mrl() );
    auto mptr = Media::create( this, IMedia::Type::Unknown,
                               utils::file::stripExtension( fileFs->name() ) );
    if ( mptr == nullptr )
    {
        LOG_ERROR( "Failed to add media ", fileFs->mrl(), " to the media library" );
        return nullptr;
    }
    // For now, assume all media are made of a single file
    auto file = mptr->addFile( *fileFs, parentFolder->id(), parentFolderFs->device()->isRemovable(), File::Type::Main );
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

std::shared_ptr<Playlist> MediaLibraryTester::playlist( int64_t playlistId )
{
    return Playlist::fetch( this, playlistId );
}

void MediaLibraryTester::deleteAlbum( int64_t albumId )
{
    Album::destroy( this, albumId );
}

std::shared_ptr<Album> MediaLibraryTester::createAlbum( const std::string& title )
{
    return MediaLibrary::createAlbum( title, 0 );
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
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE is_present != 0";
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

void MediaLibraryTester::addDiscoveredFile(std::shared_ptr<fs::IFile> fileFs,
                                std::shared_ptr<Folder> parentFolder,
                                std::shared_ptr<fs::IDirectory> parentFolderFs,
                                std::pair<std::shared_ptr<Playlist>, unsigned int>)
{
    addFile( fileFs, parentFolder, parentFolderFs );
}

sqlite::Connection* MediaLibraryTester::getDbConn()
{
    return m_dbConnection.get();
}

void MediaLibraryTester::startThumbnailer()
{
}

void MediaLibraryTester::populateFsFactories()
{
}

MediaPtr MediaLibraryTester::addMedia( const std::string& mrl )
{
    return addExternalMedia( mrl );
}

void MediaLibraryTester::deleteMedia( int64_t mediaId )
{
    Media::destroy( this, mediaId );
}

void MediaLibraryTester::outdateAllDevices()
{
    std::string req = "UPDATE " + policy::DeviceTable::Name + " SET last_seen = 1";
    sqlite::Tools::executeUpdate( getConn(), req );
}

void MediaLibraryTester::outdateAllExternalMedia()
{
    std::string req = "UPDATE " + policy::MediaTable::Name + " SET real_last_played_date = 1 "
            "WHERE type = ? OR type = ?";
    sqlite::Tools::executeUpdate( getConn(), req, IMedia::Type::External, IMedia::Type::Stream );
}
