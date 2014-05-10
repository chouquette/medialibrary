#ifndef ALBUMTRACK_H
#define ALBUMTRACK_H

#include <sqlite3.h>
#include <string>

#include "IAlbumTrack.h"

class Album;

class AlbumTrack : public IAlbumTrack
{
    public:
        AlbumTrack( sqlite3* dbConnection, sqlite3_stmt* stmt );

        virtual const std::string&genre();
        virtual const std::string&title();
        virtual unsigned int trackNumber();
        virtual IAlbum*album();

        static bool createTable( sqlite3* dbConnection );
        static AlbumTrack* fetch( sqlite3* dbConnection, unsigned int idTrack );

    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        std::string m_genre;
        unsigned int m_trackNumber;
        unsigned int m_albumId;

        Album* m_album;
};

#endif // ALBUMTRACK_H
