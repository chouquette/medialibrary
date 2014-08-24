#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "Types.h"

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() {}
        virtual bool initialize( const std::string& dbPath ) = 0;
        virtual FilePtr addFile( const std::string& path ) = 0;
        virtual FilePtr file( const std::string& path ) = 0;
        virtual bool deleteFile( const std::string& mrl ) = 0;
        virtual bool deleteFile( FilePtr file ) = 0;
        virtual LabelPtr createLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( LabelPtr label ) = 0;
        virtual bool files( std::vector<FilePtr>& res ) = 0;
        virtual AlbumPtr album( const std::string& title ) = 0;
        virtual AlbumPtr createAlbum( const std::string& title ) = 0;
        virtual ShowPtr show( const std::string& name ) = 0;
        virtual ShowPtr createShow( const std::string& name ) = 0;
        virtual MoviePtr movie( const std::string& title ) = 0;
        virtual MoviePtr createMovie( const std::string& title ) = 0;

        /**
         * @brief addMetadataService Adds a service to parse media
         *
         * Use is expected to add all services before calling parse for the first time.
         * Once parse has been called, adding another service is an undefined behavior
         */
        virtual void addMetadataService( IMetadataService* service ) = 0;
        virtual void parse( FilePtr file ) = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
