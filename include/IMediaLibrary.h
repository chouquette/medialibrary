#ifndef IMEDIALIBRARY_H
#define IMEDIALIBRARY_H

#include <vector>
#include <string>
#include <memory>

class IAlbum;
class IAlbumTrack;
class IFile;
class ILabel;
class IMetadataService;
class IShow;

typedef std::shared_ptr<IFile> FilePtr;
typedef std::shared_ptr<ILabel> LabelPtr;
typedef std::shared_ptr<IAlbum> AlbumPtr;
typedef std::shared_ptr<IAlbumTrack> AlbumTrackPtr;
typedef std::shared_ptr<IShow> ShowPtr;

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
        virtual AlbumPtr album( const std::string& id3Tag ) = 0;
        virtual AlbumPtr createAlbum( const std::string& id3Tag ) = 0;
        virtual ShowPtr show( const std::string& name ) = 0;
        virtual ShowPtr createShow( const std::string& name ) = 0;

        virtual void addMetadataService( IMetadataService* service ) = 0;
};

class MediaLibraryFactory
{
    public:
        static IMediaLibrary* create();
};

#endif // IMEDIALIBRARY_H
