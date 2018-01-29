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
#include "History.h"
#include "Media.h"
#include "MediaLibrary.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "parser/Parser.h"
#include "Playlist.h"
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
#include "metadata_services/MetadataParser.h"

// FileSystem
#include "factory/DeviceListerFactory.h"
#include "factory/FileSystemFactory.h"
#include "factory/NetworkFileSystemFactory.h"
#include "filesystem/IDevice.h"

namespace medialibrary
{

const char* const MediaLibrary::supportedExtensions[] = {
    "3gp", "a52", "aac", "ac3", "aiff", "alac", "amr", "amv",
    "aob", "ape", "asf", "asx", "avi", "b4s", "conf", /*"cue",*/
    "divx", "dts", "dv", "flac", "flv", "gxf", "ifo", "iso",
    "it", "itml",  "m1v", "m2t", "m2ts", "m2v", "m3u", "m3u8",
    "m4a", "m4b", "m4p", "m4v", "mid", "mka", "mkv", "mlp",
    "mod", "mov", "mp1", "mp2", "mp3", "mp4", "mpc", "mpeg",
    "mpeg1", "mpeg2", "mpeg4", "mpg", "mts", "mxf", "nsv",
    "nuv", "oga", "ogg", "ogm", "ogv", "ogx", "oma", "opus",
    "pls", "ps", "qtl", "ram", "rec", "rm", "rmi", "rmvb",
    "s3m", "sdp", "spx", "tod", "trp", "ts", "tta", "vlc",
    "vob", "voc", "vqf", "vro", "w64", "wav", "wax", "webm",
    "wma", "wmv", "wmx", "wpl", "wv", "wvx", "xa", "xm", "xspf"
};

const size_t MediaLibrary::NbSupportedExtensions = sizeof(supportedExtensions) / sizeof(supportedExtensions[0]);

MediaLibrary::MediaLibrary()
    : m_callback( nullptr )
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
    clearCache();
}

void MediaLibrary::clearCache()
{
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
    File::clear();
    Playlist::clear();
    History::clear();
    Genre::clear();
}

void MediaLibrary::createAllTables()
{
    // We need to create the tables in order of triggers creation
    // Device is the "root of all evil". When a device is modified,
    // we will trigger an update on folder, which will trigger
    // an update on files, and so on.

    Device::createTable( m_dbConnection.get() );
    Folder::createTable( m_dbConnection.get() );
    Media::createTable( m_dbConnection.get() );
    File::createTable( m_dbConnection.get() );
    Label::createTable( m_dbConnection.get() );
    Playlist::createTable( m_dbConnection.get() );
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
    History::createTable( m_dbConnection.get() );
    Settings::createTable( m_dbConnection.get() );
    parser::Task::createTable( m_dbConnection.get() );
}

void MediaLibrary::createAllTriggers()
{
    auto dbModelVersion = m_settings.dbModelVersion();
    Album::createTriggers( m_dbConnection.get() );
    Artist::createTriggers( m_dbConnection.get(), dbModelVersion );
    Media::createTriggers( m_dbConnection.get() );
    Genre::createTriggers( m_dbConnection.get() );
    Playlist::createTriggers( m_dbConnection.get() );
}

template <typename T>
static void propagateDeletionToCache( sqlite::Connection::HookReason reason, int64_t rowId )
{
    if ( reason != sqlite::Connection::HookReason::Delete )
        return;
    T::removeFromCache( rowId );
}

