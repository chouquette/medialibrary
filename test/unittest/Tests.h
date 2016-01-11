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

#include "gtest/gtest.h"

class Playlist;

#include "factory/IFileSystem.h"
#include "MediaLibrary.h"
#include "Folder.h"

class MediaLibraryTester : public MediaLibrary
{
public:
    MediaLibraryTester();
    std::shared_ptr<Media> media( unsigned int id );
    MediaPtr media( const std::string& path );
    std::shared_ptr<Folder> folder( const std::string& path );
    std::shared_ptr<Media> addFile( const std::string& path );
    std::shared_ptr<Playlist> playlist( unsigned int playlistId );

private:
    std::unique_ptr<fs::IDirectory> dummyDirectory;
    Folder dummyFolder;
};

class MediaLibraryWithoutParser : public MediaLibraryTester
{
    virtual void startParser() override {}
};

class MediaLibraryWithoutBackground : public MediaLibraryWithoutParser
{
    virtual void startDiscoverer() override {}
};

class Tests : public testing::Test
{
protected:
    std::unique_ptr<MediaLibraryTester> ml;

    virtual void SetUp() override;
    virtual void InstantiateMediaLibrary();
    void Reload( std::shared_ptr<factory::IFileSystem> fs = nullptr, IMediaLibraryCb* metadataCb = nullptr );
    virtual void TearDown() override;
};
