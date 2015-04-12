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
#include "Movie.h"
#include "Parser.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "SqliteTools.h"
#include "VideoTrack.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"

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

bool MediaLibrary::initialize(const std::string& dbPath)
{
    sqlite3* dbConnection;
    int res = sqlite3_open( dbPath.c_str(), &dbConnection );
    if ( res != SQLITE_OK )
        return false;
    m_dbConnection.reset( dbConnection, &sqlite3_close );
    if ( SqliteTools::executeRequest( DBConnection(m_dbConnection), "PRAGMA foreign_keys = ON" ) == false )
        return false;
    return ( File::createTable( m_dbConnection ) &&
        Folder::createTable( m_dbConnection ) &&
        Label::createTable( m_dbConnection ) &&
        Album::createTable( m_dbConnection ) &&
        AlbumTrack::createTable( m_dbConnection ) &&
        Show::createTable( m_dbConnection ) &&
        ShowEpisode::createTable( m_dbConnection ) &&
        Movie::createTable( m_dbConnection ) ) &&
        VideoTrack::createTable( m_dbConnection ) &&
        AudioTrack::createTable( m_dbConnection );
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
    auto folder = Folder::create( m_dbConnection, path );
    if ( folder == nullptr )
        return nullptr;
    std::unique_ptr<fs::IDirectory> dir;
    try
    {
        dir = fs::createDirectory( path );
    }
    catch ( std::runtime_error& )
    {
        return nullptr;
    }

    for ( auto& f : dir->files() )
    {
        if ( File::create( m_dbConnection, f->fullPath(), folder->id() ) == nullptr )
            std::cerr << "Failed to add file " << f->fullPath() << " to the media library" << std::endl;
    }
    return folder;
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
    return Folder::destroy( m_dbConnection, std::static_pointer_cast<Folder>( folder ) );
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
    return SqliteTools::fetchOne<Album>( DBConnection( m_dbConnection ), req, title );
}

AlbumPtr MediaLibrary::createAlbum(const std::string& title )
{
    return Album::create( m_dbConnection, title );
}

ShowPtr MediaLibrary::show(const std::string& name)
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return SqliteTools::fetchOne<Show>( m_dbConnection, req, name );
}

ShowPtr MediaLibrary::createShow(const std::string& name)
{
    return Show::create( m_dbConnection, name );
}

MoviePtr MediaLibrary::movie( const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return SqliteTools::fetchOne<Movie>( m_dbConnection, req, title );
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