void MediaLibrary::registerEntityHooks()
{
    if ( m_modificationNotifier == nullptr )
        return;

    m_dbConnection->registerUpdateHook( policy::MediaTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        Media::removeFromCache( rowId );
        m_modificationNotifier->notifyMediaRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::ArtistTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        Artist::removeFromCache( rowId );
        m_modificationNotifier->notifyArtistRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::AlbumTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        Album::removeFromCache( rowId );
        m_modificationNotifier->notifyAlbumRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::AlbumTrackTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        AlbumTrack::removeFromCache( rowId );
        m_modificationNotifier->notifyAlbumTrackRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::PlaylistTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason != sqlite::Connection::HookReason::Delete )
            return;
        Playlist::removeFromCache( rowId );
        m_modificationNotifier->notifyPlaylistRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::DeviceTable::Name, &propagateDeletionToCache<Device> );
    m_dbConnection->registerUpdateHook( policy::FileTable::Name, &propagateDeletionToCache<File> );
    m_dbConnection->registerUpdateHook( policy::FolderTable::Name, &propagateDeletionToCache<Folder> );
    m_dbConnection->registerUpdateHook( policy::GenreTable::Name, &propagateDeletionToCache<Genre> );
    m_dbConnection->registerUpdateHook( policy::LabelTable::Name, &propagateDeletionToCache<Label> );
    m_dbConnection->registerUpdateHook( policy::MovieTable::Name, &propagateDeletionToCache<Movie> );
    m_dbConnection->registerUpdateHook( policy::ShowTable::Name, &propagateDeletionToCache<Show> );
    m_dbConnection->registerUpdateHook( policy::ShowEpisodeTable::Name, &propagateDeletionToCache<ShowEpisode> );
    m_dbConnection->registerUpdateHook( policy::AudioTrackTable::Name, &propagateDeletionToCache<AudioTrack> );
    m_dbConnection->registerUpdateHook( policy::VideoTrackTable::Name, &propagateDeletionToCache<VideoTrack> );
}

bool MediaLibrary::validateSearchPattern( const std::string& pattern )
{
    return pattern.size() >= 3;
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
#ifdef _WIN32
    if ( mkdir( thumbnailPath.c_str() ) != 0 )
#else
    if ( mkdir( thumbnailPath.c_str(), S_IRWXU ) != 0 )
#endif
    {
        if ( errno != EEXIST )
        {
            LOG_ERROR( "Failed to create thumbnail directory: ", strerror( errno ) );
            return InitializeResult::Failed;
        }
    }
    m_thumbnailPath = thumbnailPath;
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
    startDiscoverer();
    startParser();
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
                  device->isRemovable() ? "NOT" : "", "removable)");
        return nullptr;
    }
    return file->media();
}

MediaPtr MediaLibrary::addMedia( const std::string& mrl )
{
    try
    {
        return sqlite::Tools::withRetries( 3, [this, &mrl]() -> MediaPtr {
            auto t = m_dbConnection->newTransaction();
            auto media = Media::create( this, IMedia::Type::External, utils::file::fileName( mrl ) );
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

std::vector<MediaPtr> MediaLibrary::audioFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::Audio, sort, desc );
}

std::vector<MediaPtr> MediaLibrary::videoFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::Video, sort, desc );
}

bool MediaLibrary::isExtensionSupported( const char* ext )
{
    return std::binary_search( std::begin( supportedExtensions ),
        std::end( supportedExtensions ), ext, [](const char* l, const char* r) {
            return strcasecmp( l, r ) < 0;
        });
}

void MediaLibrary::addDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                      std::shared_ptr<Folder> parentFolder,
                                      std::shared_ptr<fs::IDirectory> parentFolderFs,
                                      std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist )
{
    auto task = parser::Task::create( this, std::move( fileFs ), std::move( parentFolder ),
                                      std::move( parentFolderFs ), std::move( parentPlaylist ) );
    if ( task != nullptr && m_parser != nullptr )
        m_parser->parse( task );
}

bool MediaLibrary::deleteFolder( const Folder& folder )
{
    LOG_INFO( "deleting folder ", folder.mrl() );
    if ( Folder::destroy( this, folder.id() ) == false )
        return false;
    Media::clear();
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

std::shared_ptr<Album> MediaLibrary::createAlbum( const std::string& title, const std::string& artworkMrl )
{
    return Album::create( this, title, artworkMrl );
}

std::vector<AlbumPtr> MediaLibrary::albums( SortingCriteria sort, bool desc ) const
{
    return Album::listAll( this, sort, desc );
}

std::vector<GenrePtr> MediaLibrary::genres( SortingCriteria sort, bool desc ) const
{
    return Genre::listAll( this, sort, desc );
}

GenrePtr MediaLibrary::genre( int64_t id ) const
{
    return Genre::fetch( this, id );
}

ShowPtr MediaLibrary::show( const std::string& name ) const
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return Show::fetch( this, req, name );
}

