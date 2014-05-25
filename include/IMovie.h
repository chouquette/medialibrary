#ifndef IMOVIE_H
#define IMOVIE_H

#include "IMediaLibrary.h"

class IMovie
{
    public:
        virtual ~IMovie() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& title() const = 0;
        virtual time_t releaseDate() const = 0;
        virtual bool setReleaseDate( time_t date ) = 0;
        virtual const std::string& shortSummary() const = 0;
        virtual bool setShortSummary( const std::string& summary ) = 0;
        virtual const std::string& artworkUrl() const = 0;
        virtual bool setArtworkUrl( const std::string& artworkUrl ) = 0;
        virtual const std::string& imdbId() const = 0;
        virtual bool setImdbId( const std::string& id ) = 0;
        virtual bool destroy() = 0;
        virtual bool files( std::vector<FilePtr>& files ) = 0;
};

#endif // IMOVIE_H
