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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <algorithm>
#include <functional>
#include <utility>
#include <sys/stat.h>
#include <unistd.h>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "discoverer/DiscovererWorker.h"
#include "discoverer/probe/CrawlerProbe.h"
#include "utils/ModificationsNotifier.h"
#include "Device.h"
#include "File.h"
#include "Folder.h"
#include "Genre.h"
#include "Media.h"
#include "MediaLibrary.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "parser/Parser.h"
#include "Playlist.h"
#include "Show.h"
#include "ShowEpisode.h"
#include "SubtitleTrack.h"
#include "Thumbnail.h"
#include "database/SqliteTools.h"
#include "database/SqliteConnection.h"
#include "database/SqliteQuery.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "VideoTrack.h"
#include "Metadata.h"
#include "parser/Task.h"
#include "utils/Charsets.h"

// Discoverers:
#include "discoverer/FsDiscoverer.h"

// Metadata services:
#ifdef HAVE_LIBVLC
#include "metadata_services/vlc/VLCMetadataService.h"
#endif
#include "metadata_services/MetadataParser.h"
#include "metadata_services/ThumbnailerWorker.h"

// FileSystem
#include "factory/DeviceListerFactory.h"
#include "factory/FileSystemFactory.h"

#ifdef HAVE_LIBVLC
#include "factory/NetworkFileSystemFactory.h"

#include <vlcpp/vlc.hpp>
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
#include "metadata_services/vlc/CoreThumbnailer.h"
using ThumbnailerType = medialibrary::CoreThumbnailer;
#else
#include "metadata_services/vlc/VmemThumbnailer.h"
using ThumbnailerType = medialibrary::VmemThumbnailer;
#endif
#endif

#include "medialibrary/filesystem/IDevice.h"

namespace medialibrary
{

const char* const MediaLibrary::supportedExtensions[] = {
    "3g2", "3gp", "a52", "aac", "ac3", "adx", "aif", "aifc",
    "aiff", "alac", "amr", "amv", "aob", "ape", "asf", "asx",
    "avi", "b4s", "conf", /*"cue",*/ "divx", "dts", "dv",
    "flac", "flv", "gxf", "ifo", "iso", "it", "itml",
    "m1v", "m2t", "m2ts", "m2v", "m3u", "m3u8", "m4a", "m4b",
    "m4p", "m4v", "mid", "mka", "mkv", "mlp", "mod", "mov",
    "mp1", "mp2", "mp3", "mp4", "mpc", "mpeg", "mpeg1", "mpeg2",
    "mpeg4", "mpg", "mts", "mxf", "nsv", "nuv", "oga", "ogg",
    "ogm", "ogv", "ogx", "oma", "opus", "pls", "ps", "qtl",
    "ram", "rec", "rm", "rmi", "rmvb", "s3m", "sdp", "spx",
    "tod", "trp", "ts", "tta", "vlc", "vob", "voc", "vqf",
    "vro", "w64", "wav", "wax", "webm", "wma", "wmv", "wmx",
    "wpl", "wv", "wvx", "xa", "xm", "xspf"
};

const size_t MediaLibrary::NbSupportedExtensions = sizeof(supportedExtensions) / sizeof(supportedExtensions[0]);

MediaLibrary::MediaLibrary()
    : m_callback( nullptr )
    , m_fsFactoryCb( this )
    , m_deviceListerCbImpl( this )
    , m_verbosity( LogLevel::Error )
    , m_settings( this )
    , m_initialized( false )
    , m_discovererIdle( true )
    , m_parserIdle( true )
{
    Log::setLogLevel( m_verbosity );
}

MediaLibrary::~MediaLibrary()
{
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->stop();
    if ( m_parser != nullptr )
        m_parser->stop();
}

void MediaLibrary::createAllTables()
{
    auto dbModelVersion = m_settings.dbModelVersion();
    // We need to create the tables in order of triggers creation
    // Device is the "root of all evil". When a device is modified,
    // we will trigger an update on folder, which will trigger
    // an update on files, and so on.

    Device::createTable( m_dbConnection.get() );
    Folder::createTable( m_dbConnection.get() );
    Thumbnail::createTable( m_dbConnection.get() );
    Media::createTable( m_dbConnection.get(), dbModelVersion );
    File::createTable( m_dbConnection.get() );
    Label::createTable( m_dbConnection.get() );
    Playlist::createTable( m_dbConnection.get(), dbModelVersion );
    Genre::createTable( m_dbConnection.get() );
    Album::createTable( m_dbConnection.get() );
    AlbumTrack::createTable( m_dbConnection.get() );
    Show::createTable( m_dbConnection.get() );
    ShowEpisode::createTable( m_dbConnection.get() );
    Movie::createTable( m_dbConnection.get() );
    VideoTrack::createTable( m_dbConnection.get() );
    AudioTrack::createTable( m_dbConnection.get() );
    Artist::createTable( m_dbConnection.get() );
    Artist::createDefaultArtists( m_dbConnection.get() );
    Settings::createTable( m_dbConnection.get() );
    parser::Task::createTable( m_dbConnection.get() );
    Metadata::createTable( m_dbConnection.get() );
    SubtitleTrack::createTable( m_dbConnection.get() );
}

void MediaLibrary::createAllTriggers()
{
    auto dbModelVersion = m_settings.dbModelVersion();
    Folder::createTriggers( m_dbConnection.get(), dbModelVersion );
    Album::createTriggers( m_dbConnection.get() );
    AlbumTrack::createTriggers( m_dbConnection.get() );
    Artist::createTriggers( m_dbConnection.get(), dbModelVersion );
    Media::createTriggers( m_dbConnection.get(), dbModelVersion );
    File::createTriggers( m_dbConnection.get() );
    Genre::createTriggers( m_dbConnection.get() );
    Playlist::createTriggers( m_dbConnection.get() );
    Label::createTriggers( m_dbConnection.get() );
    Show::createTriggers( m_dbConnection.get() );
}

void MediaLibrary::registerEntityHooks()
{
    if ( m_modificationNotifier == nullptr )
        return;

    m_dbConnection->registerUpdateHook( Media::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        m_modificationNotifier->notifyMediaRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Artist::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        m_modificationNotifier->notifyArtistRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Album::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        m_modificationNotifier->notifyAlbumRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Playlist::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        m_modificationNotifier->notifyPlaylistRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Genre::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        m_modificationNotifier->notifyGenreRemoval( rowId );
    });
}

