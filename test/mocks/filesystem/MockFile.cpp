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

#include "MockFile.h"

#include "utils/Filename.h"

namespace mock
{

File::File(const std::string& mrl )
    : m_name( utils::file::fileName( mrl ) )
    , m_extension( utils::file::extension( mrl ) )
    , m_lastModification( 0 )
    , m_mrl( mrl )
{
}

const std::string& File::name() const
{
    return m_name;
}

const std::string& File::extension() const
{
    return m_extension;
}

void File::markAsModified()
{
    m_lastModification++;
}

const std::string& File::mrl() const
{
    return m_mrl;
}

bool File::isNetwork() const
{
    return false;
}

fs::IFile::LinkedFileType File::linkedType() const
{
    return LinkedFileType::None;
}

const std::string &File::linkedWith() const
{
    return m_linkedWith;
}

time_t File::lastModificationDate() const
{
    return m_lastModification;
}

int64_t File::size() const
{
    return 0;
}

}
