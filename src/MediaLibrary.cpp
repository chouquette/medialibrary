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
#include <sys/stat.h>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "discoverer/DiscovererWorker.h"
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
    "3gp", "a52", "aac", "ac3", "aiff", "amr", "amv", "aob", "ape",
    "asf", "avi", "divx", "dts", "dv", "flac", "flv", "gxf", "iso",
    "it", "m1v", "m2t", "m2ts", "m2v", "m4a", "m4b", "m4p", "m4v",
    "mid", "mka", "mkv", "mlp", "mod", "mov", "mp1", "mp2", "mp2",
    "mp3", "mp4", "mpc", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "oga", "ogg", "ogm", "ogv", "ogx",
    "oma", "opus", "ps", "rec", "rm", "rmi", "rmvb", "s3m", "spx",
    "tod", "trp", "ts", "tta", "vob", "voc", "vqf", "vro", "w64",
    "wav", "webm", "wma", "wmv", "wv", "xa", "xm"
};

const uint32_t MediaLibrary::DbModelVersion = 2;

MediaLibrary::MediaLibrary()
    : m_callback( nullptr )
    , m_verbosity( LogLevel::Error )
    , m_initialized( false )
{
    Log::setLogLevel( m_verbosity );
}

MediaLibrary::~MediaLibrary()
{
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    if ( m_discoverer != nullptr )
        m_discoverer->stop();
    if ( m_parser != nullptr )
        m_parser->stop();
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

bool MediaLibrary::createAllTables()
{
    // We need to create the tables in order of triggers creation
    // Device is the "root of all evil". When a device is modified,
    // we will trigger an update on folder, which will trigger
    // an update on files, and so on.

    auto t = m_dbConnection->newTransaction();
    auto res = Device::createTable( m_dbConnection.get() ) &&
        Folder::createTable( m_dbConnection.get() ) &&
        Media::createTable( m_dbConnection.get() ) &&
        File::createTable( m_dbConnection.get() ) &&
        Label::createTable( m_dbConnection.get() ) &&
        Playlist::createTable( m_dbConnection.get() ) &&
        Genre::createTable( m_dbConnection.get() ) &&
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
        Media::createTriggers( m_dbConnection.get() ) &&
        Genre::createTriggers( m_dbConnection.get() ) &&
        Playlist::createTriggers( m_dbConnection.get() ) &&
        History::createTable( m_dbConnection.get() ) &&
        Settings::createTable( m_dbConnection.get() );
    if ( res == false )
        return false;
    t->commit();
    return true;
}

template <typename T>
static void propagateDeletionToCache( SqliteConnection::HookReason reason, int64_t rowId )
{
    if ( reason != SqliteConnection::HookReason::Delete )
        return;
    T::removeFromCache( rowId );
}

void MediaLibrary::registerEntityHooks()
{
    if ( m_modificationNotifier == nullptr )
        return;

    m_dbConnection->registerUpdateHook( policy::MediaTable::Name,
                                        [this]( SqliteConnection::HookReason reason, int64_t rowId ) {
        if ( reason != SqliteConnection::HookReason::Delete )
            return;
        Media::removeFromCache( rowId );
        m_modificationNotifier->notifyMediaRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::ArtistTable::Name,
                                        [this]( SqliteConnection::HookReason reason, int64_t rowId ) {
        if ( reason != SqliteConnection::HookReason::Delete )
            return;
        Artist::removeFromCache( rowId );
        m_modificationNotifier->notifyArtistRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::AlbumTable::Name,
                                        [this]( SqliteConnection::HookReason reason, int64_t rowId ) {
        if ( reason != SqliteConnection::HookReason::Delete )
            return;
        Album::removeFromCache( rowId );
        m_modificationNotifier->notifyAlbumRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::AlbumTrackTable::Name,
                                        [this]( SqliteConnection::HookReason reason, int64_t rowId ) {
        if ( reason != SqliteConnection::HookReason::Delete )
            return;
        AlbumTrack::removeFromCache( rowId );
        m_modificationNotifier->notifyAlbumTrackRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( policy::PlaylistTable::Name,
                                        [this]( SqliteConnection::HookReason reason, int64_t rowId ) {
        if ( reason != SqliteConnection::HookReason::Delete )
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

bool MediaLibrary::initialize( const std::string& dbPath, const std::string& thumbnailPath, IMediaLibraryCb* mlCallback )
{
    LOG_INFO( "Initializing medialibrary..." );
    if ( m_initialized == true )
    {
        LOG_INFO( "...Already initialized" );
        return true;
    }
    if ( m_deviceLister == nullptr )
    {
        m_deviceLister = factory::createDeviceLister();
        if ( m_deviceLister == nullptr )
        {
            LOG_ERROR( "No available IDeviceLister was found." );
            return false;
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
            throw std::runtime_error( std::string( "Failed to create thumbnail directory: " ) +
                                      strerror( errno ) );
    }
    m_thumbnailPath = thumbnailPath;
    m_callback = mlCallback;
    m_dbConnection.reset( new SqliteConnection( dbPath ) );

    // Give a chance to test overloads to reject the creation of a notifier
    startDeletionNotifier();
    // Which allows us to register hooks, or not, depending on the presence of a notifier
    registerEntityHooks();

    if ( createAllTables() == false )
    {
        LOG_ERROR( "Failed to create database structure" );
        return false;
    }
    if ( m_settings.load( m_dbConnection.get() ) == false )
    {
        LOG_ERROR( "Failed to load settings" );
        return false;
    }
    if ( m_settings.dbModelVersion() != DbModelVersion )
    {
        if ( updateDatabaseModel( m_settings.dbModelVersion() ) == false )
        {
            LOG_ERROR( "Failed to update database model" );
            return false;
        }
    }
    for ( auto& fsFactory : m_fsFactories )
        refreshDevices( *fsFactory );
    startDiscoverer();
    startParser();
    m_initialized = true;
    LOG_INFO( "Successfuly initialized" );
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
        LOG_WARN( "Failed to fetch file for ", mrl, "(device ", device->uuid(), " was ",
                  device->isRemovable() ? "NOT" : "", "removable)");
        return nullptr;
    }
    return file->media();
}

MediaPtr MediaLibrary::addMedia( const std::string& mrl )
{
    auto t = m_dbConnection->newTransaction();
    auto media = Media::create( this, IMedia::Type::Unknown, utils::file::fileName( mrl ) );
    if ( media == nullptr )
        return nullptr;
    if ( media->addExternalMrl( mrl, IFile::Type::Main ) == nullptr )
        return nullptr;
    t->commit();
    return media;
}

std::vector<MediaPtr> MediaLibrary::audioFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::Audio, sort, desc );
}

std::vector<MediaPtr> MediaLibrary::videoFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::Video, sort, desc );
}

std::shared_ptr<Media> MediaLibrary::addFile( const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs )
{
    auto type = IMedia::Type::Unknown;

    if ( std::binary_search( std::begin( supportedExtensions ), std::end( supportedExtensions ),
                             fileFs.extension().c_str(),
                             [](const char* l, const char* r) { return strcasecmp( l, r ) < 0; }
                            ) == false )
    {
        LOG_INFO( "Rejecting file ", fileFs.mrl(), " due to its extension" );
        return nullptr;
    }

    LOG_INFO( "Adding ", fileFs.mrl() );
    auto mptr = Media::create( this, type, fileFs.name() );
    if ( mptr == nullptr )
    {
        LOG_ERROR( "Failed to add media ", fileFs.mrl(), " to the media library" );
        return nullptr;
    }
    // For now, assume all media are made of a single file
    auto file = mptr->addFile( fileFs, parentFolder, parentFolderFs, File::Type::Main );
    if ( file == nullptr )
    {
        LOG_ERROR( "Failed to add file ", fileFs.mrl(), " to media #", mptr->id() );
        Media::destroy( this, mptr->id() );
        return nullptr;
    }
    if ( m_parser != nullptr )
        m_parser->parse( mptr, file );
    return mptr;
}

bool MediaLibrary::deleteFolder( const Folder& folder )
{
    if ( Folder::destroy( this, folder.id() ) == false )
        return false;
    Media::clear();
    return true;
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    return Label::create( this, label );
}

bool MediaLibrary::deleteLabel( LabelPtr label )
{
    return Label::destroy( this, label->id() );
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
            + " WHERE name = ? AND is_present = 1";
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

std::vector<ArtistPtr> MediaLibrary::artists( SortingCriteria sort, bool desc ) const
{
    return Artist::listAll( this, sort, desc );
}

PlaylistPtr MediaLibrary::createPlaylist( const std::string& name )
{
    return Playlist::create( this, name );
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
    return Playlist::destroy( this, playlistId );
}

bool MediaLibrary::addToStreamHistory( MediaPtr media )
{
    return History::insert( getConn(), media->id() );
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
    auto t = getConn()->newTransaction();
    Media::clearHistory( this );
    if ( History::clearStreams( this ) == false )
        return false;
    t->commit();
    return true;
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
    m_discoverer.reset( new DiscovererWorker( this ) );
    for ( const auto& fsFactory : m_fsFactories )
        m_discoverer->addDiscoverer( std::unique_ptr<IDiscoverer>( new FsDiscoverer( fsFactory, this, m_callback ) ) );
    m_discoverer->reload();
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

bool MediaLibrary::updateDatabaseModel( unsigned int previousVersion )
{
    if ( previousVersion == 1 )
    {
        // Way too much differences, introduction of devices, and almost unused in the wild, just drop everything
        std::string req = "PRAGMA writable_schema = 1;"
                            "delete from sqlite_master;"
                            "PRAGMA writable_schema = 0;";
        if ( sqlite::Tools::executeRequest( getConn(), req ) == false )
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

void MediaLibrary::reload( const std::string& entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->reload( entryPoint );
}

void MediaLibrary::forceParserRetry()
{
    File::resetRetryCount( this );
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

DBConnection MediaLibrary::getConn() const
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

void MediaLibrary::discover( const std::string &entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->discover( entryPoint );
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
    if ( m_discoverer != nullptr )
        m_discoverer->remove( entryPoint );
}

void MediaLibrary::banFolder( const std::string& entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->ban( entryPoint );
}

void MediaLibrary::unbanFolder( const std::string& entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->unban( entryPoint );
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

bool MediaLibrary::onDeviceChanged( factory::IFileSystem& fsFactory, Device& device )
{
    auto deviceFs = fsFactory.createDevice( device.uuid() );
    auto fsDevicePresent = deviceFs != nullptr && deviceFs->isPresent();
    if ( device.isPresent() != fsDevicePresent )
    {
        LOG_INFO( "Device ", device.uuid(), " changed presence state: ",
                  device.isPresent(), " -> ", fsDevicePresent );
        device.setPresent( fsDevicePresent );
        return true;
    }
    LOG_INFO( "Device ", device.uuid(), " unchanged" );
    return false;
}

void MediaLibrary::refreshDevices( factory::IFileSystem& fsFactory )
{
    // Don't refuse to process devices when none seem to be present, it might be a valid case
    // if the user only discovered removable storages, and we would still need to mark those
    // as "not present"
    fsFactory.refreshDevices();
    auto devices = Device::fetchAll( this );
    for ( auto& d : devices )
        onDeviceChanged( fsFactory, *d );
}

bool MediaLibrary::onDevicePlugged( const std::string& uuid, const std::string& mountpoint )
{
    auto currentDevice = Device::fromUuid( this, uuid );
    LOG_INFO( "Device ", uuid, " was plugged and mounted on ", mountpoint );
    assert( currentDevice == nullptr || currentDevice->isPresent() == false );
    // If we don't know the device yet, simply postpone the device creation to the first
    // discovery/reload, and inform the caller that we didn't know this device yet.
    if ( currentDevice == nullptr )
        return true;
    for ( const auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isMrlSupported( "file://" ) )
        {
            auto res = onDeviceChanged( *fsFactory, *currentDevice );
            // Ensure the device actually changed.
            assert( res == true );
            break;
        }
    }
    return false;
}

void MediaLibrary::onDeviceUnplugged( const std::string& uuid )
{
    auto device = Device::fromUuid( this, uuid );
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
            auto res = onDeviceChanged( *fsFactory, *device );
            assert( res == true );
            break;
        }
    }
}

}
