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

#include "filesystem/common/CommonDirectory.h"
#include "medialibrary/filesystem/IFile.h"

namespace medialibrary
{
namespace fs
{
namespace libvlc
{

class Directory : public CommonDirectory
{
public:
    Directory( std::string mrl, fs::IFileSystemFactory& fsFactory );
    virtual const std::string& mrl() const override;

private:
    // These are unintuitively const, because the files/subfolders list is
    // lazily initialized when calling files() / dirs()
    virtual void read() const override;
    void addFile( std::string mrl, IFile::LinkedFileType fileType,
                  std::string linkedWith, time_t lastModificationDate,
                  int64_t fileSize ) const;

private:
    std::string m_mrl;
};

}
}
}
