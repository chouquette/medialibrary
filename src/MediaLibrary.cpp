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

#include "Fixup.h"

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "discoverer/DiscovererWorker.h"
#include "Media.h"
#include "Device.h"
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
#include "utils/Filename.h"
#include "VideoTrack.h"

// Discoverers:
#include "discoverer/FsDiscoverer.h"

// Metadata services:
#include "metadata_services/vlc/VLCMetadataService.h"
#include "metadata_services/vlc/VLCThumbnailer.h"

#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "filesystem/IDevice.h"
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

const uint32_t MediaLibrary::DbModelVersion = 2;

MediaLibrary::MediaLibrary()
    : m_verbosity( LogLevel::Error )
{
    Log::setLogLevel( m_verbosity );
}

MediaLibrary::~MediaLibrary()
{
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    if ( m_discoverer != nullptr )
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
    Device::clear();
    // Explicitely release the connection's TLS
    if ( m_dbConnection != nullptr )
        m_dbConnection->release();
}

void MediaLibrary::setFsFactory(std::shared_ptr<factory::IFileSystem> fsFactory)
{
    m_fsFactory = fsFactory;
}


bool MediaLibrary::createAllTables()
{
    auto t = m_dbConnection->newTransaction();
    auto res = Device::createTable( m_dbConnection.get() ) &&
        Folder::createTable( m_dbConnection.get() ) &&
        Media::createTable( m_dbConnection.get() ) &&
        Label::createTable( m_dbConnection.get() ) &&
        Album::createTable( m_dbConnection.get() ) &&
        AlbumTrack::createTable( m_dbConnection.get() ) &&
        Album::createTriggers( m_dbConnection.get() ) &&
        Show::createTable( m_dbConnection.get() ) &&
        ShowEpisode::createTable( m_dbConnection.get() ) &&
        Movie::createTable( m_dbConnection.get() ) &&
        VideoTrack::createTable( m_dbConnection.get() ) &&
        AudioTrack::createTable( m_dbConnection.get() ) &&
        Artist::createTable( m_dbConnection.get() ) &&
        Artist::createDefaultArtists( m_dbConnection.get() ) &&
        Artist::createTriggers( m_dbConnection.get() ) &&
        Settings::createTable( m_dbConnection.get() );
    if ( res == false )
        return false;
    t->commit();
    return true;
}

bool MediaLibrary::initialize( const std::string& dbPath, const std::string& thumbnailPath, IMediaLibraryCb* mlCallback )
{
    if ( m_fsFactory == nullptr )
        m_fsFactory.reset( new factory::FileSystemFactory );
    Folder::setFileSystemFactory( m_fsFactory );
    m_thumbnailPath = thumbnailPath;
    m_callback = mlCallback;
    m_dbConnection.reset( new SqliteConnection( dbPath ) );

    // We need to create the tables in order of triggers creation
    // Device is the "root of all evil". When a device is modified,
    // we will trigger an update on folder, which will trigger
    // an update on files, and so on.
    if ( createAllTables() == false )
    {
        LOG_ERROR( "Failed to create database structure" );
        return false;
    }
    if ( m_settings.load( m_dbConnection.get() ) == false )
        return false;
    if ( m_settings.dbModelVersion() != DbModelVersion )
    {
        if ( updateDatabaseModel( m_settings.dbModelVersion() ) == false )
            return false;
    }
    startDiscoverer();
    startParser();
    return true;
}

void MediaLibrary::setVerbosity(LogLevel v)
{
    m_verbosity = v;
    Log::setLogLevel( v );
}

std::vector<MediaPtr> MediaLibrary::files()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE is_present = 1";
    return Media::fetchAll<IMedia>( m_dbConnection.get(), req );
}

std::vector<MediaPtr> MediaLibrary::audioFiles()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE type = ? AND is_present = 1 ORDER BY title";
    return Media::fetchAll<IMedia>( m_dbConnection.get(), req, IMedia::Type::AudioType );
}

