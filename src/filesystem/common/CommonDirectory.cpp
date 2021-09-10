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

#include "CommonDirectory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"
#include "medialibrary/filesystem/Errors.h"
#include "utils/Filename.h"
#include <algorithm>

namespace medialibrary
{
namespace fs
{

medialibrary::fs::CommonDirectory::CommonDirectory( fs::IFileSystemFactory& fsFactory )
    : m_fsFactory( fsFactory )
{
}

const std::vector<std::shared_ptr<IFile>>& CommonDirectory::files() const
{
    if ( m_dirs.empty() == true && m_files.empty() == true )
        read();
    return m_files;
}

const std::vector<std::shared_ptr<IDirectory>>& CommonDirectory::dirs() const
{
    if ( m_dirs.empty() == true && m_files.empty() == true )
        read();
    return m_dirs;
}

std::shared_ptr<IDevice> CommonDirectory::device() const
{
    if ( m_device == nullptr )
        m_device = m_fsFactory.createDeviceFromMrl( mrl() );
    return m_device;
}

std::shared_ptr<IFile> CommonDirectory::file( const std::string& mrl ) const
{
    auto fs = files();
    // Don't compare entire mrls, this might yield false negative when a
    // device has multiple mountpoints.
    auto fileName = utils::file::fileName( mrl );
    auto it = std::find_if( cbegin( fs ), cend( fs ),
                            [&fileName]( const std::shared_ptr<fs::IFile>& f ) {
                                return f->name() == fileName;
                            });
    if ( it == cend( fs ) )
        throw fs::errors::NotFound{ mrl, "directory" };
    return *it;
}

bool CommonDirectory::contains( const std::string& fileName ) const
{
    auto fs = files();
    return std::find_if( cbegin( fs ), cend( fs ),
        [&fileName]( const std::shared_ptr<fs::IFile>& f ) {
            return f->name() == fileName;
    }) != cend( fs );
}

}
}
