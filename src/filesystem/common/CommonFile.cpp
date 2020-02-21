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

#include "CommonFile.h"
#include "utils/Filename.h"

#include <cassert>

namespace medialibrary
{

namespace fs
{

CommonFile::CommonFile( std::string mrl )
    : m_mrl( std::move( mrl ) )
    , m_name( utils::file::fileName( m_mrl ) )
    , m_extension( utils::file::extension( m_mrl ) )
    , m_linkedType( LinkedFileType::None )
{
}

CommonFile::CommonFile( std::string mrl, IFile::LinkedFileType linkedType,
                        std::string linkedFile )
    : m_mrl( std::move( mrl ) )
    , m_name( utils::file::fileName( m_mrl ) )
    , m_extension( utils::file::extension( m_mrl ) )
    , m_linkedFile( std::move( linkedFile ) )
    , m_linkedType( linkedType )
{

}

const std::string& CommonFile::name() const
{
    return m_name;
}

const std::string& CommonFile::extension() const
{
    return m_extension;
}

const std::string& CommonFile::mrl() const
{
    return m_mrl;
}

bool CommonFile::isNetwork() const
{
    return false;
}

IFile::LinkedFileType CommonFile::linkedType() const
{
    return m_linkedType;
}

const std::string& CommonFile::linkedWith() const
{
    assert( m_linkedType != LinkedFileType::None );
    return m_linkedFile;
}

}

}
