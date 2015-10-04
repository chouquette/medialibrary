#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

class Parser;

#include <sqlite3.h>

#include "IMediaLibrary.h"
#include "IDiscoverer.h"
#include "logging/Logger.h"
#include "vlcpp/vlc.hpp"

class MediaLibrary : public IMediaLibrary, public IDiscovererCb
{
    public:
        MediaLibrary();
        ~MediaLibrary();
        virtual bool initialize( const std::string& dbPath, const std::string& snapshotPath, IMediaLibraryCb* metadataCb );
        virtual void setFsFactory( std::shared_ptr<factory::IFileSystem> fsFactory ) override;

        virtual std::vector<FilePtr> files() override;
        virtual std::vector<FilePtr> audioFiles() override;
        virtual std::vector<FilePtr> videoFiles() override;
        virtual FilePtr file( const std::string& path ) override;
        virtual FilePtr addFile( const std::string& path ) override;
        virtual bool deleteFile( const std::string& mrl ) override;
        virtual bool deleteFile( FilePtr file ) override;

        virtual FolderPtr folder( const std::string& path ) override;
        virtual bool deleteFolder( FolderPtr folder ) override;

        virtual LabelPtr createLabel( const std::string& label ) override;
        virtual bool deleteLabel(const std::string& text ) override;
        virtual bool deleteLabel( LabelPtr label ) override;

        virtual AlbumPtr album( const std::string& title ) override;
        virtual AlbumPtr createAlbum( const std::string& title ) override;
        virtual std::vector<AlbumPtr> albums() override;

        virtual ShowPtr show( const std::string& name ) override;
        virtual ShowPtr createShow( const std::string& name ) override;

        virtual MoviePtr movie( const std::string& title ) override;
        virtual MoviePtr createMovie( const std::string& title ) override;

        virtual void discover( const std::string& entryPoint ) override;
        // IDiscovererCb implementation
        virtual FolderPtr onNewFolder( const fs::IDirectory* directory, FolderPtr parent ) override;
        virtual FilePtr onNewFile(const fs::IFile* file, FolderPtr parent ) override;

        virtual const std::string& snapshotPath() const override;
        virtual void setLogger( ILogger* logger ) override;

    private:
        static const std::vector<std::string> supportedVideoExtensions;
        static const std::vector<std::string> supportedAudioExtensions;

    private:
        bool loadFolders();
        bool checkSubfolders( fs::IDirectory* folder, unsigned int parentId );
        void checkFiles( fs::IDirectory* folder, unsigned int parentId );
        FilePtr addFile( const fs::IFile* file, unsigned int folderId );
        void addMetadataService( std::unique_ptr<IMetadataService> service );

    private:
        std::shared_ptr<sqlite3> m_dbConnection;
        std::shared_ptr<factory::IFileSystem> m_fsFactory;
        std::vector<std::unique_ptr<IDiscoverer>> m_discoverers;
        std::string m_snapshotPath;
        IMediaLibraryCb* m_callback;

        // This probably qualifies as a work around, but we need to keep the VLC::Instance
        // alive to be able to use the logging wrapper lambda
        VLC::Instance m_vlcInstance;

        // Keep the parser as last field.
        // The parser holds a (raw) pointer to the media library. When MediaLibrary's destructor gets called
        // it might still finish a few operations before exiting the parser thread. Those operations are
        // likely to require a valid MediaLibrary, which would be compromised if some fields have already been
        // deleted/destroyed.
        std::unique_ptr<Parser> m_parser;
};
#endif // MEDIALIBRARY_H
