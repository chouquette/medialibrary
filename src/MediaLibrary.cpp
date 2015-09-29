#include <algorithm>
#include <functional>
#include "Album.h"
#include "AlbumTrack.h"
#include "AudioTrack.h"
#include "File.h"
#include "Folder.h"
#include "MediaLibrary.h"
#include "IMetadataService.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "Parser.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "Utils.h"
#include "VideoTrack.h"

// Discoverers:
#include "discoverer/FsDiscoverer.h"

// Metadata services:
#include "metadata_services/vlc/VLCMetadataService.h"
#include "metadata_services/vlc/VLCThumbnailer.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "factory/FileSystem.h"

const std::vector<std::string> MediaLibrary::supportedExtensions {
    // Videos
    "avi", "3gp", "amv", "asf", "divx", "dv", "flv", "gxf",
    "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
    "mp2", "mp4", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps",
    "rec", "rm", "rmvb", "tod", "ts", "vob", "vro", "webm", "wmv",
    // Images
    "png", "jpg", "jpeg",
    // Audio
    "a52", "aac", "ac3", "aiff", "amr", "aob", "ape",
    "dts", "flac", "it", "m4a", "m4p", "mid", "mka", "mlp",
    "mod", "mp1", "mp2", "mp3", "mpc", "oga", "ogg", "oma",
    "rmi", "s3m", "spx", "tta", "voc", "vqf", "w64", "wav",
    "wma", "wv", "xa", "xm"
};

MediaLibrary::MediaLibrary()
    : m_parser( new Parser )
{
}

MediaLibrary::~MediaLibrary()
{
    File::clear();
    Folder::clear();
    Label::clear();
    Album::clear();
    AlbumTrack::clear();
    Show::clear();
    ShowEpisode::clear();
    Movie::clear();
    VideoTrack::clear();
    AudioTrack::clear();
}

void MediaLibrary::setFsFactory(std::shared_ptr<factory::IFileSystem> fsFactory)
{
    m_fsFactory = fsFactory;
}

bool MediaLibrary::initialize( const std::string& dbPath, const std::string& snapshotPath, IMediaLibraryCb* mlCallback )
{
    if ( m_fsFactory == nullptr )
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );
    m_snapshotPath = snapshotPath;
    m_callback = mlCallback;

    if ( mlCallback != nullptr )
    {
        const char* args[] = {
            "-vv",
            "--vout=dummy",
        };
        VLC::Instance vlcInstance( sizeof(args) / sizeof(args[0]), args );
        auto vlcService = std::unique_ptr<VLCMetadataService>( new VLCMetadataService( vlcInstance ) );
        auto thumbnailerService = std::unique_ptr<VLCThumbnailer>( new VLCThumbnailer( vlcInstance ) );
        addMetadataService( std::move( vlcService ) );
        addMetadataService( std::move( thumbnailerService ) );
    }

    m_discoverers.emplace_back( new FsDiscoverer( m_fsFactory, this ) );

    sqlite3* dbConnection;
    int res = sqlite3_open( dbPath.c_str(), &dbConnection );
    if ( res != SQLITE_OK )
        return false;
    m_dbConnection.reset( dbConnection, &sqlite3_close );
    if ( sqlite::Tools::executeRequest( DBConnection(m_dbConnection), "PRAGMA foreign_keys = ON" ) == false )
    {
        LOG_ERROR( "Failed to enable foreign keys" );
        return false;
    }
    if ( ! ( File::createTable( m_dbConnection ) &&
        Folder::createTable( m_dbConnection ) &&
        Label::createTable( m_dbConnection ) &&
        Album::createTable( m_dbConnection ) &&
        AlbumTrack::createTable( m_dbConnection ) &&
        Show::createTable( m_dbConnection ) &&
        ShowEpisode::createTable( m_dbConnection ) &&
        Movie::createTable( m_dbConnection ) &&
        VideoTrack::createTable( m_dbConnection ) &&
        AudioTrack::createTable( m_dbConnection ) ) )
    {
        LOG_ERROR( "Failed to create database structure" );
        return false;
    }
    return loadFolders();
}