bool MediaLibrary::validateSearchPattern( const std::string& pattern )
{
    return pattern.size() >= 3;
}

bool MediaLibrary::createThumbnailFolder( const std::string& thumbnailPath ) const
{
    auto paths = utils::file::splitPath( thumbnailPath, true );
#ifndef _WIN32
    std::string fullPath{ "/" };
#else
    std::string fullPath;
#endif
    while ( paths.empty() == false )
    {
        fullPath += paths.top();

#ifdef _WIN32
        // Don't try to create C: or various other drives
        if ( isalpha( fullPath[0] ) && fullPath[1] == ':' && fullPath.length() == 2 )
        {
            fullPath += "\\";
            paths.pop();
            continue;
        }
        auto wFullPath = charset::ToWide( fullPath.c_str() );
        if ( _wmkdir( wFullPath.get() ) != 0 )
#else
        if ( mkdir( fullPath.c_str(), S_IRWXU ) != 0 )
#endif
        {
            if ( errno != EEXIST )
                return false;
        }
        paths.pop();
#ifndef _WIN32
        fullPath += "/";
#else
        fullPath += "\\";
#endif
    }
    return true;
}

InitializeResult MediaLibrary::initialize( const std::string& dbPath,
                                           const std::string& thumbnailPath,
                                           IMediaLibraryCb* mlCallback )
{
    LOG_INFO( "Initializing medialibrary..." );
    if ( m_initialized == true )
    {
        LOG_INFO( "...Already initialized" );
        return InitializeResult::AlreadyInitialized;
    }
    if ( m_deviceLister == nullptr )
    {
        m_deviceLister = factory::createDeviceLister();
        if ( m_deviceLister == nullptr )
        {
            LOG_ERROR( "No available IDeviceLister was found." );
            return InitializeResult::Failed;
        }
    }
    addLocalFsFactory();
    populateNetworkFsFactories();
    m_thumbnailPath = utils::file::toFolderPath( thumbnailPath );
    if ( createThumbnailFolder( m_thumbnailPath ) == false )
    {
        LOG_ERROR( "Failed to create thumbnail directory (", m_thumbnailPath,
                    ": ", strerror( errno ) );
        return InitializeResult::Failed;
    }
    m_callback = mlCallback;
    m_dbConnection = sqlite::Connection::connect( dbPath );

    // Give a chance to test overloads to reject the creation of a notifier
    startDeletionNotifier();
    // Which allows us to register hooks, or not, depending on the presence of a notifier
    registerEntityHooks();

    auto res = InitializeResult::Success;
    try
    {
        auto t = m_dbConnection->newTransaction();
        createAllTables();
        if ( m_settings.load() == false )
        {
            LOG_ERROR( "Failed to load settings" );
            return InitializeResult::Failed;
        }
        createAllTriggers();
        t->commit();

        if ( m_settings.dbModelVersion() != Settings::DbModelVersion )
        {
            res = updateDatabaseModel( m_settings.dbModelVersion(), dbPath );
            if ( res == InitializeResult::Failed )
            {
                LOG_ERROR( "Failed to update database model" );
                return res;
            }
        }
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Can't initialize medialibrary: ", ex.what() );
        return InitializeResult::Failed;
    }
    m_initialized = true;
    LOG_INFO( "Successfuly initialized" );
    return res;
}

bool MediaLibrary::start()
{
    assert( m_initialized == true );
    if ( m_parser != nullptr )
        return false;

    for ( auto& fsFactory : m_fsFactories )
        refreshDevices( *fsFactory );
    // Now that we know which devices are plugged, check for outdated devices
    // Approximate 6 months for old device precision.
    Device::removeOldDevices( this, std::chrono::seconds{ 3600 * 24 * 30 * 6 } );
    Media::removeOldMedia( this, std::chrono::seconds{ 3600 * 24 * 30 * 6 } );

    startDiscoverer();
    if ( startParser() == false )
        return false;
    startThumbnailer();
    return true;
}

void MediaLibrary::setVerbosity( LogLevel v )
{
    m_verbosity = v;
    Log::setLogLevel( v );
}

MediaPtr MediaLibrary::media( int64_t mediaId ) const
{
    return Media::fetch( this, mediaId );
}

MediaPtr MediaLibrary::media( const std::string& mrl ) const
{
    LOG_INFO( "Fetching media from mrl: ", mrl );
    auto file = File::fromExternalMrl( this, mrl );
    if ( file != nullptr )
    {
        LOG_INFO( "Found external media: ", mrl );
        return file->media();
    }
    auto fsFactory = fsFactoryForMrl( mrl );
    if ( fsFactory == nullptr )
    {
        LOG_WARN( "Failed to create FS factory for path ", mrl );
        return nullptr;
    }
    auto device = fsFactory->createDeviceFromMrl( mrl );
    if ( device == nullptr )
    {
        LOG_WARN( "Failed to create a device associated with mrl ", mrl );
        return nullptr;
    }
    if ( device->isRemovable() == false )
        file = File::fromMrl( this, mrl );
    else
    {
        auto folder = Folder::fromMrl( this, utils::file::directory( mrl ) );
        if ( folder == nullptr )
        {
            LOG_WARN( "Failed to find folder containing ", mrl );
            return nullptr;
        }
        if ( folder->isPresent() == false )
        {
            LOG_INFO( "Found a folder containing ", mrl, " but it is not present" );
            return nullptr;
        }
        file = File::fromFileName( this, utils::file::fileName( mrl ), folder->id() );
    }
    if ( file == nullptr )
    {
        LOG_WARN( "Failed to fetch file for ", mrl, " (device ", device->uuid(), " was ",
                  device->isRemovable() ? "" : "NOT ", "removable)");
        return nullptr;
    }
    return file->media();
}

