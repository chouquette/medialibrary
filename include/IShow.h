#ifndef ISHOW_H
#define ISHOW_H

#include <string>
#include "IDescription.h"

class IShow : public IDescription
{
    public:
        virtual ~IShow() {}

        virtual const std::string& artworkUrl() = 0;
        virtual time_t lastSyncDate() = 0;
        virtual const std::string& tvdbId() = 0;
};

#endif // ISHOW_H
