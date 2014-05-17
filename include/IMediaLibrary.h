#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>
#include <memory>

class IFile;

typedef std::shared_ptr<IFile> FilePtr;

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() {}
        virtual bool initialize( const std::string& dbPath ) = 0;
        virtual FilePtr addFile( const std::string& path ) = 0;
        virtual FilePtr file( const std::string& path ) = 0;
        virtual bool files( std::vector<FilePtr>& res ) = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