MediaPtr MediaLibrary::addExternalMedia( const std::string& mrl, IMedia::Type type )
{
    try
    {
        return sqlite::Tools::withRetries( 3, [this, &mrl, type]() -> MediaPtr {
            auto t = m_dbConnection->newTransaction();
            auto fileName = utils::file::fileName( mrl );
            auto media = Media::create( this, type, 0, 0,
                                        utils::url::decode( fileName ) );
            if ( media == nullptr )
                return nullptr;
            if ( media->addExternalMrl( mrl, IFile::Type::Main ) == nullptr )
                return nullptr;
            t->commit();
            return media;
        });
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to create external media: ", ex.what() );
        return nullptr;
    }
}

MediaPtr MediaLibrary::addExternalMedia( const std::string& mrl )
{
    return addExternalMedia( mrl, IMedia::Type::External );
}

MediaPtr MediaLibrary::addStream( const std::string& mrl )
{
    return addExternalMedia( mrl, IMedia::Type::Stream );
}

bool MediaLibrary::removeExternalMedia(MediaPtr media)
{
    if ( media->type() != Media::Type::External &&
         media->type() != Media::Type::Stream )
    {
        assert( !"Invalid media provided" );
        return false;
    }
    return Media::destroy( this, media->id() );
}

Query<IMedia> MediaLibrary::audioFiles( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Audio, params );
}

Query<IMedia> MediaLibrary::videoFiles( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Video, params );
}

bool MediaLibrary::isExtensionSupported( const char* ext )
{
    return std::binary_search( std::begin( supportedExtensions ),
        std::end( supportedExtensions ), ext, [](const char* l, const char* r) {
            return strcasecmp( l, r ) < 0;
        });
}

void MediaLibrary::onDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                      std::shared_ptr<Folder> parentFolder,
                                      std::shared_ptr<fs::IDirectory> parentFolderFs,
                                      std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist )
{
    auto mrl = fileFs->mrl();
    try
    {
        std::shared_ptr<parser::Task> task;
        if ( parentPlaylist.first == nullptr )
        {
            // Sqlite won't ensure uniqueness for Task with the same (mrl, parent_playlist_id)
            // when parent_playlist_id is null, so we have to ensure of it ourselves
            const std::string req = "SELECT * FROM " + parser::Task::Table::Name + " "
                    "WHERE mrl = ? AND parent_playlist_id IS NULL";
            task = parser::Task::fetch( this, req, mrl );
            if ( task != nullptr )
            {
                LOG_INFO( "Not creating duplicated task for mrl: ", mrl );
                return;
            }
        }
        task = parser::Task::create( this, std::move( fileFs ), std::move( parentFolder ),
                                     std::move( parentFolderFs ),
                                     std::move( parentPlaylist ) );
        if ( task != nullptr && m_parser != nullptr )
            m_parser->parse( task );
    }
    catch(sqlite::errors::ConstraintViolation& ex)
    {
        // Most likely the file is already scheduled and we restarted the
        // discovery after a crash.
        LOG_WARN( "Failed to insert ", mrl, ": ", ex.what(), ". "
                  "Assuming the file is already scheduled for discovery" );
    }
}

void MediaLibrary::onUpdatedFile( std::shared_ptr<File> file,
                                  std::shared_ptr<fs::IFile> fileFs )
{
    auto mrl = fileFs->mrl();
    try
    {
        auto task = parser::Task::create( this, std::move( file ), std::move( fileFs ) );
        if ( task != nullptr && m_parser != nullptr )
            m_parser->parse( std::move( task ) );
    }
    catch( const sqlite::errors::ConstraintViolation& ex )
    {
        // Most likely the file is already scheduled and we restarted the
        // discovery after a crash.
        LOG_WARN( "Failed to insert ", mrl, ": ", ex.what(), ". "
                  "Assuming the file is already scheduled for discovery" );
    }
}

bool MediaLibrary::deleteFolder( const Folder& folder )
{
    LOG_INFO( "deleting folder ", folder.mrl() );
    if ( Folder::destroy( this, folder.id() ) == false )
        return false;
    return true;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    try
    {
        return Label::create( this, label );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to create a label: ", ex.what() );
        return nullptr;
    }
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    try
    {
        return Label::destroy( this, label->id() );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to delete label: ", ex.what() );
        return false;
    }
}

AlbumPtr MediaLibrary::album( int64_t id ) const
{
    return Album::fetch( this, id );
}

std::shared_ptr<Album> MediaLibrary::createAlbum( const std::string& title, int64_t thumbnailId )
{
    return Album::create( this, title, thumbnailId );
}

Query<IAlbum> MediaLibrary::albums( const QueryParameters* params ) const
{
    return Album::listAll( this, params );
}

Query<IGenre> MediaLibrary::genres( const QueryParameters* params ) const
{
    return Genre::listAll( this, params );
}

GenrePtr MediaLibrary::genre( int64_t id ) const
{
    return Genre::fetch( this, id );
}

ShowPtr MediaLibrary::show( int64_t id ) const
{
    return Show::fetch( this, id );
}

std::shared_ptr<Show> MediaLibrary::createShow( const std::string& name )
{
    return Show::create( this, name );
}

Query<IShow> MediaLibrary::shows(const QueryParameters* params) const
{
    return Show::listAll( this, params );
}

MoviePtr MediaLibrary::movie( int64_t id ) const
{
    return Movie::fetch( this, id );
}

std::shared_ptr<Movie> MediaLibrary::createMovie( Media& media )
{
    auto movie = Movie::create( this, media.id() );
    media.setMovie( movie );
    media.save();
    return movie;
}

ArtistPtr MediaLibrary::artist( int64_t id ) const
{
    return Artist::fetch( this, id );
}

std::shared_ptr<Artist> MediaLibrary::createArtist( const std::string& name )
{
    return Artist::create( this, name );
}

Query<IArtist> MediaLibrary::artists( bool includeAll, const QueryParameters* params ) const
{
    return Artist::listAll( this, includeAll, params );
}

PlaylistPtr MediaLibrary::createPlaylist( const std::string& name )
{
    try
    {
        auto pl = Playlist::create( this, name );
        if ( pl != nullptr && m_modificationNotifier != nullptr )
            m_modificationNotifier->notifyPlaylistCreation( pl );
        return pl;
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to create a playlist: ", ex.what() );
        return nullptr;
    }
}

Query<IPlaylist> MediaLibrary::playlists( const QueryParameters* params )
{
    return Playlist::listAll( this, params );
}

PlaylistPtr MediaLibrary::playlist( int64_t id ) const
{
    return Playlist::fetch( this, id );
}

