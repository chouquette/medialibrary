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

#pragma once

#include "IFile.h"
#include "database/DatabaseHelpers.h"
#include "database/SqliteConnection.h"
#include "filesystem/IFile.h"
#include "utils/Cache.h"

class File;
class Media;

namespace policy
{
struct FileTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static unsigned int File::*const PrimaryKey;
};
}

class File : public IFile, public DatabaseHelpers<File, policy::FileTable>
{
public:
    File( DBConnection dbConnection, sqlite::Row& row );
    File( unsigned int mediaId, Type type, const fs::IFile& file, unsigned int folderId, bool isRemovable );
    virtual unsigned int id() const override;
    virtual const std::string& mrl() const override;
    virtual Type type() const override;
    virtual unsigned int lastModificationDate() const override;
    /// Explicitely mark a media as fully parsed, meaning no metadata service needs to run anymore.
    //FIXME: This lacks granularity as we don't have a straight forward way to know which service
    //needs to run or not.
    void markParsed();
    bool isParsed() const;
    std::shared_ptr<Media> media() const;
    bool destroy();

    static bool createTable( DBConnection dbConnection );
    static std::shared_ptr<File> create( DBConnection dbConnection, unsigned int mediaId, Type type, const fs::IFile& file, unsigned int folderId, bool isRemovable );

private:
    unsigned int m_id;
    unsigned int m_mediaId;
    std::string m_mrl;
    Type m_type;
    unsigned int m_lastModificationDate;
    bool m_isParsed;
    unsigned int m_folderId;
    bool m_isPresent;
    bool m_isRemovable;

    DBConnection m_dbConnection;
    mutable Cache<std::string> m_fullPath;
    mutable Cache<std::shared_ptr<Media>> m_media;

    friend class policy::FileTable;
};
