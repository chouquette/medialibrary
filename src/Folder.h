#pragma once

#include "database/Cache.h"
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

struct FolderCache
{
    using KeyType = std::string;
    static const KeyType& key( const std::shared_ptr<Folder>& self );
    static KeyType key( sqlite3_stmt* stmt );
};

}

class Folder : public IFolder, public Cache<Folder, IFolder, policy::FolderTable, policy::FolderCache>
{
    using _Cache = Cache<Folder, IFolder, policy::FolderTable, policy::FolderCache>;

public:
    Folder(DBConnection dbConnection, sqlite3_stmt* stmt);
    Folder(const std::string& path , unsigned int parent, unsigned int lastModificationDate );

    static bool createTable( DBConnection connection );
    static FolderPtr create(DBConnection connection, const std::string& path , unsigned int parent, unsigned int lastModifDate);

    virtual unsigned int id() const override;
    virtual const std::string& path() override;
    virtual std::vector<FilePtr> files() override;
    virtual std::vector<FolderPtr> folders() override;
    virtual FolderPtr parent() override;
    virtual unsigned int lastModificationDate() override;
    virtual bool setLastModificationDate(unsigned int lastModificationDate) override;

private:
    DBConnection m_dbConection;

    unsigned int m_id;
    std::string m_path;
    unsigned int m_parent;
    unsigned int m_lastModificationDate;

    friend _Cache;
    friend struct policy::FolderTable;
};
