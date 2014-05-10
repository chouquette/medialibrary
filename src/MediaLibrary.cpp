#include "MediaLibrary.h"

MediaLibrary::MediaLibrary()
{
}

bool MediaLibrary::initialize(const std::string& dbPath)
{
    int res = sqlite3_open( dbPath.c_str(), &m_dbConnection );
    //FIXME:
    // PRAGMA foreign_keys = ON;
    return res == SQLITE_OK;
}


const std::vector<IFile*>&MediaLibrary::files()
{
    if ( m_files == NULL )
    {
        m_files = new std::vector<IFile*>();
        // magic
    }
    return *m_files;
}
