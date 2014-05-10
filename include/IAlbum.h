#ifndef IALBUM_H
#define IALBUM_H

#include <string>
#include <vector>

#include "IDescription.h"

class ITrack;

class IAlbum
{
    public:
        virtual ~IAlbum() {}
        virtual const std::string& name() = 0;
        virtual unsigned int releaseYear() = 0;
        virtual const std::string& shortSummary() = 0;
        virtual const std::string& artworkUrl() = 0;
        virtual const std::vector<ITrack*>& tracks() = 0;
};

#endif // IALBUM_H
