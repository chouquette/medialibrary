#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

#include <sqlite3.h>

#include "IMediaLibrary.h"

class MediaLibrary : public IMediaLibrary
{
    public:
        MediaLibrary();
        virtual bool initialize( const std::string& dbPath );
        virtual bool files( std::vector<FilePtr>& res );
        virtual FilePtr file( const std::string& path );
        virtual FilePtr addFile( const std::string& path );
        virtual LabelPtr createLabel(const std::string& label);

    private:
        sqlite3* m_dbConnection;
};
#endif // MEDIALIBRARY_H
