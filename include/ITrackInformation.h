#ifndef ITRACKINFORMATION_H
#define ITRACKINFORMATION_H

class ITrackInformation
{
        virtual ~ITrackInformation() {}
        virtual std::string codec() const = 0;
};

#endif // ITRACKINFORMATION_H