bool MediaLibrary::deletePlaylist( int64_t playlistId )
{
    try
    {
        return Playlist::destroy( this, playlistId );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to delete playlist: ", ex.what() );
        return false;
    }
}

Query<IMedia> MediaLibrary::history() const
{
    return Media::fetchHistory( this );
}

Query<IMedia> MediaLibrary::streamHistory() const
{
    return Media::fetchStreamHistory( this );
}

bool MediaLibrary::clearHistory()
{
    try
    {
        return sqlite::Tools::withRetries( 3, [this]() {
            Media::clearHistory( this );
            return true;
        });
    }
    catch ( sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to clear history: ", ex.what() );
        return false;
    }
}

Query<IMedia> MediaLibrary::searchMedia( const std::string& title,
                                                const QueryParameters* params ) const
{
    if ( validateSearchPattern( title ) == false )
        return {};
    return Media::search( this, title, params );
}

Query<IMedia> MediaLibrary::searchAudio( const std::string& pattern, const QueryParameters* params ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Media::search( this, pattern, IMedia::Type::Audio, params );
}

Query<IMedia> MediaLibrary::searchVideo( const std::string& pattern, const QueryParameters* params ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Media::search( this, pattern, IMedia::Type::Video, params );
}

Query<IPlaylist> MediaLibrary::searchPlaylists( const std::string& name,
                                                const QueryParameters* params ) const
{
    if ( validateSearchPattern( name ) == false )
        return {};
    return Playlist::search( this, name, params );
}

Query<IAlbum> MediaLibrary::searchAlbums( const std::string& pattern,
                                          const QueryParameters* params ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Album::search( this, pattern, params );
}

Query<IGenre> MediaLibrary::searchGenre( const std::string& genre,
                                         const QueryParameters* params ) const
{
    if ( validateSearchPattern( genre ) == false )
        return {};
    return Genre::search( this, genre, params );
}

Query<IArtist> MediaLibrary::searchArtists( const std::string& name, bool includeAll,
                                            const QueryParameters* params ) const
{
    if ( validateSearchPattern( name ) == false )
        return {};
    return Artist::search( this, name, includeAll, params );
}

Query<IShow> MediaLibrary::searchShows( const std::string& pattern,
                                        const QueryParameters* params ) const
{
    return Show::search( this, pattern, params );
}

SearchAggregate MediaLibrary::search( const std::string& pattern,
                                      const QueryParameters* params ) const
{
    SearchAggregate res;
    res.albums = searchAlbums( pattern, params );
    res.artists = searchArtists( pattern, true, params );
    res.genres = searchGenre( pattern, params );
    res.media = searchMedia( pattern, params );
    res.playlists = searchPlaylists( pattern, params );
    res.shows = searchShows( pattern, params );
    return res;
}

bool MediaLibrary::startParser()
{
    m_parser.reset( new parser::Parser( this ) );

    if ( m_services.size() == 0 )
    {
#ifdef HAVE_LIBVLC
        m_parser->addService( std::make_shared<parser::VLCMetadataService>() );
#else
        return false;
#endif
    }
    else
    {
        assert( m_services[0]->targetedStep() == parser::Step::MetadataExtraction );
        m_parser->addService( m_services[0] );
    }
    m_parser->addService( std::make_shared<parser::MetadataAnalyzer>() );
    m_parser->start();
    return true;
}

void MediaLibrary::startDiscoverer()
{
    m_discovererWorker.reset( new DiscovererWorker( this ) );
    for ( const auto& fsFactory : m_fsFactories )
    {
        std::unique_ptr<prober::CrawlerProbe> probePtr( new prober::CrawlerProbe{} );
        m_discovererWorker->addDiscoverer( std::unique_ptr<IDiscoverer>( new FsDiscoverer( fsFactory, this, m_callback,
                                                                                           std::move ( probePtr ) ) ) );
    }
}

void MediaLibrary::startDeletionNotifier()
{
    m_modificationNotifier.reset( new ModificationNotifier( this ) );
    m_modificationNotifier->start();
}

void MediaLibrary::startThumbnailer()
{
#ifdef HAVE_LIBVLC
    if ( m_thumbnailers.empty() == true )
        m_thumbnailers.push_back( std::make_shared<ThumbnailerType>( this ) );
#endif
    for ( const auto& thumbnailer : m_thumbnailers )
    {
        // For now this make little sense (as we are instantiating the same
        // object in a loop, but at some point we will have multiple thumbnailer,
        // or the thumbnailer worker will handle multiple IThumbnailer implementations
        m_thumbnailer = std::unique_ptr<ThumbnailerWorker>(
                    new ThumbnailerWorker( this, thumbnailer ) );
    }
}

void MediaLibrary::populateNetworkFsFactories()
{
#ifdef HAVE_LIBVLC
    m_externalNetworkFsFactories.emplace_back(
        std::make_shared<factory::NetworkFileSystemFactory>( "smb://", "dsm-sd" ) );
#endif
}

void MediaLibrary::addLocalFsFactory()
{
    m_fsFactories.emplace( begin( m_fsFactories ), std::make_shared<factory::FileSystemFactory>( m_deviceLister ) );
}

