#ifndef ISHOWEPISODE_H
#define ISHOWEPISODE_H

class IShow;

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
        virtual bool shouldBeDisplayed() = 0;
        virtual const std::string& tvdbId() = 0;
        virtual bool isUnread() = 0;
        virtual IShow* show() = 0;
};

#endif // ISHOWEPISODE_H
