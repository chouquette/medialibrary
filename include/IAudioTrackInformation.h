#ifndef IAUDIOTRACKINFORMATION_H
#define IAUDIOTRACKINFORMATION_H

#include "ITrackInformation.h"

class IAudioTrackInformation : public ITrackInformation
{
    public:
        virtual ~IAudioTrackInformation() {}

        virtual unsigned int bitrate() const = 0;
        virtual unsigned int nbChannel() const = 0;
};

#endif // IAUDIOTRACKINFORMATION_H