InitializeResult MediaLibrary::updateDatabaseModel( unsigned int previousVersion,
                                        const std::string& dbPath )
{
    LOG_INFO( "Updating database model from ", previousVersion, " to ", Settings::DbModelVersion );
    auto originalPreviousVersion = previousVersion;
    // Up until model 3, it's safer (and potentially more efficient with index changes) to drop the DB
    // It's also way simpler to implement
    // In case of downgrade, just recreate the database
    for ( auto i = 0u; i < 3; ++i )
    {
        try
        {
            bool needRescan = false;
            // Up until model 3, it's safer (and potentially more efficient with index changes) to drop the DB
            // It's also way simpler to implement
            // In case of downgrade, just recreate the database
            // We might also have some special cases for failed upgrade (see
            // comments below for per-version details)
            if ( previousVersion < 3 ||
                 previousVersion > Settings::DbModelVersion ||
                 previousVersion == 4 )
            {
                if( recreateDatabase( dbPath ) == false )
                    throw std::runtime_error( "Failed to recreate the database" );
                return InitializeResult::DbReset;
            }
            /**
             * Migration from 3 to 4 didn't happen so well and broke a few
             * users DB. So:
             * - Any v4 database will be dropped and recreated in v5
             * - Any v3 database will be upgraded to v5
             * V4 database is only used by VLC-android 2.5.6 // 2.5.8, which are
             * beta versions.
             */
            if ( previousVersion == 3 )
            {
                migrateModel3to5();
                previousVersion = 5;
            }
            if ( previousVersion == 5 )
            {
                migrateModel5to6();
                previousVersion = 6;
            }
            if ( previousVersion == 6 )
            {
                // Force a rescan to solve metadata analysis problems.
                // The insertion is fixed, but won't edit already inserted data.
                forceRescan();
                previousVersion = 7;
            }
            /**
             * V7 introduces artist.nb_tracks and an associated trigger to delete
             * artists when it has no track/album left.
             */
            if ( previousVersion == 7 )
            {
                migrateModel7to8();
                previousVersion = 8;
            }
            if ( previousVersion == 8 )
            {

                // Multiple changes justify the rescan:
                // - Changes in the way we chose to encode or not MRL, meaning
                //   potentially all MRL are wrong (more precisely, will
                //   mismatch what VLC expects, which makes playlist analysis
                //   break.
                // - Fix in the way we chose album candidates, meaning some
                //   albums were likely to be wrongfully created.
                needRescan = true;
                migrateModel8to9();
                previousVersion = 9;
            }
            if ( previousVersion == 9 )
            {
                needRescan = true;
                migrateModel9to10();
                previousVersion = 10;
            }
            if ( previousVersion == 10 )
            {
                needRescan = true;
                migrateModel10to11();
                previousVersion = 11;
            }
            if ( previousVersion == 11 )
            {
                parser::Task::recoverUnscannedFiles( this );
                previousVersion = 12;
            }
            if ( previousVersion == 12 )
            {
                migrateModel12to13();
                previousVersion = 13;
            }
            if ( previousVersion == 13 )
            {
                // We need to recreate many thumbnail records, and hopefully
                // generate better ones
                needRescan = true;
                migrateModel13to14( originalPreviousVersion );
                previousVersion = 14;
            }
            // To be continued in the future!

            if ( needRescan == true )
                forceRescan();

            // Safety check: ensure we didn't forget a migration along the way
            assert( previousVersion == Settings::DbModelVersion );
            m_settings.setDbModelVersion( Settings::DbModelVersion );
            if ( m_settings.save() == false )
                return InitializeResult::Failed;
            return InitializeResult::Success;
        }
        catch( const std::exception& ex )
        {
            LOG_ERROR( "An error occured during the database upgrade: ",
                       ex.what() );
        }
        catch( ... )
        {
            LOG_ERROR( "An unknown error occured during the database upgrade." );
        }
        LOG_WARN( "Retrying database migration, attempt ", i + 1, " / 3" );
    }
    LOG_ERROR( "Failed to upgrade database, recreating it" );
    for ( auto i = 0u; i < 3; ++i )
    {
        try
        {
            if( recreateDatabase( dbPath ) == true )
                return InitializeResult::DbReset;
        }
        catch( const std::exception& ex )
        {
            LOG_ERROR( "Failed to recreate database: ", ex.what() );
        }
        catch(...)
        {
            LOG_ERROR( "Unknown error while trying to recreate the database." );
        }
        LOG_WARN( "Retrying to recreate the database, attempt ", i + 1, " / 3" );
    }
    return InitializeResult::Failed;
}

bool MediaLibrary::recreateDatabase( const std::string& dbPath )
{
    // Close all active connections, flushes all previously run statements.
    m_dbConnection.reset();
    unlink( dbPath.c_str() );
    m_dbConnection = sqlite::Connection::connect( dbPath );
    createAllTables();
    // We dropped the database, there is no setting to be read anymore
    if( m_settings.load() == false )
        return false;
    return true;
}

