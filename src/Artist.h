#pragma once

#include "database/Cache.h"
#include "IArtist.h"
#include "IMediaLibrary.h"

class Artist;

namespace policy
{
struct ArtistTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Artist::*const PrimaryKey;
};
}

class Artist : public IArtist, public Cache<Artist, IArtist, policy::ArtistTable>
{
private:
    using _Cache = Cache<Artist, IArtist, policy::ArtistTable>;

public:
    Artist( DBConnection dbConnection, sqlite3_stmt* stmt );
    Artist( const std::string& name );

    virtual unsigned int id() const override;
    virtual const std::string &name() const override;
    virtual const std::string& shortBio() const override;
    virtual bool setShortBio( const std::string& shortBio ) override;
    virtual std::vector<AlbumPtr> albums() const override;
    virtual std::vector<AlbumTrackPtr> tracks() const override;

    static bool createTable( DBConnection dbConnection );
    static ArtistPtr create( DBConnection dbConnection, const std::string& name );

private:
    DBConnection m_dbConnection;
    unsigned int m_id;
    std::string m_name;
    std::string m_shortBio;

    friend _Cache;
    friend struct policy::ArtistTable;
};
