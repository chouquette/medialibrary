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

#ifndef FS_DISCOVERER_H
# define FS_DISCOVERER_H

#include <memory>

#include "IDiscoverer.h"
#include "factory/IFileSystem.h"

class MediaLibrary;
class Folder;

class FsDiscoverer : public IDiscoverer
{
public:
    FsDiscoverer(std::shared_ptr<factory::IFileSystem> fsFactory, MediaLibrary* ml, DBConnection dbConn );
    virtual bool discover(const std::string &entryPoint ) override;
    virtual void reload() override;

private:
    bool checkSubfolders(fs::IDirectory *folder, Folder* parentFolder , const std::vector<std::shared_ptr<Folder> > blacklist);
    void checkFiles(fs::IDirectory *folder, Folder* parentFolder );
    std::vector<std::shared_ptr<Folder>> blacklist() const;
    bool isBlacklisted( const std::string& path, const std::vector<std::shared_ptr<Folder>>& blacklist ) const;

private:
    MediaLibrary* m_ml;
    DBConnection m_dbConn;
    std::shared_ptr<factory::IFileSystem> m_fsFactory;
};

#endif // FS_DISCOVERER_H
