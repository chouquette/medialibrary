#ifndef IMETADATASERVICE_H
#define IMETADATASERVICE_H

#include "IMediaLibrary.h"

class IMetadataServiceCb
{
    public:
        virtual ~IMetadataServiceCb(){}
        virtual void done( FilePtr file, ServiceStatus status, void* data ) = 0;
};

class IMetadataService
{
    public:
        virtual ~IMetadataService() = default;
        virtual bool initialize( IMetadataServiceCb* callback, IMediaLibrary* ml ) = 0;
        virtual unsigned int priority() const = 0;
        virtual bool run( FilePtr file, void* data ) = 0;
};

#endif // IMETADATASERVICE_H