std::vector<MediaPtr> MediaLibrary::videoFiles()
{
    static const std::string req = "SELECT * FROM " + policy::MediaTable::Name + " WHERE type = ? AND is_present = 1 ORDER BY title";
    return Media::fetchAll<IMedia>( m_dbConnection.get(), req, IMedia::Type::VideoType );
}

std::shared_ptr<Media> MediaLibrary::addFile( const std::string& path, Folder& parentFolder, fs::IDirectory& parentFolderFs )
{
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

    LOG_INFO( "Adding ", path );
    auto fptr = Media::create( m_dbConnection.get(), type, file.get(), parentFolder.id(),
                               parentFolderFs.device()->isRemovable() );
    if ( fptr == nullptr )
    {
        LOG_ERROR( "Failed to add file ", file->fullPath(), " to the media library" );
        return nullptr;
    }
    if ( m_callback != nullptr )
        m_callback->onFileAdded( fptr );
    if ( m_parser != nullptr )
        m_parser->parse( fptr );
    return fptr;
}

bool MediaLibrary::deleteFile( const Media* file )
{
    return Media::destroy( m_dbConnection.get(), file->id() );
}

bool MediaLibrary::deleteFolder( const Folder* folder )
{
    if ( Folder::destroy( m_dbConnection.get(), folder->id() ) == false )
        return false;
    Media::clear();
    return true;
}

std::shared_ptr<Device> MediaLibrary::device( const std::string& uuid )
{
    return Device::fromUuid( m_dbConnection.get(), uuid );
}

std::shared_ptr<Device> MediaLibrary::addDevice( const std::string& uuid, bool isRemovable )
{
    return Device::create( m_dbConnection.get(), uuid, isRemovable );
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
            " WHERE is_present=1"
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
            + " WHERE name = ? AND is_present = 1";
    return Artist::fetch( m_dbConnection.get(), req, name );
}

std::shared_ptr<Artist> MediaLibrary::createArtist( const std::string& name )
{
    return Artist::create( m_dbConnection.get(), name );
}

std::vector<ArtistPtr> MediaLibrary::artists() const
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name +
            " WHERE nb_albums > 0 AND is_present = 1";
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

void MediaLibrary::startParser()
{
    m_parser.reset( new Parser( m_dbConnection.get(), m_callback ) );

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
    m_parser->start();
}

void MediaLibrary::startDiscoverer()
{
    m_discoverer.reset( new DiscovererWorker );
    m_discoverer->setCallback( m_callback );
    m_discoverer->addDiscoverer( std::unique_ptr<IDiscoverer>( new FsDiscoverer( m_fsFactory, this, m_dbConnection.get() ) ) );
    m_discoverer->reload();
}

bool MediaLibrary::updateDatabaseModel( unsigned int previousVersion )
{
    if ( previousVersion == 1 )
    {
        // Way too much differences, introduction of devices, and almost unused in the wild, just drop everything
        std::string req = "PRAGMA writable_schema = 1;"
                            "delete from sqlite_master;"
                            "PRAGMA writable_schema = 0;";
        if ( sqlite::Tools::executeRequest( m_dbConnection.get(), req ) == false )
            return false;
        if ( createAllTables() == false )
            return false;
        ++previousVersion;
    }
    // To be continued in the future!

    // Safety check: ensure we didn't forget a migration along the way
    assert( previousVersion == DbModelVersion );
    m_settings.setDbModelVersion( DbModelVersion );
    m_settings.save();
    return true;
}

void MediaLibrary::reload()
{
    if ( m_discoverer != nullptr )
        m_discoverer->reload();
}

void MediaLibrary::pauseBackgroundOperations()
{
    if ( m_parser != nullptr )
        m_parser->pause();
}

void MediaLibrary::resumeBackgroundOperations()
{
    if ( m_parser != nullptr )
        m_parser->resume();
}

void MediaLibrary::discover( const std::string &entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->discover( entryPoint );
}

bool MediaLibrary::ignoreFolder( const std::string& path )
{
    return Folder::blacklist( m_dbConnection.get(), path );
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

