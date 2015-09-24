#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "Types.h"
#include "factory/IFileSystem.h"
#include "IDiscoverer.h"

class IParserCb
{
    public:
        virtual ~IParserCb() = default;

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

class IMediaLibrary : public IDiscovererCb
{
    public:
        virtual ~IMediaLibrary() {}
        ///
        /// \brief  initialize Initializes the media library.
        ///         This will use the provided discoverer to search for new media asynchronously.
        ///
        /// \param dbPath       Path to the database
        /// \return true in case of success, false otherwise
        ///
        virtual bool initialize( const std::string& dbPath, const std::string& snapshotPath, std::shared_ptr<factory::IFileSystem> fsFactory ) = 0;
        /// Adds a stand alone file
        virtual FilePtr addFile( const std::string& path ) = 0;
        virtual FilePtr file( const std::string& path ) = 0;
        virtual bool deleteFile( const std::string& mrl ) = 0;
        virtual bool deleteFile( FilePtr file ) = 0;

        /// Adds a folder and all the files it contains
        virtual FolderPtr folder( const std::string& path ) = 0;
        virtual bool deleteFolder( FolderPtr folder ) = 0;

        virtual LabelPtr createLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( const std::string& label ) = 0;
        virtual bool deleteLabel( LabelPtr label ) = 0;
        virtual std::vector<FilePtr> files() = 0;
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
        virtual void addMetadataService( std::unique_ptr<IMetadataService> service ) = 0;
        virtual void parse( FilePtr file, IParserCb* cb ) = 0;

        virtual void addDiscoverer( std::unique_ptr<IDiscoverer> discoverer ) = 0;
        /**
         * @brief discover Launch a discovery on the provided entry point.
         * There no garanty on how this will be processed, or if it will be processed synchronously or not.
         * Depending on which discoverer modules where provided, this might or might not work
         * @param entryPoint What to discover.
         */
        virtual void discover( const std::string& entryPoint ) = 0;
        virtual const std::string& snapshotPath() const = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