std::shared_ptr<Show> MediaLibrary::createShow( const std::string& name )
{
    return Show::create( this, name );
}

MoviePtr MediaLibrary::movie( const std::string& title ) const
{
    static const std::string req = "SELECT * FROM " + policy::MovieTable::Name
            + " WHERE title = ?";
    return Movie::fetch( this, req, title );
}

std::shared_ptr<Movie> MediaLibrary::createMovie( Media& media, const std::string& title )
{
    auto movie = Movie::create( this, media.id(), title );
    media.setMovie( movie );
    media.save();
    return movie;
}

ArtistPtr MediaLibrary::artist( int64_t id ) const
{
    return Artist::fetch( this, id );
}

ArtistPtr MediaLibrary::artist( const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name
            + " WHERE name = ? AND is_present != 0";
    return Artist::fetch( this, req, name );
}

std::shared_ptr<Artist> MediaLibrary::createArtist( const std::string& name )
{
    try
    {
        return Artist::create( this, name );
    }
    catch( sqlite::errors::ConstraintViolation &ex )
    {
        LOG_WARN( "ContraintViolation while creating an artist (", ex.what(), ") attempting to fetch it instead" );
        return std::static_pointer_cast<Artist>( artist( name ) );
    }
}

std::vector<ArtistPtr> MediaLibrary::artists( bool includeAll,
                                              SortingCriteria sort,
                                              bool desc ) const
{
    return Artist::listAll( this, includeAll, sort, desc );
}

PlaylistPtr MediaLibrary::createPlaylist( const std::string& name )
{
    try
    {
        return Playlist::create( this, name );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to create a playlist: ", ex.what() );
        return nullptr;
    }
}

