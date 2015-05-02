#include <algorithm>
#include <functional>
#include <queue>

#include "Album.h"
#include "AlbumTrack.h"
#include "AudioTrack.h"
#include "File.h"
#include "Folder.h"
#include "MediaLibrary.h"
#include "IMetadataService.h"
#include "Label.h"
#include "Movie.h"
#include "Parser.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "database/SqliteTools.h"
#include "Utils.h"
#include "VideoTrack.h"

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

bool MediaLibrary::initialize( const std::string& dbPath, std::shared_ptr<factory::IFileSystem> fsFactory )
{
    if ( fsFactory != nullptr )
        m_fsFactory = fsFactory;
    else
        m_fsFactory.reset( new factory::FileSystemDefaultFactory );

    sqlite3* dbConnection;
    int res = sqlite3_open( dbPath.c_str(), &dbConnection );
    if ( res != SQLITE_OK )
        return false;
    m_dbConnection.reset( dbConnection, &sqlite3_close );
    if ( sqlite::Tools::executeRequest( DBConnection(m_dbConnection), "PRAGMA foreign_keys = ON" ) == false )
        return false;
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
        std::cerr << "Failed to create database structure" << std::endl;
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
    auto file = File::create( m_dbConnection, path, 0 );
    if ( file == nullptr )
        return nullptr;
    return file;
}

FolderPtr MediaLibrary::addFolder( const std::string& path )
{
    std::queue<std::pair<std::string, unsigned int>> folders;
    FolderPtr root;

    folders.emplace( path, 0 );
    while ( folders.empty() == false )
    {
        std::unique_ptr<fs::IDirectory> dir;
        auto currentFolder = folders.front();
        folders.pop();

        try
        {
            dir = m_fsFactory->createDirectory( currentFolder.first );
        }
        catch ( std::runtime_error& )
        {
            // If the first directory fails to open, stop now.
            // Otherwise, assume something went wrong in a subdirectory.
            if (root == nullptr)
                return nullptr;
            continue;
        }

        auto folder = Folder::create( m_dbConnection, dir->path(), currentFolder.second, dir->lastModificationDate() );
        if ( folder == nullptr && root == nullptr )
            return nullptr;
        if ( root == nullptr )
            root = folder;

        for ( auto& f : dir->files() )
        {
            if ( std::find( begin( supportedExtensions ), end( supportedExtensions ),
                            utils::file::extension( f ) ) == end( supportedExtensions ) )
                continue;
            if ( File::create( m_dbConnection, f, folder->id() ) == nullptr )
                std::cerr << "Failed to add file " << f << " to the media library" << std::endl;
        }
        for ( auto& f : dir->dirs() )
            folders.emplace( f, folder->id() );
    }
    return root;
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

void MediaLibrary::addMetadataService(IMetadataService* service)
{
    service->initialize( m_parser.get(), this );
    m_parser->addService( service );
}

void MediaLibrary::parse(FilePtr file , IParserCb* cb)
{
    m_parser->parse( file, cb );
}

bool MediaLibrary::loadFolders()
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name
            + " WHERE id_parent IS NULL";
    auto rootFolders = sqlite::Tools::fetchAll<Folder, IFolder>( m_dbConnection, req );
    if ( rootFolders.size() == 0 )
        return true;
    for ( const auto f : rootFolders )
    {
        auto folder = m_fsFactory->createDirectory( f->path() );
        if ( folder->lastModificationDate() == f->lastModificationDate() )
            continue;
        checkSubfolders( folder.get(), f->id() );
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
            addFolder( subFolderPath );
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
        subFoldersInDB.erase( it );
    }
    // Now all folders we had in DB but haven't seen from the FS must have been deleted.
    for ( auto f : subFoldersInDB )
    {
        deleteFolder( f );
    }
    return true;
}

