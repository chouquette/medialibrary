#ifndef MOVIE_H
#define MOVIE_H

#include "IMovie.h"
#include <sqlite3.h>
#include "Cache.h"

class Movie;

namespace policy
{
struct MovieTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int Movie::*const PrimaryKey;
};
}

class Movie : public IMovie, public Cache<Movie, IMovie, policy::MovieTable>
{
    public:
        Movie( DBConnection dbConnection, sqlite3_stmt* stmt );
        Movie( const std::string& title );

        virtual unsigned int id() const;
        virtual const std::string& title() const;
        virtual time_t releaseDate() const;
        virtual bool setReleaseDate(time_t date);
        virtual const std::string& shortSummary() const;
        virtual bool setShortSummary(const std::string& summary);
        virtual const std::string& artworkUrl() const;
        virtual bool setArtworkUrl(const std::string& artworkUrl);
        virtual const std::string& imdbId() const;
        virtual bool setImdbId(const std::string& imdbId);
        virtual bool destroy();
        virtual std::vector<FilePtr> files();

        static bool createTable( DBConnection dbConnection );
        static MoviePtr create( DBConnection dbConnection, const std::string& title );

    private:
        DBConnection m_dbConnection;
        unsigned int m_id;
        std::string m_title;
        time_t m_releaseDate;
        std::string m_summary;
        std::string m_artworkUrl;
        std::string m_imdbId;

        typedef Cache<Movie, IMovie, policy::MovieTable> _Cache;
        friend struct policy::MovieTable;
};

#endif // MOVIE_H
