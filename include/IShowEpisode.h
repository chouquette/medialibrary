#ifndef ISHOWEPISODE_H
#define ISHOWEPISODE_H

class IShow;

#include "IMediaLibrary.h"

class IShowEpisode
{
    public:
        virtual ~IShowEpisode(){}

        virtual unsigned int id() const = 0;
        virtual const std::string& artworkUrl() const = 0;
        virtual bool setArtworkUrl( const std::string& artworkUrl ) = 0;
        virtual unsigned int episodeNumber() const = 0;
        virtual time_t lastSyncDate() const = 0;
        virtual const std::string& name() const = 0;
        virtual unsigned int seasonNumber() const = 0;
        virtual bool setSeasonNumber(unsigned int seasonNumber) = 0;
        virtual const std::string& shortSummary() const = 0;
        virtual bool setShortSummary( const std::string& summary ) = 0;
        virtual const std::string& tvdbId() const = 0;
        virtual bool setTvdbId( const std::string& tvdbId ) = 0;
        virtual std::shared_ptr<IShow> show() = 0;
        virtual std::vector<FilePtr> files() = 0;
        /**
         * @brief destroy Deletes the album track and the file(s) associated
         */
        virtual bool destroy() = 0;
};

#endif // ISHOWEPISODE_H
