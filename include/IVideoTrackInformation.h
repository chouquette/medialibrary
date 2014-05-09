#ifndef IVIDEOTRACKINFORMATION_H
#define IVIDEOTRACKINFORMATION_H

#include "ITrackInformation.h"

class IVideoTrackInformation : public ITrackInformation
{
    public:
        virtual ~IVideoTrackInformation() {}
        virtual unsigned int width() const = 0;
        virtual unsigned int height() const = 0;
};

#endif // IVIDEOTRACKINFORMATION_H
