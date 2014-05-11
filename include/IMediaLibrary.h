#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "IFile.h"

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() {}
        virtual bool initialize( const std::string& dbPath ) = 0;
        virtual IFile* addFile( const std::string& path ) = 0;
        virtual const std::vector<IFile*>& files() = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
