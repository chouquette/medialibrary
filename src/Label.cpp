#include <cassert>
#include <cstdlib>
#include <cstring>

#include "Label.h"
#include "File.h"
#include "SqliteTools.h"

const std::string policy::LabelTable::Name = "Label";
const std::string policy::LabelTable::CacheColumn = "name";
unsigned int Label::* const policy::LabelTable::PrimaryKey = &Label::m_id;

Label::Label( DBConnection dbConnection, sqlite3_stmt* stmt )
    : m_dbConnection( dbConnection )
{
    m_id = sqlite3_column_int( stmt, 0 );
    m_name = (const char*)sqlite3_column_text( stmt, 1 );
}

Label::Label( const std::string& name )
    : m_id( 0 )
    , m_name( name )
{
}

unsigned int Label::id() const
{
    return m_id;
}

const std::string& Label::name()
{
    return m_name;
}

bool Label::files( std::vector<FilePtr>& files )
{
    static const std::string req = "SELECT f.* FROM " + policy::FileTable::Name + " f "
            "LEFT JOIN LabelFileRelation lfr ON lfr.id_file = f.id_file "
            "WHERE lfr.id_label = ?";
    return SqliteTools::fetchAll<File>( m_dbConnection, req, files, m_id );
}

LabelPtr Label::create(DBConnection dbConnection, const std::string& name )
{
    auto self = std::make_shared<Label>( name );
    const char* req = "INSERT INTO Label VALUES(NULL, ?)";
    if ( _Cache::insert( dbConnection, self, req, self->m_name ) == false )
        return nullptr;
    self->m_dbConnection = dbConnection;
    return self;
}

bool Label::createTable(DBConnection dbConnection)
{
    std::string req = "CREATE TABLE IF NOT EXISTS " + policy::LabelTable::Name + "("
                "id_label INTEGER PRIMARY KEY AUTOINCREMENT, "
                "name TEXT UNIQUE ON CONFLICT FAIL"
            ")";
    if ( SqliteTools::executeRequest( dbConnection, req ) == false )
        return false;
    req = "CREATE TABLE IF NOT EXISTS LabelFileRelation("
                "id_label INTEGER,"
                "id_file INTEGER,"
            "PRIMARY KEY (id_label, id_file)"
            "FOREIGN KEY(id_label) REFERENCES Label(id_label) ON DELETE CASCADE,"
            "FOREIGN KEY(id_file) REFERENCES File(id_file) ON DELETE CASCADE);";
    return SqliteTools::executeRequest( dbConnection, req );
}

const std::string&policy::LabelCachePolicy::key( const std::shared_ptr<ILabel> self )
{
    return self->name();
}

std::string policy::LabelCachePolicy::key(sqlite3_stmt* stmt)
{
    return Traits<KeyType>::Load( stmt, 1 );
}
