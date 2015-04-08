#pragma once

#include "Cache.h"
#include "IFolder.h"

#include <sqlite3.h>

class Folder;

namespace policy
{
struct FolderTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Folder::*const PrimaryKey;
};
}

class Folder : public IFolder, public Cache<Folder, IFolder, policy::FolderTable>
{
    using _Cache = Cache<Folder, IFolder, policy::FolderTable>;

public:
    Folder(DBConnection dbConnection, sqlite3_stmt* stmt);
    Folder( const std::string& path );

    static bool createTable( DBConnection connection );
    static FolderPtr create( DBConnection connection, const std::string& path );

    virtual unsigned int id() const override;
    virtual const std::string& path() override;
    virtual std::vector<FilePtr> files() override;

private:
    DBConnection m_dbConection;

    unsigned int m_id;
    std::string m_path;

    friend class Cache<Folder, IFolder, policy::FolderTable>;
    friend class policy::FolderTable;
};
