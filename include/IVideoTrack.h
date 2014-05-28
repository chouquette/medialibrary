#ifndef IVIDEOTRACK_H
#define IVIDEOTRACK_H

#include "IMediaLibrary.h"

class IVideoTrack
{
    public:
        virtual ~IVideoTrack() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& codec() const = 0;
        virtual unsigned int width() const = 0;
        virtual unsigned int height() const = 0;
        virtual float fps() const = 0;
};

#endif // IVIDEOTRACK_H
