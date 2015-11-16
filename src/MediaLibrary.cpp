/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include <algorithm>
#include <functional>
#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "discoverer/DiscovererWorker.h"
#include "Media.h"
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
#include "database/SqliteConnection.h"
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

const std::vector<std::string> MediaLibrary::supportedVideoExtensions {
    // Videos
    "avi", "3gp", "amv", "asf", "divx", "dv", "flv", "gxf",
    "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
    "mp2", "mp4", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps",
    "rec", "rm", "rmvb", "tod", "ts", "vob", "vro", "webm", "wmv"
};

const std::vector<std::string> MediaLibrary::supportedAudioExtensions {
    // Audio
    "a52", "aac", "ac3", "aiff", "amr", "aob", "ape",
    "dts", "flac", "it", "m4a", "m4p", "mid", "mka", "mlp",
    "mod", "mp1", "mp2", "mp3", "mpc", "oga", "ogg", "oma",
    "rmi", "s3m", "spx", "tta", "voc", "vqf", "w64", "wav",
    "wma", "wv", "xa", "xm"
};

MediaLibrary::MediaLibrary()
    : m_discoverer( new DiscovererWorker )
    , m_verbosity( LogLevel::Error )
{
    Log::setLogLevel( m_verbosity );
}

MediaLibrary::~MediaLibrary()
{
    // The log callback isn't shared by all VLC::Instance's, yet since
    // they all share a single libvlc_instance_t, any VLC::Instance still alive
    // with a log callback set will try to invoke it.
    // We manually call logUnset to ensure that the callback that is about to be deleted will
    // not be called anymore.
    if ( m_vlcInstance.isValid() )
        m_vlcInstance.logUnset();
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    m_discoverer->stop();
    Media::clear();
    Folder::clear();
    Label::clear();
    Album::clear();
    AlbumTrack::clear();
    Show::clear();
    ShowEpisode::clear();
    Movie::clear();
    VideoTrack::clear();
    AudioTrack::clear();
    Artist::clear();
    // Explicitely release the connection's TLS
    if ( m_dbConnection != nullptr )
        m_dbConnection->release();
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
    m_dbConnection.reset( new SqliteConnection( dbPath ) );
    m_parser.reset( new Parser( m_dbConnection.get(), m_callback ) );

    if ( mlCallback != nullptr )
    {
        const char* args[] = {
            "-vv",
        };
        m_vlcInstance = VLC::Instance( sizeof(args) / sizeof(args[0]), args );
        m_vlcInstance.logSet([this](int lvl, const libvlc_log_t*, std::string msg) {
            if ( m_verbosity != LogLevel::Verbose )
                return ;
            if ( lvl == LIBVLC_ERROR )
                Log::Error( msg );
            else if ( lvl == LIBVLC_WARNING )
                Log::Warning( msg );
            else
                Log::Info( msg );
        });

        auto vlcService = std::unique_ptr<VLCMetadataService>( new VLCMetadataService( m_vlcInstance, m_dbConnection.get(), m_fsFactory ) );
        auto thumbnailerService = std::unique_ptr<VLCThumbnailer>( new VLCThumbnailer( m_vlcInstance ) );
        addMetadataService( std::move( vlcService ) );
        addMetadataService( std::move( thumbnailerService ) );
    }

    auto t = m_dbConnection->newTransaction();
    if ( ! ( Media::createTable( m_dbConnection.get() ) &&
        Folder::createTable( m_dbConnection.get() ) &&
        Label::createTable( m_dbConnection.get() ) &&
        Album::createTable( m_dbConnection.get() ) &&
        AlbumTrack::createTable( m_dbConnection.get() ) &&
        Show::createTable( m_dbConnection.get() ) &&
        ShowEpisode::createTable( m_dbConnection.get() ) &&
        Movie::createTable( m_dbConnection.get() ) &&
        VideoTrack::createTable( m_dbConnection.get() ) &&
        AudioTrack::createTable( m_dbConnection.get() ) &&
        Artist::createTable( m_dbConnection.get() ) &&
        Artist::createDefaultArtists( m_dbConnection.get() ) ) )
    {
        LOG_ERROR( "Failed to create database structure" );
        return false;
    }
    t->commit();
    m_discoverer->setCallback( m_callback );
    m_discoverer->addDiscoverer( std::unique_ptr<IDiscoverer>( new FsDiscoverer( m_fsFactory, this, m_dbConnection.get() ) ) );
    m_discoverer->reload();
    m_parser->start();
    return true;
}

void MediaLibrary::setVerbosity(LogLevel v)
{
    m_verbosity = v;
    Log::setLogLevel( v );
}

std::vector<MediaPtr> MediaLibrary::files()
{
    return Media::fetchAll<IMedia>( m_dbConnection.get() );
}

std::vector<MediaPtr> MediaLibrary::audioFiles()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE type = ? ORDER BY title";
    return Media::fetchAll<IMedia>( m_dbConnection.get(), req, IMedia::Type::AudioType );
}

