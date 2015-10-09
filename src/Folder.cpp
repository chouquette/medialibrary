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

#include "Folder.h"
#include "File.h"

#include "database/SqliteTools.h"
#include "filesystem/IDirectory.h"

namespace policy
{
    const std::string FolderTable::Name = "Folder";
    const std::string FolderTable::CacheColumn = "path";
    unsigned int Folder::* const FolderTable::PrimaryKey = &Folder::m_id;

    const FolderCache::KeyType&FolderCache::key(const std::shared_ptr<Folder>& self)
    {
        return self->path();
    }

    FolderCache::KeyType FolderCache::key( sqlite3_stmt* stmt )
    {
        return sqlite::Traits<FolderCache::KeyType>::Load( stmt, 1 );
    }

}

Folder::Folder( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConection( dbConnection )
{
    m_id = sqlite::Traits<unsigned int>::Load( stmt, 0 );
    m_path = sqlite::Traits<std::string>::Load( stmt, 1 );
    m_parent = sqlite::Traits<unsigned int>::Load( stmt, 2 );
    m_lastModificationDate = sqlite::Traits<unsigned int>::Load( stmt, 3 );
    m_isRemovable = sqlite::Traits<bool>::Load( stmt, 4 );
}

Folder::Folder( const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parent )
    : m_id( 0 )
    , m_path( path )
    , m_parent( parent )
    , m_lastModificationDate( lastModificationDate )
    , m_isRemovable( isRemovable )
{
}

bool Folder::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
            "("
            "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT UNIQUE ON CONFLICT FAIL,"
            "id_parent UNSIGNED INTEGER,"
            "last_modification_date UNSIGNED INTEGER,"
            "is_removable INTEGER,"
            "FOREIGN KEY (id_parent) REFERENCES " + policy::FolderTable::Name +
            "(id_folder) ON DELETE CASCADE"
            ")";
    return sqlite::Tools::executeRequest( connection, req );
}

FolderPtr Folder::create( DBConnection connection, const std::string& path, time_t lastModificationDate, bool isRemovable, unsigned int parentId )
{
    auto self = std::make_shared<Folder>( path, lastModificationDate, isRemovable, parentId );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, id_parent, last_modification_date, is_removable) VALUES(?, ?, ?, ?)";
    if ( _Cache::insert( connection, self, req, path, sqlite::ForeignKey( parentId ),
                         lastModificationDate, isRemovable ) == false )
        return nullptr;
    self->m_dbConection = connection;
    return self;
}

unsigned int Folder::id() const
{
    return m_id;
}

const std::string& Folder::path()
{
    return m_path;
}

std::vector<FilePtr> Folder::files()
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name +
        " WHERE folder_id = ?";
    return File::fetchAll( m_dbConection, req, m_id );
}

std::vector<FolderPtr> Folder::folders()
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE id_parent = ?";
    return fetchAll( m_dbConection, req, m_id );
}

FolderPtr Folder::parent()
{
    //FIXME: use path to be able to fetch from cache?
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE id_folder = ?";
    return fetchOne( m_dbConection, req, m_parent );
}

unsigned int Folder::lastModificationDate()
{
    return m_lastModificationDate;
}

bool Folder::setLastModificationDate( unsigned int lastModificationDate )
{
    static const std::string req = "UPDATE " + policy::FolderTable::Name +
            " SET last_modification_date = ? WHERE id_folder = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConection, req, lastModificationDate, m_id ) == false )
        return false;
    m_lastModificationDate = lastModificationDate;
    return true;
}

bool Folder::isRemovable()
{
    return m_isRemovable;
}
