#ifndef IALBUM_H
#define IALBUM_H

#include <string>
#include <vector>

#include "IDescription.h"

class IAlbum : public IDescription
{
    public:
        virtual ~IAlbum() {}
        virtual const std::vector<ITrack*>& tracks() = 0;
};

#endif // IALBUM_H