void MediaLibrary::migrateModel3to5()
{
    /*
     * Disable Foreign Keys & recursive triggers to avoid cascading deletion
     * while remodeling the database into the transaction.
     */
    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    auto t = getConn()->newTransaction();
    // As SQLite do not allow us to remove or add some constraints,
    // we use the method described here https://www.sqlite.org/faq.html#q11
    std::string reqs[] = {
#               include "database/migrations/migration3-5.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );
    // Re-create triggers removed in the process
    Media::createTriggers( getConn(), 5 );
    Playlist::createTriggers( getConn() );
    t->commit();
}

void MediaLibrary::migrateModel5to6()
{
    std::string req = "DELETE FROM " + Media::Table::Name + " WHERE type = ?";
    sqlite::Tools::executeRequest( getConn(), req, Media::Type::Unknown );

    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    req = "UPDATE " + Media::Table::Name + " SET is_present = 1 WHERE is_present != 0";
    sqlite::Tools::executeRequest( getConn(), req );
}

void MediaLibrary::migrateModel7to8()
{
    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    auto t = getConn()->newTransaction();
    std::string reqs[] = {
#               include "database/migrations/migration7-8.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );
    // Re-create triggers removed in the process
    Artist::createTriggers( getConn(), 8u );
    Media::createTriggers( getConn(), 5 );
    File::createTriggers( getConn() );
    t->commit();
}

void MediaLibrary::migrateModel8to9()
{
    // A bug in a previous migration caused our triggers to be missing for the
    // first application run (after the migration).
    // This could have caused media associated to deleted files not to be
    // deleted as well, so let's do that now.
    const std::string req = "DELETE FROM " + Media::Table::Name + " "
            "WHERE id_media IN "
            "(SELECT id_media FROM " + Media::Table::Name + " m LEFT JOIN " +
                File::Table::Name + " f ON f.media_id = m.id_media "
                "WHERE f.media_id IS NULL)";

    // Don't check for the return value, we don't mind if nothing deleted.
    // Quite the opposite actually :)
    sqlite::Tools::executeDelete( getConn(), req );
}

void MediaLibrary::migrateModel9to10()
{
    const std::string req = "SELECT * FROM " + File::Table::Name +
            " WHERE mrl LIKE '%#%%' ESCAPE '#'";
    auto files = File::fetchAll<File>( this, req );
    auto t = getConn()->newTransaction();
    for ( const auto& f : files )
    {
        // We must not call mrl() from here. We might not have all devices yet,
        // and calling mrl would crash for files stored on removable devices.
        auto newMrl = utils::url::encode( utils::url::decode( f->rawMrl() ) );
        LOG_INFO( "Converting ", f->rawMrl(), " to ", newMrl );
        f->setMrl( newMrl );
    }
    t->commit();
}

void MediaLibrary::migrateModel10to11()
{
    const std::string req = "SELECT * FROM " + parser::Task::Table::Name +
            " WHERE mrl LIKE '%#%%' ESCAPE '#'";
    const std::string folderReq = "SELECT * FROM " + Folder::Table::Name +
            " WHERE path LIKE '%#%%' ESCAPE '#'";
    auto tasks = parser::Task::fetchAll<parser::Task>( this, req );
    auto folders = Folder::fetchAll<Folder>( this, folderReq );
    auto t = getConn()->newTransaction();
    for ( const auto& t : tasks )
    {
        auto newMrl = utils::url::encode( utils::url::decode( t->item().mrl() ) );
        LOG_INFO( "Converting task mrl: ", t->item().mrl(), " to ", newMrl );
        t->setMrl( std::move( newMrl ) );
    }
    for ( const auto &f : folders )
    {
        // We must not call mrl() from here. We might not have all devices yet,
        // and calling mrl would crash for files stored on removable devices.
        auto newMrl = utils::url::encode( utils::url::decode( f->rawMrl() ) );
        f->setMrl( std::move( newMrl ) );
    }
    t->commit();
}

/*
 * - Some is_present related triggers were fixed in model 6 to 7 migration, but
 *   they were not recreated if already existing. The has_file_present trigger
 *   was recreated as part of model 7 to 8 migration, but we need to ensure
 *   has_album_present (Artist) & is_album_present (Album) triggers are
 *   recreated to behave as expected
 * - Due to a typo, is_track_present was named is_track_presentAFTER, and was
 *   executed BEFORE the update took place, thus using the wrong is_present value.
 *   The trigger will be recreated as part of this migration, and the values
 *   will be enforced, causing the entire update chain to be triggered, and
 *   restoring correct is_present values for all AlbumTrack/Album/Artist entries
 */
void MediaLibrary::migrateModel12to13()
{
    auto t = getConn()->newTransaction();
    const std::string reqs[] = {
        "DROP TRIGGER IF EXISTS is_track_presentAFTER",
        "DROP TRIGGER has_album_present",
        "DROP TRIGGER is_album_present",
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeDelete( getConn(), req );

    AlbumTrack::createTriggers( getConn() );
    Album::createTriggers( getConn() );
    Artist::createTriggers( getConn(), 13 );
    // Leave the weak context as we now need to update is_present fields, which
    // are propagated through recursive triggers
    const std::string migrateData = "UPDATE " + AlbumTrack::Table::Name +
            " SET is_present = (SELECT is_present FROM " + Media::Table::Name +
            " WHERE id_media = media_id)";
    sqlite::Tools::executeUpdate( getConn(), migrateData );
    t->commit();
}

/*
 * - Remove the Media.thumbnail
 * - Add Media.thumbnail_id
 * - Add Media.thumbnail_generated
 */
void MediaLibrary::migrateModel13to14( uint32_t originalPreviousVersion )
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();
    using ThumbnailType = typename std::underlying_type<Thumbnail::Origin>::type;
    std::string reqs[] = {
#               include "database/migrations/migration13-14.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
    // Task table was introduced in model 8, so if the user is migrating from a
    // version earlier than this one, the Task table will be properly created and
    // doesn't need a migration
    if ( originalPreviousVersion >= 8 )
    {
        const std::string migrateTaskReqs[] = {
            "CREATE TEMPORARY TABLE " + parser::Task::Table::Name + "_backup"
            "("
                "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
                "step INTEGER NOT NULL DEFAULT 0,"
                "retry_count INTEGER NOT NULL DEFAULT 0,"
                "mrl TEXT,"
                "file_id UNSIGNED INTEGER,"
                "parent_folder_id UNSIGNED INTEGER,"
                "parent_playlist_id INTEGER,"
                "parent_playlist_index UNSIGNED INTEGER"
            ")",

            "INSERT INTO " + parser::Task::Table::Name + "_backup SELECT * FROM " + parser::Task::Table::Name,

            "DROP TABLE " + parser::Task::Table::Name,

            #include "database/tables/Task_v14.sql"

            "INSERT INTO " + parser::Task::Table::Name + " SELECT "
            "id_task, step, retry_count, mrl, file_id, parent_folder_id, parent_playlist_id,"
            "parent_playlist_index, 0 FROM " + parser::Task::Table::Name + "_backup",

            "DROP TABLE " + parser::Task::Table::Name + "_backup",
        };
        for ( const auto& req : migrateTaskReqs )
            sqlite::Tools::executeRequest( dbConn, req );
    }
    // Migrate path to thumbnails out of the sql file, as we need to bind the
    // the mrl
    const std::string updateThumbnailReq = "UPDATE " + Thumbnail::Table::Name +
            " SET mrl = replace(mrl, ?, '')";
    sqlite::Tools::executeUpdate( dbConn, updateThumbnailReq, m_thumbnailPath );
    // Re-create tables that we just removed
    // We will run a re-scan, so we don't care about keeping their content
    Album::createTable( dbConn );
    Artist::createTable( dbConn );
    Movie::createTable( dbConn );
    Show::createTable( dbConn );
    VideoTrack::createTable( dbConn );
    // Re-create triggers removed in the process
    Media::createTriggers( dbConn, 14 );
    AlbumTrack::createTriggers( dbConn );
    Album::createTriggers( dbConn );
    Artist::createTriggers( dbConn, 14 );
    Show::createTriggers( dbConn );
    Playlist::createTriggers( dbConn );
    Folder::createTriggers( dbConn, 14 );
    const std::string req = "SELECT * FROM " + Media::Table::Name +
            " WHERE filename LIKE '%#%%' ESCAPE '#'";
    auto media = Media::fetchAll<Media>( this, req );
    for ( const auto& m : media )
    {
        // We must not call mrl() from here. We might not have all devices yet,
        // and calling mrl would crash for files stored on removable devices.
        auto newFileName = utils::url::decode( m->fileName() );
        LOG_INFO( "Converting ", m->fileName(), " to ", newFileName );
        m->setFileName( std::move( newFileName ) );
    }
    auto folders = Folder::fetchAll<Folder>( this );
    for ( const auto& f : folders )
    {
        f->setName( utils::file::directoryName( f->rawMrl() ) );
    }

    t->commit();
}

void MediaLibrary::reload()
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->reload();
}

void MediaLibrary::reload( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->reload( entryPoint );
}

bool MediaLibrary::forceParserRetry()
{
    try
    {
        parser::Task::resetRetryCount( this );
        return true;
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to force parser retry: ", ex.what() );
        return false;
    }
}

void MediaLibrary::pauseBackgroundOperations()
{
    if ( m_parser != nullptr )
        m_parser->pause();
#ifdef HAVE_LIBVLC
    if ( m_thumbnailer != nullptr )
        m_thumbnailer->pause();
#endif
}

void MediaLibrary::resumeBackgroundOperations()
{
    if ( m_parser != nullptr )
        m_parser->resume();
#ifdef HAVE_LIBVLC
    if ( m_thumbnailer != nullptr )
        m_thumbnailer->resume();
#endif
}

void MediaLibrary::onDiscovererIdleChanged( bool idle )
{
    bool expected = !idle;
    if ( m_discovererIdle.compare_exchange_strong( expected, idle ) == true )
    {
        // If any idle state changed to false, then we need to trigger the callback.
        // If switching to idle == true, then both background workers need to be idle before signaling.
        LOG_INFO( idle ? "Discoverer thread went idle" : "Discover thread was resumed" );
        if ( idle == false || m_parserIdle == true )
        {
            if ( idle == true && m_modificationNotifier != nullptr )
            {
                // Now that all discovery / parsing operations are completed,
                // force the modification notifier to signal changes before
                // signaling that we're done.
                // This allows one to flush all modifications when the medialib
                // goes back to idle
                m_modificationNotifier->flush();
            }
            LOG_INFO( "Setting background idle state to ",
                      idle ? "true" : "false" );
            m_callback->onBackgroundTasksIdleChanged( idle );
        }
    }
}

void MediaLibrary::onParserIdleChanged( bool idle )
{
    bool expected = !idle;
    if ( m_parserIdle.compare_exchange_strong( expected, idle ) == true )
    {
        LOG_INFO( idle ? "All parser services went idle" : "Parse services were resumed" );
        if ( idle == false || m_discovererIdle == true )
        {
            if ( idle == true && m_modificationNotifier != nullptr )
            {
                // See comments above
                m_modificationNotifier->flush();
            }
            LOG_INFO( "Setting background idle state to ",
                      idle ? "true" : "false" );
            m_callback->onBackgroundTasksIdleChanged( idle );
        }
    }
}

sqlite::Connection* MediaLibrary::getConn() const
{
    return m_dbConnection.get();
}

IMediaLibraryCb* MediaLibrary::getCb() const
{
    return m_callback;
}

std::shared_ptr<ModificationNotifier> MediaLibrary::getNotifier() const
{
    return m_modificationNotifier;
}

IDeviceListerCb* MediaLibrary::setDeviceLister( DeviceListerPtr lister )
{
    assert( m_initialized == false );
    m_deviceLister = lister;
    return static_cast<IDeviceListerCb*>( &m_deviceListerCbImpl );
}

std::shared_ptr<fs::IFileSystemFactory> MediaLibrary::fsFactoryForMrl( const std::string& mrl ) const
{
    for ( const auto& f : m_fsFactories )
    {
        if ( f->isMrlSupported( mrl ) )
            return f;
    }
    return nullptr;
}

void MediaLibrary::discover( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->discover( entryPoint );
}

void MediaLibrary::addNetworkFileSystemFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory )
{
    m_externalNetworkFsFactories.emplace_back( std::move( fsFactory ) );
}

bool MediaLibrary::setDiscoverNetworkEnabled( bool enabled )
{
    if ( enabled )
    {
        auto it = std::find_if( begin( m_fsFactories ),
                                end( m_fsFactories ),
                                []( const std::shared_ptr<fs::IFileSystemFactory> fs ) {
                                    return fs->isNetworkFileSystem();
                                });
        if ( it != end( m_fsFactories ) )
            return true;
        auto previousSize = m_fsFactories.size();
        for ( auto fsFactory : m_externalNetworkFsFactories )
        {
            if ( fsFactory->start( &m_fsFactoryCb ) == true )
                m_fsFactories.push_back( std::move( fsFactory ) );
        }
        return m_fsFactories.size() != previousSize;
    }

    auto it = std::remove_if( begin( m_fsFactories ), end( m_fsFactories ),
                              []( const std::shared_ptr<fs::IFileSystemFactory>& fs ) {
        return fs->isNetworkFileSystem();
    });
    std::for_each( it, end( m_fsFactories ), [this]( const std::shared_ptr<fs::IFileSystemFactory>& fsFactory )
    {
        auto t = m_dbConnection->newTransaction();
        auto devices = Device::fetchByScheme( this, fsFactory->scheme() );
        for ( const auto& d : devices )
            d->setPresent( false );
        t->commit();
        fsFactory->stop();
    });
    m_fsFactories.erase( it, end( m_fsFactories ) );
    return true;
}

Query<IFolder> MediaLibrary::entryPoints() const
{
    static const std::string req = "FROM " + Folder::Table::Name + " WHERE parent_id IS NULL"
            " AND is_banned = 0";
    return make_query<Folder, IFolder>( this, "*", req, "" );
}

bool MediaLibrary::isIndexed( const std::string& mrl ) const
{
    auto folderMrl = utils::file::directory( mrl );
    auto folder = Folder::fromMrl( this, folderMrl );
    return folder != nullptr;
}

Query<IFolder> MediaLibrary::folders( const QueryParameters* params ) const
{
    return Folder::withMedia( this, params );
}

Query<IFolder> MediaLibrary::searchFolders( const std::string& pattern,
                                            const QueryParameters* params ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Folder::searchWithMedia( this, pattern, params );
}

FolderPtr MediaLibrary::folder( int64_t id ) const
{
    return Folder::fetch( this, id );
}

FolderPtr MediaLibrary::folder( const std::string& mrl ) const
{
    return Folder::fromMrl( this, mrl, Folder::BannedType::Any );
}

void MediaLibrary::removeEntryPoint( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->remove( entryPoint );
}

void MediaLibrary::banFolder( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->ban( entryPoint );
}

void MediaLibrary::unbanFolder( const std::string& entryPoint )
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->unban( entryPoint );
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

void MediaLibrary::refreshDevices( fs::IFileSystemFactory& fsFactory )
{
    // Don't refuse to process devices when none seem to be present, it might be a valid case
    // if the user only discovered removable storages, and we would still need to mark those
    // as "not present"
    fsFactory.refreshDevices();
    auto devices = Device::fetchByScheme( this, fsFactory.scheme() );
    for ( auto& d : devices )
    {
        auto deviceFs = fsFactory.createDevice( d->uuid() );
        auto fsDevicePresent = deviceFs != nullptr && deviceFs->isPresent();
        if ( d->isPresent() != fsDevicePresent )
        {
            LOG_INFO( "Device ", d->uuid(), " changed presence state: ",
                      d->isPresent(), " -> ", fsDevicePresent );
            d->setPresent( fsDevicePresent );
        }
        else
            LOG_INFO( "Device ", d->uuid(), " unchanged" );

        if ( d->isPresent() == true )
            d->updateLastSeen();
    }
}

void MediaLibrary::forceRescan()
{
    if ( m_parser != nullptr )
    {
        m_parser->pause();
        m_parser->flush();
    }
    {
        auto t = getConn()->newTransaction();
        // Let the triggers clear out the Fts tables
        AlbumTrack::deleteAll( this );
        Genre::deleteAll( this );
        Album::deleteAll( this );
        Artist::deleteAll( this );
        Movie::deleteAll( this );
        ShowEpisode::deleteAll( this );
        Show::deleteAll( this );
        VideoTrack::deleteAll( this );
        AudioTrack::deleteAll( this );
        Playlist::clearExternalPlaylistContent( this );
        parser::Task::resetParsing( this );
        Artist::createDefaultArtists( getConn() );
        t->commit();
    }
    if ( m_parser != nullptr )
    {
        m_parser->restart();
        m_parser->restore();
        m_parser->resume();
    }
}

bool MediaLibrary::requestThumbnail( MediaPtr media )
{
    if ( m_thumbnailer == nullptr )
        return false;
    m_thumbnailer->requestThumbnail( media );
    return true;
}

void MediaLibrary::addParserService( std::shared_ptr<parser::IParserService> service )
{
    // For now we only support 1 external service of type MetadataExtraction
    if ( service->targetedStep() != parser::Step::MetadataExtraction )
        return;
    if ( m_services.size() != 0 )
        return;
    m_services.emplace_back( std::move( service ) );
}

void MediaLibrary::addThumbnailer( std::shared_ptr<IThumbnailer> thumbnailer )
{
    if ( m_thumbnailers.size() != 0 )
    {
        // We only support a single thumbnailer for videos for now.
        LOG_WARN( "Discarding thumbnailer since one has already been provided" );
        return;
    }
    m_thumbnailers.push_back( std::move( thumbnailer ) );
}

bool MediaLibrary::DeviceListerCb::onDeviceMounted( const std::string& uuid, const std::string& mountpoint )
{
    auto currentDevice = Device::fromUuid( m_ml, uuid );
    LOG_INFO( "Device ", uuid, " was plugged and mounted on ", mountpoint );
    for ( const auto& fsFactory : m_ml->m_fsFactories )
    {
        if ( fsFactory->isMrlSupported( mountpoint ) )
        {
            auto deviceFs = fsFactory->createDevice( uuid );
            if ( deviceFs != nullptr )
            {
                auto previousPresence = deviceFs->isPresent();
                deviceFs->addMountpoint( mountpoint );
                if ( previousPresence == false )
                {
                    LOG_INFO( "Device ", uuid, " changed presence state: 0 -> 1" );
                    if ( currentDevice != nullptr )
                        currentDevice->setPresent( true );
                }
            }
            break;
        }
    }
    return currentDevice == nullptr;
}

void MediaLibrary::DeviceListerCb::onDeviceUnmounted( const std::string& uuid,
                                                      const std::string& mountpoint )
{
    auto device = Device::fromUuid( m_ml, uuid );
    assert( device->isRemovable() == true );
    if ( device == nullptr )
    {
        LOG_WARN( "Unknown device ", uuid, " was unplugged. Ignoring." );
        return;
    }
    LOG_INFO( "Device ", uuid, " was unplugged" );
    for ( const auto& fsFactory : m_ml->m_fsFactories )
    {
        if ( fsFactory->scheme() == device->scheme() )
        {
            auto deviceFs = fsFactory->createDevice( uuid );
            if ( deviceFs != nullptr )
            {
                assert( deviceFs->isPresent() == true );
                deviceFs->removeMountpoint( mountpoint );
                if ( deviceFs->isPresent() == false )
                {
                    LOG_INFO( "Device ", uuid, " changed presence state: 1 -> 0" );
                    device->setPresent( false );
                }
            }
        }
    }
}

bool MediaLibrary::DeviceListerCb::isDeviceKnown( const std::string& uuid ) const
{
    return Device::fromUuid( m_ml, uuid ) != nullptr;
}

MediaLibrary::DeviceListerCb::DeviceListerCb( MediaLibrary* ml )
    : m_ml( ml )
{
}

MediaLibrary::FsFactoryCb::FsFactoryCb(MediaLibrary* ml)
    : m_ml( ml )
{
}

void MediaLibrary::FsFactoryCb::onDevicePlugged( const std::string& uuid )
{
    onDeviceChanged( uuid, true );
}

void MediaLibrary::FsFactoryCb::onDeviceUnplugged( const std::string& uuid )
{
    onDeviceChanged( uuid, false );
}

void MediaLibrary::FsFactoryCb::onDeviceChanged( const std::string& uuid, bool isPresent )
{
    auto device = Device::fromUuid( m_ml, uuid );
    if ( device == nullptr )
        return;
    assert( device->isRemovable() == true );
    if ( device->isPresent() == isPresent )
        return;
    device->setPresent( isPresent );
}

}
