#ifndef IMETADATASERVICE_H
#define IMETADATASERVICE_H

#include "IMediaLibrary.h"

class IMetadataServiceCb
{
    public:
        virtual void updated( FilePtr file ) = 0;
        virtual void error( FilePtr file, const std::string& error ) = 0;
};

class IMetadataService
{
    public:
        virtual ~IMetadataService() {}
        virtual bool initialize( IMetadataServiceCb* callback, IMediaLibrary* ml ) = 0;
        virtual unsigned int priority() const = 0;
        virtual bool run( FilePtr file ) = 0;
};

#endif // IMETADATASERVICE_H
