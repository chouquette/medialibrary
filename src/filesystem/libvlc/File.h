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

#include "filesystem/common/CommonFile.h"

namespace medialibrary
{
namespace fs
{

class IFileSystemFactory;

namespace libvlc
{

class File : public CommonFile
{
public:
    File( std::string mrl, IFileSystemFactory& fsFactory,
          time_t lastModificationDate, uint64_t size );
    File( std::string mrl, IFileSystemFactory& fsFactory,
          time_t lastModificationDate, uint64_t size,
          IFile::LinkedFileType linkedType, std::string linkedFile );
    virtual time_t lastModificationDate() const override;
    virtual uint64_t size() const override;
    virtual bool isNetwork() const override;

private:
    time_t m_lastModificationDate;
    uint64_t m_size;
    bool m_isNetwork;
};
}
}
}
