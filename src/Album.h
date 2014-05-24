#ifndef ALBUM_H
#define ALBUM_H

#include <memory>
#include <sqlite3.h>

#include "IMediaLibrary.h"

#include "Cache.h"
#include "IAlbum.h"

class Album;

namespace policy
{
struct AlbumTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Album::*const PrimaryKey;
};
}

class Album : public IAlbum, public Cache<Album, IAlbum, policy::AlbumTable>
{
    private:
        typedef Cache<Album, IAlbum, policy::AlbumTable> _Cache;
    public:
        Album( sqlite3* dbConnection, sqlite3_stmt* stmt );
        Album( const std::string& id3tag );

        virtual unsigned int id() const;
        virtual const std::string& name() const;
        virtual bool setName( const std::string& name );
        virtual time_t releaseDate() const;
        virtual bool setReleaseDate( time_t date );
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary( const std::string& summary );
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl( const std::string& artworkUrl );
        virtual time_t lastSyncDate() const;
        virtual bool tracks( std::vector<std::shared_ptr<IAlbumTrack>>& tracks ) const;
        virtual AlbumTrackPtr addTrack( const std::string& name, unsigned int trackNb );

        static bool createTable( sqlite3* dbConnection );
        static AlbumPtr create( sqlite3* dbConnection, const std::string& id3Tag );

    protected:
        sqlite3* m_dbConnection;
        unsigned int m_id;
        std::string m_name;
        unsigned int m_releaseDate;
        std::string m_shortSummary;
        std::string m_artworkUrl;
        time_t m_lastSyncDate;
        std::string m_id3tag;

        friend class Cache<Album, IAlbum, policy::AlbumTable>;
        friend struct policy::AlbumTable;
};

#endif // ALBUM_H
