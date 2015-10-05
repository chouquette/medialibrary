#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>

#include "Types.h"
#include "factory/IFileSystem.h"
#include "IDiscoverer.h"

class IMediaLibraryCb
{
public:
    virtual ~IMediaLibraryCb() = default;
    /**
     * @brief onFileAdded Will be called when a file gets added.
     * Depending if the file is being restored or was just discovered,
     * the file type might be a best effort guess. If the file was freshly
     * discovered, it is extremely likely that no metadata will be
     * available yet.
     * @param file
     */
    virtual void onFileAdded( FilePtr file ) = 0;
    /**
     * @brief onFileUpdated Will be called when a file metadata gets updated.
     * @param file The updated file.
     */
    virtual void onFileUpdated( FilePtr file ) = 0;

    virtual void onDiscoveryStarted( const std::string& entryPoint ) = 0;
    virtual void onDiscoveryCompleted( const std::string& entryPoint ) = 0;
};

class IMediaLibrary
{
    public:
        virtual ~IMediaLibrary() = default;
        ///
        /// \brief  initialize Initializes the media library.
        ///         This will use the provided discoverer to search for new media asynchronously.
        ///
        /// \param dbPath       Path to the database
        /// \return true in case of success, false otherwise
        ///
        virtual bool initialize( const std::string& dbPath, const std::string& snapshotPath, IMediaLibraryCb* metadataCb ) = 0;
        /**
         * Replaces the default filesystem factory
         * The default one will use standard opendir/readdir functions
         * Calling this after initialize() is not a supported scenario.
         */
        virtual void setFsFactory( std::shared_ptr<factory::IFileSystem> fsFactory ) = 0;
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
        virtual std::vector<FilePtr> audioFiles() = 0;
        virtual std::vector<FilePtr> videoFiles() = 0;
        virtual AlbumPtr album( const std::string& title ) = 0;
        virtual AlbumPtr createAlbum( const std::string& title ) = 0;
        virtual std::vector<AlbumPtr> albums() = 0;
        virtual ShowPtr show( const std::string& name ) = 0;
        virtual ShowPtr createShow( const std::string& name ) = 0;
        virtual MoviePtr movie( const std::string& title ) = 0;
        virtual MoviePtr createMovie( const std::string& title ) = 0;
        virtual ArtistPtr artist( const std::string& name ) = 0;
        virtual ArtistPtr createArtist( const std::string& name ) = 0;

        /**
         * @brief discover Launch a discovery on the provided entry point.
         * The actuall discovery will run asynchronously, meaning this method will immediatly return.
         * Depending on which discoverer modules where provided, this might or might not work
         * @param entryPoint What to discover.
         */
        virtual void discover( const std::string& entryPoint ) = 0;
        virtual const std::string& snapshotPath() const = 0;
        virtual void setLogger( ILogger* logger ) = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
