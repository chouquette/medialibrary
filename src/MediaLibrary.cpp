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

const std::vector<std::string> MediaLibrary::supportedVideoExtensions {
    // Videos
    "avi", "3gp", "amv", "asf", "divx", "dv", "flv", "gxf",
    "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
    "mp2", "mp4", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "ogm", "ogv", "ogx", "ps",
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
    : m_callback( nullptr )
    , m_verbosity( LogLevel::Error )
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
        Playlist::createTriggers( m_dbConnection.get() ) &&
        History::createTable( m_dbConnection.get() ) &&
        Settings::createTable( m_dbConnection.get() );
    if ( res == false )
        return false;
    t->commit();
    return true;
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
}


bool MediaLibrary::validateSearchPattern( const std::string& pattern )
{
    return pattern.size() >= 3;
}

bool MediaLibrary::initialize( const std::string& dbPath, const std::string& thumbnailPath, IMediaLibraryCb* mlCallback )
{
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

MediaPtr MediaLibrary::media( int64_t mediaId ) const
{
    return Media::fetch( this, mediaId );
}

MediaPtr MediaLibrary::media( const std::string& mrl ) const
{
    auto fsFactory = fsFactoryForPath( mrl );
    if ( fsFactory == nullptr )
        return nullptr;
    auto device = fsFactory->createDeviceFromPath( mrl );
    if ( device == nullptr )
    {
        LOG_WARN( "Failed to create a device associated with mrl ", mrl );
        return nullptr;
    }
    std::shared_ptr<File> file;
    if ( device->isRemovable() == false )
        file = File::fromPath( this, mrl );
    else
    {
        auto folder = Folder::fromPath( this, utils::file::directory( mrl ) );
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

std::vector<MediaPtr> MediaLibrary::audioFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::AudioType, sort, desc );
}

std::vector<MediaPtr> MediaLibrary::videoFiles( SortingCriteria sort, bool desc ) const
{
    return Media::listAll( this, IMedia::Type::VideoType, sort, desc );
}

std::shared_ptr<Media> MediaLibrary::addFile( const fs::IFile& fileFs, Folder& parentFolder, fs::IDirectory& parentFolderFs )
{
    auto type = IMedia::Type::UnknownType;
    auto ext = fileFs.extension();
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

    LOG_INFO( "Adding ", fileFs.fullPath() );
    auto mptr = Media::create( this, type, fileFs );
    if ( mptr == nullptr )
    {
        LOG_ERROR( "Failed to add media ", fileFs.fullPath(), " to the media library" );
        return nullptr;
    }
    // For now, assume all media are made of a single file
    auto file = mptr->addFile( fileFs, parentFolder, parentFolderFs, File::Type::Entire );
    if ( file == nullptr )
    {
        LOG_ERROR( "Failed to add file ", fileFs.fullPath(), " to media #", mptr->id() );
        Media::destroy( this, mptr->id() );
        return nullptr;
    }
    m_modificationNotifier->notifyMediaCreation( mptr );
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

std::shared_ptr<Device> MediaLibrary::device( const std::string& uuid )
{
    return Device::fromUuid( this, uuid );
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

std::shared_ptr<Album> MediaLibrary::createAlbum(const std::string& title )
{
    return Album::create( this, title );
}

std::vector<AlbumPtr> MediaLibrary::albums( SortingCriteria sort, bool desc ) const
{
    return Album::listAll( this, sort, desc );
}

std::vector<GenrePtr> MediaLibrary::genres( SortingCriteria sort, bool desc ) const
{
    return Genre::listAll( this, sort, desc );
}

GenrePtr MediaLibrary::genre(int64_t id) const
{
    return Genre::fetch( this, id );
}

ShowPtr MediaLibrary::show( const std::string& name ) const
{
    static const std::string req = "SELECT * FROM " + policy::ShowTable::Name
            + " WHERE name = ?";
    return Show::fetch( this, req, name );
}

std::shared_ptr<Show> MediaLibrary::createShow(const std::string& name)
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

std::vector<ArtistPtr> MediaLibrary::artists(SortingCriteria sort, bool desc) const
{
    return Artist::listAll( this, sort, desc );
}

PlaylistPtr MediaLibrary::createPlaylist( const std::string& name )
{
    return Playlist::create( this, name );
}

std::vector<PlaylistPtr> MediaLibrary::playlists(SortingCriteria sort, bool desc)
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

bool MediaLibrary::addToHistory( const std::string& mrl )
{
    return History::insert( getConn(), mrl );
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

std::shared_ptr<factory::IFileSystem> MediaLibrary::fsFactoryForPath( const std::string& path ) const
{
    for ( const auto& f : m_fsFactories )
    {
        if ( f->isPathSupported( path ) )
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
        std::remove_if( begin( m_fsFactories ), end( m_fsFactories ), []( const std::shared_ptr<factory::IFileSystem> fs ) {
            return fs->isNetworkFileSystem();
        });
    }
}

bool MediaLibrary::banFolder( const std::string& path )
{
    return Folder::blacklist( this, path );
}

bool MediaLibrary::unbanFolder( const std::string& path )
{
    auto folder = Folder::blacklistedFolder( this, path );
    if ( folder == nullptr )
        return false;
    deleteFolder( *folder );

    // We are about to refresh the folder we blacklisted earlier, if we don't have a discoverer, stop early
    if ( m_discoverer == nullptr )
        return true;

    auto parentPath = utils::file::parentDirectory( path );
    // If the parent folder was never added to the media library, the discoverer will reject it.
    // We could check it from here, but that would mean fetching the folder twice, which would be a waste.
    m_discoverer->reload( parentPath );
    return true;
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

void MediaLibrary::onDevicePlugged( const std::string&, const std::string& mountpoint )
{
    for ( const auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isPathSupported( "file://" ) )
        {
            fsFactory->refreshDevices();
            break;
        }
    }
}

void MediaLibrary::onDeviceUnplugged( const std::string& )
{
    for ( const auto& fsFactory : m_fsFactories )
    {
        if ( fsFactory->isPathSupported( "file://" ) )
        {
            fsFactory->refreshDevices();
            break;
        }
    }
}

}