std::vector<MediaPtr> MediaLibrary::videoFiles()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE type = ? ORDER BY title";
    return Media::fetchAll<IMedia>( m_dbConnection.get(), req, IMedia::Type::VideoType );
}

MediaPtr MediaLibrary::file( const std::string& path )
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name +
            " WHERE mrl = ?";
    return Media::fetch( m_dbConnection.get(), req, path );
}

std::shared_ptr<Media> MediaLibrary::addFile( const std::string& path, FolderPtr parentFolder )
{
    LOG_INFO( "Adding ", path );
    std::unique_ptr<fs::IFile> file;
    try
    {
        file = m_fsFactory->createFile( path );
    }
    catch (std::exception& ex)
    {
        LOG_ERROR( "Failed to create an IFile for ", path, ": ", ex.what() );
        return nullptr;
    }

    auto type = IMedia::Type::UnknownType;
    auto ext = file->extension();
    auto predicate = [ext](const std::string& v) {
        return strcasecmp(v.c_str(), ext.c_str()) == 0;
    };

    if ( std::find_if( begin( supportedVideoExtensions ), end( supportedVideoExtensions ),
                    predicate ) != end( supportedVideoExtensions ) )
    {
        type = IMedia::Type::VideoType;
    }
    else if ( std::find_if( begin( supportedAudioExtensions ), end( supportedAudioExtensions ),
                         predicate ) != end( supportedAudioExtensions ) )
    {
        type = IMedia::Type::AudioType;
    }
    if ( type == IMedia::Type::UnknownType )
        return nullptr;

    auto fptr = Media::create( m_dbConnection.get(), type, file.get(), parentFolder != nullptr ? parentFolder->id() : 0 );
    if ( fptr == nullptr )
    {
        LOG_ERROR( "Failed to add file ", file->fullPath(), " to the media library" );
        return nullptr;
    }
    if ( m_callback != nullptr )
        m_callback->onFileAdded( fptr );
    m_parser->parse( fptr );
    return fptr;
}

FolderPtr MediaLibrary::folder( const std::string& path )
{
    return Folder::fromPath( m_dbConnection.get(), path );
}

bool MediaLibrary::deleteFile( const Media* file )
{
    return Media::destroy( m_dbConnection.get(), file->id() );
}

bool MediaLibrary::deleteFolder( FolderPtr folder )
{
    if ( Folder::destroy( m_dbConnection.get(), folder->id() ) == false )
        return false;
    Media::clear();
    return true;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    return Label::create( m_dbConnection.get(), label );
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    return Label::destroy( m_dbConnection.get(), label->id() );
}

AlbumPtr MediaLibrary::album( unsigned int id )
{
    return Album::fetch( m_dbConnection.get(), id );
}

std::shared_ptr<Album> MediaLibrary::createAlbum(const std::string& title )
{
    return Album::create( m_dbConnection.get(), title );
}

std::vector<AlbumPtr> MediaLibrary::albums()
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
            " ORDER BY title ASC";
    return Album::fetchAll<IAlbum>( m_dbConnection.get(), req );
}

ShowPtr MediaLibrary::show(const std::string& name)
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return Show::fetch( m_dbConnection.get(), req, name );
}

std::shared_ptr<Show> MediaLibrary::createShow(const std::string& name)
{
    return Show::create( m_dbConnection.get(), name );
}

MoviePtr MediaLibrary::movie( const std::string& title )
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return Movie::fetch( m_dbConnection.get(), req, title );
}

std::shared_ptr<Movie> MediaLibrary::createMovie( const std::string& title )
{
    return Movie::create( m_dbConnection.get(), title );
}

ArtistPtr MediaLibrary::artist(unsigned int id)
{
    return Artist::fetch( m_dbConnection.get(), id );
}

ArtistPtr MediaLibrary::artist( const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name
            + " WHERE name = ?";
    return Artist::fetch( m_dbConnection.get(), req, name );
}

std::shared_ptr<Artist> MediaLibrary::createArtist( const std::string& name )
{
    return Artist::create( m_dbConnection.get(), name );
}

std::vector<ArtistPtr> MediaLibrary::artists() const
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name +
            " WHERE nb_albums > 0";
    return Artist::fetchAll<IArtist>( m_dbConnection.get(), req );
}

void MediaLibrary::addMetadataService(std::unique_ptr<IMetadataService> service)
{
    if ( service->initialize( m_parser.get(), this ) == false )
    {
        //FIXME: This is missing a name or something more specific
        LOG_ERROR( "Failed to initialize service" );
        return;
    }
    m_parser->addService( std::move( service ) );
}

void MediaLibrary::reload()
{
    m_discoverer->reload();
}

void MediaLibrary::pauseBackgroundOperations()
{
    m_parser->pause();
}

void MediaLibrary::resumeBackgroundOperations()
{
    m_parser->resume();
}

void MediaLibrary::discover( const std::string &entryPoint )
{
    m_discoverer->discover( entryPoint );
}

const std::string& MediaLibrary::snapshotPath() const
{
    return m_snapshotPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