std::vector<FilePtr> MediaLibrary::files()
{
    return File::fetchAll( m_dbConnection );
}

FilePtr MediaLibrary::file( const std::string& path )
{
    return File::fetch( m_dbConnection, path );
}

FilePtr MediaLibrary::addFile( const std::string& path )
{
    auto fsFile = m_fsFactory->createFile( path );
    return addFile( fsFile.get(), 0 );
}

FolderPtr MediaLibrary::folder( const std::string& path )
{
    return Folder::fetch( m_dbConnection, path );
}

bool MediaLibrary::deleteFile( const std::string& mrl )
{
    return File::destroy( m_dbConnection, mrl );
}

bool MediaLibrary::deleteFile( FilePtr file )
{
    return File::destroy( m_dbConnection, std::static_pointer_cast<File>( file ) );
}

bool MediaLibrary::deleteFolder( FolderPtr folder )
{
    if ( Folder::destroy( m_dbConnection, std::static_pointer_cast<Folder>( folder ) ) == false )
        return false;
    File::clear();
    return true;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    return Label::create( m_dbConnection, label );
}

bool MediaLibrary::deleteLabel( const std::string& text )
{
    return Label::destroy( m_dbConnection, text );
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    return Label::destroy( m_dbConnection, std::static_pointer_cast<Label>( label ) );
}

AlbumPtr MediaLibrary::album(const std::string& title )
{
    // We can't use Cache helper, since albums are cached by primary keys
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " WHERE title = ?";
    return sqlite::Tools::fetchOne<Album>( DBConnection( m_dbConnection ), req, title );
}

AlbumPtr MediaLibrary::createAlbum(const std::string& title )
{
    return Album::create( m_dbConnection, title );
}

ShowPtr MediaLibrary::show(const std::string& name)
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return sqlite::Tools::fetchOne<Show>( m_dbConnection, req, name );
}

ShowPtr MediaLibrary::createShow(const std::string& name)
{
    return Show::create( m_dbConnection, name );
}

MoviePtr MediaLibrary::movie( const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return sqlite::Tools::fetchOne<Movie>( m_dbConnection, req, title );
}

MoviePtr MediaLibrary::createMovie( const std::string& title )
{
    return Movie::create( m_dbConnection, title );
}

void MediaLibrary::addMetadataService(std::unique_ptr<IMetadataService> service)
{
    if ( service->initialize( m_parser.get(), this ) == false )
    {
        std::cout << "Failed to initialize service" << std::endl;
        return;
    }
    m_parser->addService( std::move( service ) );
}

void MediaLibrary::discover( const std::string &entryPoint )
{
    std::thread t([this, entryPoint] {
        //FIXME: This will crash if the media library gets deleted while we
        //are discovering.
        if ( m_callback != nullptr )
            m_callback->onDiscoveryStarted( entryPoint );

        for ( auto& d : m_discoverers )
            d->discover( entryPoint );

        if ( m_callback != nullptr )
            m_callback->onDiscoveryCompleted( entryPoint );
    });
    t.detach();
}

FolderPtr MediaLibrary::onNewFolder( const fs::IDirectory* directory, FolderPtr parent )
{
    //FIXME: Since we insert files/folders with a UNIQUE constraint, maybe we should
    //just let sqlite try to insert, throw an exception in case the contraint gets violated
    //catch it and return nullptr from here.
    //We previously were fetching the folder manually here, but that triggers an eroneous entry
    //in the cache. This might also be something to fix...
    return Folder::create( m_dbConnection, directory,
                        parent == nullptr ? 0 : parent->id() );
}

FilePtr MediaLibrary::onNewFile( const fs::IFile *file, FolderPtr parent )
{
    //FIXME: Same uniqueness comment as onNewFolder above.
    return addFile( file, parent == nullptr ? 0 : parent->id() );
}

