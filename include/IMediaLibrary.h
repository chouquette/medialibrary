#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "Types.h"

class IParserCb
{
    public:
        virtual ~IParserCb() {}

        /**
         * @brief onServiceDone will be called after each MetadataService completes
         * @param file      The file being parsed
         * @param status    A flag describing the parsing outcome
         */
        virtual void onServiceDone( FilePtr file, ServiceStatus status ) = 0;
        /**
         * @brief onFileDone will be called when all parsing operations on a given file have been completed
         *
         * This doesn't imply all modules have succeeded
         */
        //FIXME: We should probably expose some way of telling if the user should retry later in
        // case of tmeporary failure
        virtual void onFileDone( FilePtr file ) = 0;
};

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() {}
        virtual bool initialize( const std::string& dbPath ) = 0;
        /// Adds a stand alone file
        virtual FilePtr addFile( const std::string& path ) = 0;
        /// Adds a folder and all the files it contains
        virtual FolderPtr addFolder( const std::string& path ) = 0;
        virtual FilePtr file( const std::string& path ) = 0;
        virtual bool deleteFile( const std::string& mrl ) = 0;
        virtual bool deleteFile( FilePtr file ) = 0;
        virtual bool deleteFolder( FolderPtr folder ) = 0;
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
         * User is expected to add all services before calling parse for the first time.
         * Once parse has been called, adding another service is an undefined behavior.
         * This method will call service->initialize(), therefor the passed ServiceStatus
         * is expected to be uninitialized.
         */
        virtual void addMetadataService( IMetadataService* service ) = 0;
        virtual void parse( FilePtr file, IParserCb* cb ) = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
