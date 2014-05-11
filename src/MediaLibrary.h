#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include <sqlite3.h>

#include "IMediaLibrary.h"

class MediaLibrary : public IMediaLibrary
{
    public:
        MediaLibrary();
        virtual bool initialize( const std::string& dbPath );
        virtual const std::vector<IFile*>& files();
        virtual IFile* addFile( const std::string& path );

    private:
        sqlite3* m_dbConnection;

        std::vector<IFile*>* m_files;
};

#endif // MEDIALIBRARY_H
