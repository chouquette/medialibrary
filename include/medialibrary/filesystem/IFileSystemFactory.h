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

#include <memory>
#include <string>

namespace medialibrary
{

namespace fs
{
    class IDirectory;
    class IFile;
    class IDevice;

    /**
     * @brief IFileSystemFactoryCb is for external file system factories to signal device changes
     */
    class IFileSystemFactoryCb
    {
    public:
        virtual ~IFileSystemFactoryCb() = default;
        /**
         * @brief onDevicePlugged Shall be invoked when a device gets plugged
         * @param device The mounted device
         *
         * When this callback is invoked, the FS device presence must already be
         * updated.
         */
        virtual void onDeviceMounted( const fs::IDevice& device ) = 0;
        /**
         * @brief onDeviceUnplugged Shall be invoked when a device gets unplugged
         * @param device The unmounted device
         *
         * When this callback is invoked, the FS device presence must already be
         * updated. If no more mountpoints are available, device->isPresent must
         * return false by the time this is called
         */
        virtual void onDeviceUnmounted( const fs::IDevice& device ) = 0;
    };

    class IFileSystemFactory
    {
    public:
        virtual ~IFileSystemFactory() = default;
        ///
        /// \brief createDirectory creates a representation of a directory
        /// \note This method can fail by throwing an exception if the
        ///       directory doesn't exist, or any other I/O issue occurs.
        ///       Once created, the path of this IDirectory will be sanitized.
        /// \param mrl An MRL describing the desired directory
        ///
        virtual std::shared_ptr<fs::IDirectory> createDirectory( const std::string& mrl ) = 0;
        ///
        /// \brief createFile Creates a representation of a file
        /// \note This method can fail by throwing an exception if the file
        ///       doesn't exist, or if any other I/O issue occurs.
        /// \note The resulting file will have its mrl sanitized.
        /// \param mrl An MRL describing the desired file
        ///
        virtual std::shared_ptr<fs::IFile> createFile( const std::string& mrl ) = 0;
        ///
        /// \brief createDevice creates a representation of a device
        /// \param uuid The device UUID
        /// \return A representation of the device, or nullptr if the device is currently unavailable.
        ///
        virtual std::shared_ptr<fs::IDevice> createDevice( const std::string& uuid ) = 0;
        ///
        /// \brief createDeviceFromPath creates a representation of a device
        /// \param path An MRL.
        /// \return A representation of the device, or nullptr if none match.
        ///
        /// The provided path can and most often will refer to a file in that
        /// device, and will not simply be the path to the device mountpoint
        /// This function guarantees that the returned device, if any, is the
        /// one that matches the mrl best.
        /// For instance, if a device contains another device mountpoint, if
        /// that mountpoint or once of its subfolder is passed, the innermost
        /// device will be returned, even though more than one device matched
        ///
        virtual std::shared_ptr<fs::IDevice> createDeviceFromMrl( const std::string& mrl ) = 0;
        ///
        /// \brief refresh Will cause any FS cache to be refreshed.
        ///
        virtual void refreshDevices() = 0;
        ///
        /// \brief isPathSupported Checks for support of a path by this FS factory
        /// \param path The path to probe for support
        /// \return True if supported, false otherwise
        ///
        virtual bool isMrlSupported( const std::string& path ) const = 0;
        ///
        /// \brief isNetworkFileSystem Returns true if this FS factory handles network file systems
        ///
        virtual bool isNetworkFileSystem() const = 0;
        ///
        /// \brief scheme Returns the mrl scheme handled by this file system factory
        ///
        virtual const std::string& scheme() const = 0;
        ///
        /// \brief start Starts any potential background operation that would be
        /// required by this factory.
        /// \return False in case of an error.
        /// \param cb An instance of IFileSystemFactoryCb to be used to signal device changes
        ///           This instance is owned by the medialibrary, and must not
        ///           be released
        ///
        virtual bool start( IFileSystemFactoryCb* cb ) = 0;
        ///
        /// \brief stop stops any potential background operation that would be
        /// required by this factory.
        ///
        virtual void stop() = 0;
    };
}

}
