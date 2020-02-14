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

#pragma once

#include "compat/Mutex.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/Types.h"
#include "Types.h"
#include "medialibrary/IDeviceLister.h"

#include <string>
#include <unordered_map>

namespace medialibrary
{

namespace factory
{

class FileSystemFactory : public fs::IFileSystemFactory, public IDeviceListerCb
{
    // UUID -> Device instance map
    using DeviceCacheMap = std::unordered_map<std::string, std::shared_ptr<fs::IDevice>>;

public:
    explicit FileSystemFactory( MediaLibraryPtr ml );
    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& mrl ) override;
    virtual std::shared_ptr<fs::IFile> createFile( const std::string& mrl ) override;
    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override;
    virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& mrl ) override;
    virtual void refreshDevices() override;
    virtual bool isMrlSupported( const std::string& path ) const override;
    virtual bool isNetworkFileSystem() const override;
    virtual const std::string& scheme() const override;
    virtual bool start( fs::IFileSystemFactoryCb* cb ) override;
    virtual void stop() override;

private:
    virtual bool onDeviceMounted( const std::string& uuid,
                                  const std::string& mountpoint,
                                  bool removable ) override;
    virtual void onDeviceUnmounted( const std::string& uuid,
                                    const std::string& mountpoint ) override;

private:
    DeviceListerPtr m_deviceLister;
    DeviceCacheMap m_deviceCache;
    fs::IFileSystemFactoryCb* m_cb;
};

}

}
