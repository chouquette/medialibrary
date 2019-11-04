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

#include <cassert>

#include "MockDevice.h"
#include "MockDirectory.h"
#include "utils/Filename.h"
#include "medialibrary/filesystem/Errors.h"

namespace mock
{

Device::Device(const std::string& mountpoint, const std::string& uuid)
    : CommonDevice( uuid, mountpoint, false )
    , m_present( true )
    , m_removable( false )
{
}

void Device::setupRoot()
{
    m_root = std::make_shared<Directory>( mountpoint(), shared_from_this() );
}

std::shared_ptr<Directory> Device::root()
{
    return m_root;
}

bool Device::isPresent() const { return m_present; }

bool Device::isRemovable() const { return m_removable; }

void Device::setRemovable(bool value) { m_removable = value; }

void Device::setPresent(bool value) { m_present = value; }

void Device::addFile( const std::string& mrl )
{
    m_root->addFile( relativeMrl( mrl ) );
}

void Device::addFolder( const std::string& mrl )
{
    m_root->addFolder( relativeMrl( mrl ) );
}

void Device::removeFile( const std::string& mrl )
{
    m_root->removeFile( relativeMrl( mrl ) );
}

void Device::removeFolder( const std::string& mrl )
{
    auto relMrl = relativeMrl( mrl );
    if ( relMrl.empty() == true )
        m_root = nullptr;
    else
        m_root->removeFolder( relMrl );
}

std::shared_ptr<fs::IFile> Device::file(const std::string& mrl )
{
    if ( m_root == nullptr || m_present == false )
        return nullptr;
    return m_root->file( relativeMrl( mrl ) );
}

std::shared_ptr<Directory> Device::directory( const std::string& mrl )
{
    if ( m_root == nullptr || m_present == false )
        throw medialibrary::fs::errors::System{ ENOENT, "Mock directory" };
    const auto relMrl = relativeMrl( mrl );
    if ( relMrl.empty() == true )
        return m_root;
    return m_root->directory( relMrl );
}

void Device::setMountpointRoot( const std::string& mrl, std::shared_ptr<Directory> root )
{
    auto relMrl = relativeMrl( mrl );
    // m_root is already a mountpoint, we can't add a mountpoint to it.
    assert( relMrl.empty() == false );
    m_root->setMountpointRoot( relMrl, root );
}

void Device::invalidateMountpoint( const std::string& mrl )
{
    auto relMrl = relativeMrl( mrl );
    assert( relMrl.empty() == false );
    m_root->invalidateMountpoint( relMrl );
}

}
