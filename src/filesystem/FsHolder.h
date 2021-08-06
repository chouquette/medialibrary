/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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
#include "medialibrary/Types.h"
#include "Types.h"
#include "compat/Mutex.h"

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace medialibrary
{

class FsHolder
{
public:
    FsHolder( MediaLibrary* ml );

    bool addFsFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory );
    void registerDeviceLister( const std::string& scheme, DeviceListerPtr lister );
    DeviceListerPtr deviceLister( const std::string& scheme ) const;
    bool setNetworkEnabled( bool enabled );
    bool isNetworkEnabled() const;

    /**
     * @brief startFsFactoriesAndRefresh Starts fs factories & refreshes all known devices
     *
     * This will start all provided & required file system factories (ie. local
     * ones, and network ones if network discovery is enabled), and refresh the
     * presence & last seen date for all known devices we have in database.
     * This operation must not be based on the available FsFactories, as we might
     * not have a factory that was used to create a device before.
     * We still need to mark all the associated devices as missing.
     */
    void startFsFactoriesAndRefresh();

    void stopNetworkFsFactories();

    std::shared_ptr<fs::IFileSystemFactory>
    fsFactoryForMrl( const std::string& mrl ) const;

    void startFsFactory( fs::IFileSystemFactory& fsFactory ) const;

    class FsFactoryCb : public fs::IFileSystemFactoryCb
    {
    public:
        explicit FsFactoryCb( MediaLibrary* ml );
    private:
        virtual void onDeviceMounted( const fs::IDevice& deviceFs,
                                      const std::string& newMountpoint ) override;
        virtual void onDeviceUnmounted( const fs::IDevice& deviceFs,
                                        const std::string& removedMountpoint ) override;
    private:
        MediaLibrary* m_ml;
    };

private:
    std::shared_ptr<fs::IFileSystemFactory>
    fsFactoryForMrlLocked( const std::string& mrl ) const;

private:
    MediaLibrary* m_ml;
    mutable compat::Mutex m_mutex;

    /* All fs factory callbacks must outlive the fs factory itself, since
     * it might invoke some of the callback interface methods during teardown
     */
    mutable FsFactoryCb m_fsFactoryCb;

    std::vector<std::shared_ptr<fs::IFileSystemFactory>> m_fsFactories;
    // Device lister will invoke fs factories through IDeviceListerCb so
    // the device lister must be destroyed before the fs factories
    std::unordered_map<std::string, DeviceListerPtr> m_deviceListers;
    std::atomic_bool m_networkDiscoveryEnabled;
    std::atomic_bool m_started;
};

}
