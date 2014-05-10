#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "IFile.h"

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary(){}
        virtual bool initialize( const std::string& dbPath );

        virtual const std::vector<IFile*>& files() = 0;
};

#endif // IMEDIALIBRARY_H
