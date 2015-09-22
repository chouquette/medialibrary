#ifndef MEDIALIBRARY_H
#define MEDIALIBRARY_H

class Parser;

#include <sqlite3.h>

#include "IMediaLibrary.h"
#include "IDiscoverer.h"

class MediaLibrary : public IMediaLibrary
{
    public:
        MediaLibrary();
        ~MediaLibrary();
        virtual bool initialize( const std::string& dbPath, std::shared_ptr<factory::IFileSystem> fsFactory );

        virtual std::vector<FilePtr> files();
        virtual FilePtr file( const std::string& path );
        virtual FilePtr addFile( const std::string& path );
        virtual bool deleteFile( const std::string& mrl );
        virtual bool deleteFile( FilePtr file );

        virtual FolderPtr folder( const std::string& path ) override;
        virtual bool deleteFolder( FolderPtr folder ) override;

        virtual LabelPtr createLabel( const std::string& label );
        virtual bool deleteLabel(const std::string& text );
        virtual bool deleteLabel( LabelPtr label );

        virtual AlbumPtr album( const std::string& title );
        virtual AlbumPtr createAlbum( const std::string& title );

        virtual ShowPtr show( const std::string& name );
        virtual ShowPtr createShow( const std::string& name );

        virtual MoviePtr movie( const std::string& title );
        virtual MoviePtr createMovie( const std::string& title );

        virtual void addMetadataService( std::unique_ptr<IMetadataService> service );
        virtual void parse( FilePtr file, IParserCb* cb );

        virtual void addDiscoverer( std::unique_ptr<IDiscoverer> discoverer ) override;
        virtual void discover( const std::string& entryPoint ) override;
        // IDiscovererCb implementation
        virtual FolderPtr onNewFolder( const fs::IDirectory* directory, FolderPtr parent ) override;
        virtual FilePtr onNewFile(const fs::IFile* file, FolderPtr parent ) override;

    private:
        static const std::vector<std::string> supportedExtensions;

    private:
        bool loadFolders();
        bool checkSubfolders( fs::IDirectory* folder, unsigned int parentId );
        void checkFiles( fs::IDirectory* folder, unsigned int parentId );
        FilePtr addFile( const fs::IFile* file, unsigned int folderId );

    private:
        std::shared_ptr<sqlite3> m_dbConnection;
        std::unique_ptr<Parser> m_parser;
        std::shared_ptr<factory::IFileSystem> m_fsFactory;
        std::vector<std::unique_ptr<IDiscoverer>> m_discoverers;
};
#endif // MEDIALIBRARY_H