std::vector<PlaylistPtr> MediaLibrary::playlists( SortingCriteria sort, bool desc )
{
    return Playlist::listAll( this, sort, desc );
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

bool MediaLibrary::addToStreamHistory( MediaPtr media )
{
    try
    {
        return History::insert( getConn(), media->id() );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to add stream to history: ", ex.what() );
        return false;
    }
}

std::vector<HistoryPtr> MediaLibrary::lastStreamsPlayed() const
{
    return History::fetch( this );
}

std::vector<MediaPtr> MediaLibrary::lastMediaPlayed() const
{
    return Media::fetchHistory( this );
}

bool MediaLibrary::clearHistory()
{
    try
    {
        return sqlite::Tools::withRetries( 3, [this]() {
            auto t = getConn()->newTransaction();
            Media::clearHistory( this );
            History::clearStreams( this );
            t->commit();
            return true;
        });
    }
    catch ( sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to clear history: ", ex.what() );
        return false;
    }
}

MediaSearchAggregate MediaLibrary::searchMedia( const std::string& title ) const
{
    if ( validateSearchPattern( title ) == false )
        return {};
    auto tmp = Media::search( this, title );
    MediaSearchAggregate res;
    for ( auto& m : tmp )
    {
        switch ( m->subType() )
        {
        case IMedia::SubType::AlbumTrack:
            res.tracks.emplace_back( std::move( m ) );
            break;
        case IMedia::SubType::Movie:
            res.movies.emplace_back( std::move( m ) );
            break;
        case IMedia::SubType::ShowEpisode:
            res.episodes.emplace_back( std::move( m ) );
            break;
        default:
            res.others.emplace_back( std::move( m ) );
            break;
        }
    }
    return res;
}

std::vector<PlaylistPtr> MediaLibrary::searchPlaylists( const std::string& name ) const
{
    if ( validateSearchPattern( name ) == false )
        return {};
    return Playlist::search( this, name );
}

std::vector<AlbumPtr> MediaLibrary::searchAlbums( const std::string& pattern ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Album::search( this, pattern );
}

std::vector<GenrePtr> MediaLibrary::searchGenre( const std::string& genre ) const
{
    if ( validateSearchPattern( genre ) == false )
        return {};
    return Genre::search( this, genre );
}

std::vector<ArtistPtr> MediaLibrary::searchArtists(const std::string& name ) const
{
    if ( validateSearchPattern( name ) == false )
        return {};
    return Artist::search( this, name );
}

SearchAggregate MediaLibrary::search( const std::string& pattern ) const
{
    SearchAggregate res;
    res.albums = searchAlbums( pattern );
    res.artists = searchArtists( pattern );
    res.genres = searchGenre( pattern );
    res.media = searchMedia( pattern );
    res.playlists = searchPlaylists( pattern );
    return res;
}

void MediaLibrary::startParser()
{
    m_parser.reset( new Parser( this ) );

    auto vlcService = std::unique_ptr<VLCMetadataService>( new VLCMetadataService );
    auto metadataService = std::unique_ptr<MetadataParser>( new MetadataParser );
    auto thumbnailerService = std::unique_ptr<VLCThumbnailer>( new VLCThumbnailer );
    m_parser->addService( std::move( vlcService ) );
    m_parser->addService( std::move( metadataService ) );
    m_parser->addService( std::move( thumbnailerService ) );
    m_parser->start();
}

void MediaLibrary::startDiscoverer()
{
    m_discovererWorker.reset( new DiscovererWorker( this ) );
    for ( const auto& fsFactory : m_fsFactories )
    {
        auto probePtr = std::unique_ptr<prober::CrawlerProbe>( new prober::CrawlerProbe{} );
        m_discovererWorker->addDiscoverer( std::unique_ptr<IDiscoverer>( new FsDiscoverer( fsFactory, this, m_callback,
                                                                                           std::move ( probePtr ) ) ) );
    }
}

void MediaLibrary::startDeletionNotifier()
{
    m_modificationNotifier.reset( new ModificationNotifier( this ) );
    m_modificationNotifier->start();
}

void MediaLibrary::addLocalFsFactory()
{
    m_fsFactories.insert( begin( m_fsFactories ), std::make_shared<factory::FileSystemFactory>( m_deviceLister ) );
}

InitializeResult MediaLibrary::updateDatabaseModel( unsigned int previousVersion,
                                        const std::string& dbPath )
{
    LOG_INFO( "Updating database model from ", previousVersion, " to ", Settings::DbModelVersion );
    // Up until model 3, it's safer (and potentially more efficient with index changes) to drop the DB
    // It's also way simpler to implement
    // In case of downgrade, just recreate the database
    for ( auto i = 0u; i < 3; ++i )
    {
        try
        {
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
                if ( migrateModel3to5() == false )
                    throw std::logic_error( "Failed to migrate from 3 to 5" );
                previousVersion = 5;
            }
            if ( previousVersion == 5 )
            {
                if ( migrateModel5to6() == false )
                    throw std::logic_error( "Failed to migrate from 5 to 6" );
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
            // To be continued in the future!

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

bool MediaLibrary::migrateModel3to5()
{
    /*
     * Disable Foreign Keys & recursive triggers to avoid cascading deletion
     * while remodeling the database into the transaction.
     */
    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    auto t = getConn()->newTransaction();
    using namespace policy;
    // As SQLite do not allow us to remove or add some constraints,
    // we use the method described here https://www.sqlite.org/faq.html#q11
    std::string reqs[] = {
#               include "database/migrations/migration3-5.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );
    // Re-create triggers removed in the process
    Media::createTriggers( getConn() );
    Playlist::createTriggers( getConn() );
    t->commit();
    return true;
}

bool MediaLibrary::migrateModel5to6()
{
    std::string req = "DELETE FROM " + policy::MediaTable::Name + " WHERE type = ?";
    sqlite::Tools::executeRequest( getConn(), req, Media::Type::Unknown );

    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    using namespace policy;
    req = "UPDATE " + MediaTable::Name + " SET is_present = 1 WHERE is_present != 0";
    sqlite::Tools::executeRequest( getConn(), req );

    return true;
}

void MediaLibrary::migrateModel7to8()
{
    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    auto t = getConn()->newTransaction();
    using namespace policy;
    std::string reqs[] = {
#               include "database/migrations/migration7-8.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );
    // Re-create triggers removed in the process
    Artist::createTriggers( getConn(), 8u );
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
}

void MediaLibrary::resumeBackgroundOperations()
{
    if ( m_parser != nullptr )
        m_parser->resume();
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
    return static_cast<IDeviceListerCb*>( this );
}

std::shared_ptr<factory::IFileSystem> MediaLibrary::fsFactoryForMrl( const std::string& mrl ) const
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

void MediaLibrary::setDiscoverNetworkEnabled( bool enabled )
{
    if ( enabled )
    {
        auto it = std::find_if( begin( m_fsFactories ), end( m_fsFactories ), []( const std::shared_ptr<factory::IFileSystem> fs ) {
            return fs->isNetworkFileSystem();
        });
        if ( it == end( m_fsFactories ) )
            m_fsFactories.push_back( std::make_shared<factory::NetworkFileSystemFactory>( "smb", "dsm-sd" ) );
    }
    else
    {
        m_fsFactories.erase( std::remove_if( begin( m_fsFactories ), end( m_fsFactories ), []( const std::shared_ptr<factory::IFileSystem> fs ) {
            return fs->isNetworkFileSystem();
        }), end( m_fsFactories ) );
    }
}

std::vector<FolderPtr> MediaLibrary::entryPoints() const
{
    static const std::string req = "SELECT * FROM " + policy::FolderTable::Name + " WHERE parent_id IS NULL"
            " AND is_blacklisted = 0";
    return Folder::fetchAll<IFolder>( this, req );
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

void MediaLibrary::refreshDevices( factory::IFileSystem& fsFactory )
{
    // Don't refuse to process devices when none seem to be present, it might be a valid case
    // if the user only discovered removable storages, and we would still need to mark those
    // as "not present"
    fsFactory.refreshDevices();
    auto devices = Device::fetchAll( this );
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
        Playlist::deleteAllExternal( this );
        parser::Task::resetParsing( this );
        clearCache();
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

bool MediaLibrary::onDevicePlugged( const std::string& uuid, const std::string& mountpoint )
{
    auto currentDevice = Device::fromUuid( this, uuid );
    LOG_INFO( "Device ", uuid, " was plugged and mounted on ", mountpoint );
    for ( const auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isMrlSupported( "file://" ) )
        {
            auto deviceFs = fsFactory->createDevice( uuid );
            if ( deviceFs != nullptr )
            {
                LOG_INFO( "Device ", uuid, " changed presence state: 0 -> 1" );
                assert( deviceFs->isPresent() == false );
                deviceFs->setPresent( true );
                if ( currentDevice != nullptr )
                    currentDevice->setPresent( true );
            }
            else
                refreshDevices( *fsFactory );
            break;
        }
    }
    return currentDevice == nullptr;
}

void MediaLibrary::onDeviceUnplugged( const std::string& uuid )
{
    auto device = Device::fromUuid( this, uuid );
    assert( device->isRemovable() == true );
    if ( device == nullptr )
    {
        LOG_WARN( "Unknown device ", uuid, " was unplugged. Ignoring." );
        return;
    }
    LOG_INFO( "Device ", uuid, " was unplugged" );
    for ( const auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isMrlSupported( "file://" ) )
        {
            auto deviceFs = fsFactory->createDevice( uuid );
            if ( deviceFs != nullptr )
            {
                assert( deviceFs->isPresent() == true );
                LOG_INFO( "Device ", uuid, " changed presence state: 1 -> 0" );
                deviceFs->setPresent( false );
                device->setPresent( false );
            }
            else
                refreshDevices( *fsFactory );
        }
    }
}

bool MediaLibrary::isDeviceKnown( const std::string& uuid ) const
{
    return Device::fromUuid( this, uuid ) != nullptr;
}

}
