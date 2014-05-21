#ifndef IMETADATASERVICE_H
#define IMETADATASERVICE_H

#include "IMediaLibrary.h"

class IMetadataServiceCb
{
    public:
        virtual void updated( FilePtr file ) = 0;
};

class IMetadataService
{
    public:
        enum Result
        {
            Success,
            Failure,
            NotApplicable // If trying to fetch the tv show summary of an album, for instance
        };

        virtual ~IMetadataService() {}
        virtual unsigned int priority() = 0;
        virtual Result run( FilePtr file ) = 0;
};

#endif // IMETADATASERVICE_H
