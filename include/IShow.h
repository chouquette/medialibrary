#ifndef ISHOW_H
#define ISHOW_H

#include <string>

class IShow
{
    public:
        virtual ~IShow() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& name() const = 0;
        virtual time_t releaseDate() const = 0;
        virtual bool setReleaseDate( time_t date ) = 0;
        virtual const std::string& shortSummary() const = 0;
        virtual bool setShortSummary( const std::string& summary ) = 0;
        virtual const std::string& artworkUrl() const = 0;
        virtual bool setArtworkUrl( const std::string& artworkUrl ) = 0;
        virtual const std::string& tvdbId() = 0;
        virtual bool setTvdbId( const std::string& id ) = 0;
};

#endif // ISHOW_H
