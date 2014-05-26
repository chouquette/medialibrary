#ifndef ALBUMTRACK_H
#define ALBUMTRACK_H

#include <sqlite3.h>
#include <string>

#include "IAlbumTrack.h"
#include "IMediaLibrary.h"
#include "Cache.h"

class Album;
class AlbumTrack;

namespace policy
{
struct AlbumTrackTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int AlbumTrack::*const PrimaryKey;
};
}

class AlbumTrack : public IAlbumTrack, public Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>
{
    private:
        typedef Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable> _Cache;
    public:
        AlbumTrack( DBConnection dbConnection, sqlite3_stmt* stmt );
        AlbumTrack( const std::string& title, unsigned int trackNumber, unsigned int albumId );

        virtual unsigned int id() const;
        virtual const std::string& genre();
        virtual bool setGenre( const std::string& genre );
        virtual const std::string& title();
        virtual unsigned int trackNumber();
        virtual std::shared_ptr<IAlbum> album();
        virtual bool destroy();
        virtual bool files( std::vector<FilePtr>& files );

        static bool createTable( DBConnection dbConnection );
        static AlbumTrackPtr create( DBConnection dbConnection, unsigned int albumId,
                                     const std::string& name, unsigned int trackNb );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        std::string m_genre;
        unsigned int m_trackNumber;
        unsigned int m_albumId;

        std::shared_ptr<Album> m_album;

        friend class Cache<AlbumTrack, IAlbumTrack, policy::AlbumTrackTable>;
        friend struct policy::AlbumTrackTable;
};

#endif // ALBUMTRACK_H
