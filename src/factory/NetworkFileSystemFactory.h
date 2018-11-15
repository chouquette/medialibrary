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

#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "filesystem/network/Device.h"
#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

#include <vlcpp/vlc.hpp>


namespace medialibrary
{

class IDeviceListerCb;

namespace factory
{
class NetworkFileSystemFactory : public fs::IFileSystemFactory
{
public:
    /**
     * @brief NetworkFileSystemFactory Constructs a network protocol specific filesystem factory
     * @param protocol The protocol name
     */
    NetworkFileSystemFactory( const std::string& protocol, const std::string& name );
    virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& path ) override;
    virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) override;
    virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& path ) override;
    virtual void refreshDevices() override;
    virtual bool isMrlSupported( const std::string& path ) const override;
    virtual bool isNetworkFileSystem() const override;
    virtual const std::string& scheme() const override;
    virtual void start( fs::IFileSystemFactoryCb* cb ) override;
    virtual void stop() override;

private:
    void onDeviceAdded( VLC::MediaPtr media );
    void onDeviceRemoved( VLC::MediaPtr media );

private:
    struct Device
    {
        Device( const std::string& name, const std::string& mrl, VLC::Media media )
            : name( name )
            , mrl( mrl )
            , media( media )
            , device( std::make_shared<fs::NetworkDevice>( name, mrl ) )
        {}
        std::string name;
        std::string mrl;
        VLC::Media media;
        std::shared_ptr<fs::NetworkDevice> device;
    };

    const std::string m_protocol;
    compat::Mutex m_devicesLock;
    compat::ConditionVariable m_deviceCond;
    std::vector<Device> m_devices;
    VLC::MediaDiscoverer m_discoverer;
    std::shared_ptr<VLC::MediaList> m_mediaList;
    fs::IFileSystemFactoryCb* m_cb;
};

}
}
