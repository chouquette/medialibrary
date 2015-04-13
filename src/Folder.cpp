#include "Folder.h"
#include "File.h"

#include "SqliteTools.h"

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
        return Traits<FolderCache::KeyType>::Load( stmt, 1 );
    }

}

Folder::Folder( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConection( dbConnection )
{
    m_id = Traits<unsigned int>::Load( stmt, 0 );
    m_path = Traits<std::string>::Load( stmt, 1 );
}

Folder::Folder( const std::string& path )
    : m_path( path )
{
}

bool Folder::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name + "("
            + "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT UNIQUE ON CONFLICT FAIL"
            ")";
    return SqliteTools::executeRequest( connection, req );
}

FolderPtr Folder::create(DBConnection connection, const std::string& path)
{
    auto self = std::make_shared<Folder>( path );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path) VALUES(?)";
    if ( _Cache::insert( connection, self, req, path ) == false )
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
    static const std::string req = "SELECT f.* FROM " + policy::FileTable::Name +
        " WHERE f.id_folder = ?";
    return SqliteTools::fetchAll<File, IFile>( m_dbConection, req, m_id );
}
