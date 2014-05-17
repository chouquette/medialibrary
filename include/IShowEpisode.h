#ifndef ISHOWEPISODE_H
#define ISHOWEPISODE_H

class IShow;

#include <memory>
#include <string>

class IShowEpisode
{
    public:
        virtual ~IShowEpisode(){}

        virtual const std::string& artworkUrl() = 0;
        virtual unsigned int episodeNumber() = 0;
        virtual time_t lastSyncDate() = 0;
        virtual const std::string& name() = 0;
        virtual unsigned int seasonNuber() = 0;
        virtual const std::string& shortSummary() = 0;
        virtual const std::string& tvdbId() = 0;
        virtual std::shared_ptr<IShow> show() = 0;
};

#endif // ISHOWEPISODE_H
