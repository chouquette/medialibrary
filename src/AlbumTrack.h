#ifndef ALBUMTRACK_H
#define ALBUMTRACK_H

#include <sqlite3.h>
#include <string>

#include "IAlbumTrack.h"
#include "Cache.h"

class Album;

namespace policy
{
struct AlbumTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
};
}

class AlbumTrack : public IAlbumTrack, public Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>
{
    public:
        AlbumTrack( sqlite3* dbConnection, sqlite3_stmt* stmt );

        virtual const std::string& genre();
        virtual const std::string& title();
        virtual unsigned int trackNumber();
        virtual std::shared_ptr<IAlbum> album();

        static bool createTable( sqlite3* dbConnection );

    private:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        std::string m_genre;
        unsigned int m_trackNumber;
        unsigned int m_albumId;

        std::shared_ptr<Album> m_album;

        friend class Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>;
};

#endif // ALBUMTRACK_H
