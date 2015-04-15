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
    m_parent = Traits<unsigned int>::Load( stmt, 2 );
}

Folder::Folder( const std::string& path, unsigned int parent )
    : m_path( path )
    , m_parent( parent )
{
}

bool Folder::createTable(DBConnection connection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
            "("
            "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
            "path TEXT UNIQUE ON CONFLICT FAIL,"
            "id_parent UNSIGNED INTEGER,"
            "FOREIGN KEY (id_parent) REFERENCES " + policy::FolderTable::Name +
            "(id_folder) ON DELETE CASCADE"
            ")";
    return SqliteTools::executeRequest( connection, req );
}

FolderPtr Folder::create(DBConnection connection, const std::string& path, unsigned int parent )
{
    auto self = std::make_shared<Folder>( path, parent );
    static const std::string req = "INSERT INTO " + policy::FolderTable::Name +
            "(path, id_parent) VALUES(?, ?)";
    if ( parent == 0 )
    {
        if ( _Cache::insert( connection, self, req, path, nullptr ) == false )
            return nullptr;
    }
    else
    {
        if ( _Cache::insert( connection, self, req, path, parent ) == false )
            return nullptr;
    }
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
    return SqliteTools::fetchAll<File, IFile>( m_dbConection, req, m_id );
}

FolderPtr Folder::parent()
{
    //FIXME: use path to be able to fetch from cache?
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name +
            " WHERE id_folder = ?";
    return SqliteTools::fetchOne<Folder>( m_dbConection, req, m_parent );
}
