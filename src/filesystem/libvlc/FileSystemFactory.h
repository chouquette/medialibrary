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

#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/IDeviceLister.h"
#include "compat/Mutex.h"
#include "Types.h"

namespace medialibrary
{

namespace fs
{
namespace libvlc
{

class FileSystemFactory : public fs::IFileSystemFactory, public IDeviceListerCb
{
public:
    /**
     * @brief FileSystemFactory Constructs a filesystem factory for the given scheme
     * @param scheme The targeted scheme, including '://'
     */
    FileSystemFactory( MediaLibraryPtr ml, const std::string& scheme );
    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& mrl ) override;
    virtual std::shared_ptr<fs::IFile> createFile( const std::string& mrl ) override;
    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override;
    virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& mrl ) override;
    virtual void refreshDevices() override;
    virtual bool isMrlSupported( const std::string& mrl ) const override;
    virtual bool isNetworkFileSystem() const override;
    virtual const std::string& scheme() const override;
    virtual bool start( fs::IFileSystemFactoryCb* cb ) override;
    virtual void stop() override;
    virtual bool isStarted() const override;

private:
    virtual void onDeviceMounted( const std::string& uuid,
                                  const std::string& mountpoint, bool removable ) override;
    virtual void onDeviceUnmounted( const std::string& uuid, const std::string& mountpoint ) override;

    std::shared_ptr<fs::IDevice> deviceByUuidLocked( const std::string& uuid );
    std::shared_ptr<fs::IDevice> deviceByMrlLocked( const std::string& mrl );

private:
    const std::string m_scheme;
    compat::Mutex m_devicesLock;
    std::vector<std::shared_ptr<fs::IDevice>> m_devices;
    std::shared_ptr<IDeviceLister> m_deviceLister;
    fs::IFileSystemFactoryCb* m_cb;
    bool m_isNetwork;
};

}
}
}