const std::string& MediaLibrary::snapshotPath() const
{
    return m_snapshotPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

bool MediaLibrary::loadFolders()
{
    //FIXME: This should probably be in a sql transaction
    //FIXME: This shouldn't be done for "removable"/network files
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent IS NULL";
    auto rootFolders = sqlite::Tools::fetchAll<Folder, IFolder>( m_dbConnection, req );
    for ( const auto f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder->lastModificationDate() == f->lastModificationDate() )
            continue;
        checkSubfolders( folder.get(), f->id() );
        f->setLastModificationDate( folder->lastModificationDate() );
    }
    return true;
}

bool MediaLibrary::checkSubfolders( fs::IDirectory* folder, unsigned int parentId )
{
    // From here we can have:
    // - New subfolder(s)
    // - Deleted subfolder(s)
    // - New file(s)
    // - Deleted file(s)
    // - Changed file(s)
    // ... in this folder, or in all the sub folders.

    // Load the folders we already know of:
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent = ?";
    auto subFoldersInDB = sqlite::Tools::fetchAll<Folder, IFolder>( m_dbConnection, req, parentId );
    for ( const auto& subFolderPath : folder->dirs() )
    {
        auto it = std::find_if( begin( subFoldersInDB ), end( subFoldersInDB ), [subFolderPath](const std::shared_ptr<IFolder>& f) {
            return f->path() == subFolderPath;
        });
        // We don't know this folder, it's a new one
        if ( it == end( subFoldersInDB ) )
        {
            //FIXME: In order to add the new folder, we need to use the same discoverer.
            // This probably means we need to store which discoverer was used to add which file
            // and store discoverers as a map instead of a vector
            continue;
        }
        auto subFolder = m_fsFactory->createDirectory( subFolderPath );
        if ( subFolder->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Remove all folders that still exist in FS. That way, the list of folders that
            // will still be in subFoldersInDB when we're done is the list of folders that have
            // been deleted from the FS
            subFoldersInDB.erase( it );
            continue;
        }
        // This folder was modified, let's recurse
        checkSubfolders( subFolder.get(), (*it)->id() );
        checkFiles( subFolder.get(), (*it)->id() );
        (*it)->setLastModificationDate( subFolder->lastModificationDate() );
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        std::cout << "Folder " << f->path() << " not found in FS, deleting it" << std::endl;
        deleteFolder( f );
    }
    return true;
}

void MediaLibrary::checkFiles( fs::IDirectory* folder, unsigned int parentId )
{
    static const std::string req = "SELECT * FROM " + policy::FileTable::Name
            + " WHERE folder_id = ?";
    auto files = sqlite::Tools::fetchAll<File, IFile>( m_dbConnection, req, parentId );
    for ( const auto& filePath : folder->files() )
    {
        auto file = m_fsFactory->createFile( filePath );
        auto it = std::find_if( begin( files ), end( files ), [filePath](const std::shared_ptr<IFile>& f) {
            return f->mrl() == filePath;
        });
        if ( it == end( files ) )
        {
            addFile( file.get(), parentId );
            continue;
        }
        if ( file->lastModificationDate() == (*it)->lastModificationDate() )
        {
            // Unchanged file
            files.erase( it );
            continue;
        }
        deleteFile( filePath );
        addFile( file.get(), parentId );
        files.erase( it );
    }
    for ( auto file : files )
    {
        deleteFile( file );
    }
}

FilePtr MediaLibrary::addFile( const fs::IFile* file, unsigned int folderId )
{
    if ( std::find( begin( supportedExtensions ), end( supportedExtensions ),
                    file->extension() ) == end( supportedExtensions ) )
        return false;
    auto fptr = File::create( m_dbConnection, file, folderId );
    if ( fptr == nullptr )
    {
        LOG_ERROR( "Failed to add file ", file->fullPath(), " to the media library" );
        return nullptr;
    }
    // Keep in mind that this is queued by the parser thread, there is no waranty about
    // when the metadata will be available
    if ( m_callback != nullptr )
        m_callback->onFileAdded( fptr );
    m_parser->parse( fptr, m_callback );
    return fptr;
}

