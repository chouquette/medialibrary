/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
#include <cstring>

#include "Album.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "discoverer/DiscovererWorker.h"
#include "utils/ModificationsNotifier.h"
#include "Chapter.h"
#include "Device.h"
#include "File.h"
#include "Folder.h"
#include "Genre.h"
#include "LockFile.h"
#include "Media.h"
#include "MediaLibrary.h"
#include "Label.h"
#include "logging/Logger.h"
#include "logging/IostreamLogger.h"
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
#include "utils/Charsets.h"
#include "utils/Filename.h"
#include "utils/Directory.h"
#include "utils/Url.h"
#include "utils/File.h"
#include "VideoTrack.h"
#include "Metadata.h"
#include "parser/Task.h"
#include "utils/Charsets.h"
#include "utils/TitleAnalyzer.h"
#include "Bookmark.h"
#include "MediaGroup.h"
#include "utils/Enums.h"
#include "Deprecated.h"

// Discoverers:
#include "discoverer/FsDiscoverer.h"

// Metadata services:
#ifdef HAVE_LIBVLC
#include "metadata_services/vlc/VLCMetadataService.h"
#endif
#include "metadata_services/MetadataParser.h"
#include "metadata_services/LinkService.h"

#include "thumbnails/ThumbnailerWorker.h"

// FileSystem
#include "factory/DeviceListerFactory.h"

#ifdef HAVE_LIBVLC
#include "filesystem/libvlc/FileSystemFactory.h"
#include "filesystem/libvlc/DeviceLister.h"
#include "utils/VLCInstance.h"

#include <vlcpp/vlc.hpp>
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
#include "thumbnails/CoreThumbnailer.h"
using ThumbnailerType = medialibrary::CoreThumbnailer;
#else
#include "thumbnails/VmemThumbnailer.h"
using ThumbnailerType = medialibrary::VmemThumbnailer;
#endif
#endif

#include "medialibrary/filesystem/IDevice.h"
#include "medialibrary/filesystem/Errors.h"

#ifdef __APPLE__
# include <TargetConditionals.h>
#endif

namespace medialibrary
{

const std::vector<const char*> MediaLibrary::SupportedMediaExtensions = {
    "3g2",
    "3ga",
    "3gp",
    "3gp2",
    "3gpp",
    "669",
    "a52",
    "aac",
    "ac3",
    "adt",
    "adts",
    "adx",
    "aif",
    "aifc",
    "aiff",
    "alac",
    "amb",
    "amr",
    "amrec",
    "amv",
    "aob",
    "ape",
    "asf",
    "au",
    "avi",
    "awb",
    "bik",
    "caf",
    "crf",
    "divx",
    "drc",
    "dts",
    "dv",
    "evo",
    "f4v",
    "far",
    "flac",
    "flv",
    "gvi",
    "gxf",
    "iso",
    "it",
    "itml",
    "kar",
    "m1v",
    "m2t",
    "m2ts",
    "m2v",
    "m4a",
    "m4b",
    "m4p",
    "m4v",
    "m5p",
    "mid",
    "mk3d",
    "mka",
    "mkv",
    "mlp",
    "mod",
    "mov",
    "mp1",
    "mp2",
    "mp2v",
    "mp3",
    "mp4",
    "mpa",
    "mpc",
    "mpe",
    "mpeg",
    "mpeg1",
    "mpeg2",
    "mpeg4",
    "mpg",
    "mpv2",
    "mtm",
    "mts",
    "mus",
    "mxf",
    "mxg",
    "nsv",
    "nuv",
    "oga",
    "ogg",
    "ogm",
    "ogv",
    "ogx",
    "oma",
    "opus",
    "ps",
    "qcp",
    "qtl",
    "ra",
    "rec",
    "rm",
    "rmi",
    "rmj",
    "rmvb",
    "rpl",
    "s3m",
    "sid",
    "spx",
    "tak",
    "thd",
    "thp",
    "tod",
    "trp",
    "ts",
    "tta",
    "tts",
    "txd",
    "vlc",
    "vob",
    "voc",
    "vqf",
    "vro",
    "w64",
    "wav",
    "webm",
    "wm",
    "wma",
    "wmv",
    "wmx",
    "wtv",
    "wv",
    "wvx",
    "xa",
    "xesc",
    "xm"
};

const std::vector<const char*> MediaLibrary::SupportedPlaylistExtensions = {
    "asx",
    "b4s",
    "conf",
    /*"cue",*/
    "ifo",
    "m3u",
    "m3u8",
    "pls",
    "ram",
    "sdp",
    "vlc",
    "wax",
    "wpl",
    "xspf"
};

const std::vector<const char*> MediaLibrary::SupportedSubtitleExtensions = {
    "aqt",
    "ass",
    "cdg",
    "dks",
    "idx",
    "jss",
    "mpl2",
    "mpsub",
    "pjs",
    "psb",
    "rt",
    "sami",
    "sbv",
    "scc",
    "smi",
    "srt",
    "ssa",
    "stl",
    "sub",
    "tt",
    "ttml",
    "usf",
    "vtt",
    "webvtt"
};

std::unique_ptr<MediaLibrary> MediaLibrary::create( const std::string& dbPath,
                                                    const std::string& mlFolderPath,
                                                    bool lockFile, const SetupConfig* cfg )
{
    if ( cfg != nullptr && cfg->logger != nullptr )
        Log::SetLogger( cfg->logger );
    else
        Log::SetLogger( std::make_shared<IostreamLogger>() );
    Log::setLogLevel( cfg != nullptr ? cfg->logLevel : LogLevel::Error );

    std::unique_ptr<LockFile> lock;
    if ( lockFile )
    {
        lock = LockFile::lock( mlFolderPath );
        if ( !lock )
            return {};
    }

    return std::make_unique<MediaLibrary>( dbPath, mlFolderPath, std::move( lock ), cfg );
}

MediaLibrary::MediaLibrary( const std::string& dbPath,
                            const std::string& mlFolderPath,
                            std::unique_ptr<LockFile> lockFile,
                            const SetupConfig* cfg )
    : m_settings( this )
    , m_initialized( false )
    , m_discovererIdle( true )
    , m_parserIdle( true )
    , m_dbPath( dbPath )
    , m_mlFolderPath( utils::file::toFolderPath( mlFolderPath ) )
    , m_thumbnailPath( m_mlFolderPath + "thumbnails/" )
    , m_playlistPath( m_mlFolderPath + "playlists/" )
    , m_lockFile( std::move( lockFile ) )
    , m_callback( nullptr )
    , m_fsHolder( this )
    , m_parser( this, &m_fsHolder )
    , m_discovererWorker( this, &m_fsHolder )
{
    if ( cfg != nullptr )
    {
        for ( const auto& p : cfg->deviceListers )
            m_fsHolder.registerDeviceLister( p.first, p.second );
        for ( const auto& fsf : cfg->fsFactories )
        {
            if ( m_fsHolder.addFsFactory( fsf ) == false )
                assert( !"Can't initialize provided file system factory" );
        }
    }
    if ( cfg != nullptr && cfg->parserServices.empty() == false )
    {
        for ( const auto& s : cfg->parserServices )
        {
            if ( s->targetedStep() != parser::Step::MetadataExtraction )
                return;
            m_parser.addService( s );
        }
    }
#ifdef HAVE_LIBVLC
    else
    {
        m_parser.addService( std::make_shared<parser::VLCMetadataService>() );
    }
#endif
    m_parser.addService( std::make_shared<parser::MetadataAnalyzer>() );
    m_parser.addService( std::make_shared<parser::LinkService>() );
}

MediaLibrary::~MediaLibrary()
{
    stopBackgroundJobs();
}

bool MediaLibrary::createAllTables()
{
    // We need to create the tables in order of triggers creation
    // Device is the "root of all evil". When a device is modified,
    // we will trigger an update on folder, which will trigger
    // an update on files, and so on.

    // We may specify the current model version when creating the tables, in order
    // for the old versions to be created as they were before, only to be migrated
    // afterward.
    // Individual migrations might take shortcuts, but it will become increasingly
    // hard to do, as we have to mainting major changes across versions.

    auto dbConn = m_dbConnection.get();

    Device::createTable( dbConn );
    Folder::createTable( dbConn );
    Thumbnail::createTable( dbConn );
    Media::createTable( dbConn );
    File::createTable( dbConn );
    Label::createTable( dbConn );
    Playlist::createTable( dbConn );
    Genre::createTable( dbConn );
    Album::createTable( dbConn );
    Show::createTable( dbConn );
    if ( Show::createUnknownShow( dbConn ) == false )
        return false;
    ShowEpisode::createTable( dbConn );
    Movie::createTable( dbConn );
    VideoTrack::createTable( dbConn );
    AudioTrack::createTable( dbConn );
    Artist::createTable( dbConn );
    if ( Artist::createDefaultArtists( dbConn ) == false )
        return false;
    parser::Task::createTable( dbConn );
    Metadata::createTable( dbConn );
    SubtitleTrack::createTable( dbConn );
    Chapter::createTable( dbConn );
    Bookmark::createTable( dbConn );
    MediaGroup::createTable( dbConn );
    return true;
}

void MediaLibrary::deleteAllTables( sqlite::Connection* dbConn )
{
    /*
     * For the general case, don't probe the database to delete the active tables.
     * If the database is badly corrupted, we might not get any table which would
     * cause a failure down the line.
     */
    const std::string tables[] = {
        Device::Table::Name,
        Device::MountpointTable::Name,
        Folder::Table::Name,
        Folder::FtsTable::Name,
        Thumbnail::Table::Name,
        Thumbnail::LinkingTable::Name,
        Thumbnail::CleanupTable::Name,
        Media::Table::Name,
        Media::FtsTable::Name,
        File::Table::Name,
        Label::Table::Name,
        Label::FileRelationTable::Name,
        Playlist::Table::Name,
        Playlist::FtsTable::Name,
        Playlist::MediaRelationTable::Name,
        Genre::Table::Name,
        Genre::FtsTable::Name,
        Album::Table::Name,
        Album::FtsTable::Name,
        Show::Table::Name,
        Show::FtsTable::Name,
        ShowEpisode::Table::Name,
        Movie::Table::Name,
        VideoTrack::Table::Name,
        AudioTrack::Table::Name,
        Artist::Table::Name,
        Artist::FtsTable::Name,
        Artist::MediaRelationTable::Name,
        parser::Task::Table::Name,
        Metadata::Table::Name,
        SubtitleTrack::Table::Name,
        Chapter::Table::Name,
        Bookmark::Table::Name,
        MediaGroup::Table::Name,
        MediaGroup::FtsTable::Name,
        "Settings",
    };
    for ( const auto& table : tables )
        sqlite::Tools::executeRequest( dbConn, "DROP TABLE IF EXISTS " + table );
    /*
     * Keep probing the database in case it is quite old and still contains a
     * deprecated table not listed above
     */
    const auto leftOverTables = sqlite::Tools::listTables( dbConn );
    for ( const auto& table : leftOverTables )
        sqlite::Tools::executeRequest( dbConn, "DROP TABLE " + table );

}

void MediaLibrary::waitForBackgroundTasksIdle()
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    m_idleCond.wait( lock, [this]() {
        return m_parserIdle == true && m_discovererIdle == true;
    });
}

void MediaLibrary::createAllTriggers()
{
    auto dbConn = m_dbConnection.get();

    Folder::createTriggers( dbConn );
    Folder::createIndexes( dbConn );
    Album::createTriggers( dbConn );
    Album::createIndexes( dbConn );
    Artist::createTriggers( dbConn );
    Artist::createIndexes( dbConn );
    Media::createTriggers( dbConn );
    Media::createIndexes( dbConn );
    File::createIndexes( dbConn );
    Genre::createTriggers( dbConn );
    Playlist::createTriggers( dbConn );
    Playlist::createIndexes( dbConn );
    Label::createTriggers( dbConn );
    Show::createTriggers( dbConn );
    ShowEpisode::createIndexes( dbConn );
    Thumbnail::createTriggers( dbConn );
    Thumbnail::createIndexes( dbConn );
    parser::Task::createTriggers( dbConn );
    AudioTrack::createIndexes( dbConn );
    SubtitleTrack::createIndexes( dbConn );
    VideoTrack::createIndexes( dbConn );
    MediaGroup::createTriggers( dbConn );
    MediaGroup::createIndexes( dbConn );
    Movie::createIndexes( dbConn );
    parser::Task::createIndex( dbConn );
    Bookmark::createIndexes( dbConn );
    Chapter::createIndexes( dbConn );
}

bool MediaLibrary::checkDatabaseIntegrity()
{
    auto schemaOk = Device::checkDbModel( this ) &&
            Folder::checkDbModel( this ) &&
            Thumbnail::checkDbModel( this ) &&
            Media::checkDbModel( this ) &&
            File::checkDbModel( this ) &&
            Label::checkDbModel( this ) &&
            Playlist::checkDbModel( this ) &&
            Genre::checkDbModel( this ) &&
            Album::checkDbModel( this ) &&
            Show::checkDbModel( this ) &&
            ShowEpisode::checkDbModel( this ) &&
            Movie::checkDbModel( this ) &&
            VideoTrack::checkDbModel( this ) &&
            AudioTrack::checkDbModel( this ) &&
            Artist::checkDbModel( this ) &&
            parser::Task::checkDbModel( this ) &&
            Metadata::checkDbModel( this ) &&
            SubtitleTrack::checkDbModel( this ) &&
            Chapter::checkDbModel( this ) &&
            Bookmark::checkDbModel( this ) &&
            MediaGroup::checkDbModel( this );
    return schemaOk == true &&
            m_dbConnection->checkSchemaIntegrity() == true &&
            m_dbConnection->checkForeignKeysIntegrity() == true;
}

void MediaLibrary::registerEntityHooks()
{
    if ( m_modificationNotifier == nullptr )
        return;

    m_dbConnection->registerUpdateHook( Media::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyMediaRemoval( rowId );
        else if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyMediaModification( rowId );
    });
    m_dbConnection->registerUpdateHook( Artist::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyArtistRemoval( rowId );
        else if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyArtistModification( rowId );
    });
    m_dbConnection->registerUpdateHook( Album::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyAlbumRemoval( rowId );
        else if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyAlbumModification( rowId );
    });
    m_dbConnection->registerUpdateHook( Playlist::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyPlaylistRemoval( rowId );
        else if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyPlaylistModification( rowId );
    });
    m_dbConnection->registerUpdateHook( Genre::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyGenreRemoval( rowId );
        else if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyGenreModification( rowId );
    });
    m_dbConnection->registerUpdateHook( Thumbnail::CleanupTable::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Insert )
            m_modificationNotifier->notifyThumbnailCleanupInserted( rowId );
    });
    m_dbConnection->registerUpdateHook( MediaGroup::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyMediaGroupModification( rowId );
        else if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyMediaGroupRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Bookmark::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyBookmarkModification( rowId );
        else if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyBookmarkRemoval( rowId );
    });
    m_dbConnection->registerUpdateHook( Folder::Table::Name,
                                        [this]( sqlite::Connection::HookReason reason, int64_t rowId ) {
        if ( reason == sqlite::Connection::HookReason::Update )
            m_modificationNotifier->notifyFolderModification( rowId );
        else if ( reason == sqlite::Connection::HookReason::Delete )
            m_modificationNotifier->notifyFolderRemoval( rowId );
    });
}

void MediaLibrary::removeThumbnails()
{
    auto thumbnailsFolderMrl = utils::file::toMrl( m_thumbnailPath );
    auto fsFactory = fsFactoryForMrl( thumbnailsFolderMrl );
    if ( fsFactory == nullptr )
    {
        LOG_ERROR( "Failed to create an fs factory to flush the thumbnails" );
        return;
    }
    try
    {
        auto dir = fsFactory->createDirectory( thumbnailsFolderMrl );
        auto files = dir->files();
        for ( const auto& f : files )
        {
            auto path = utils::url::toLocalPath( f->mrl() );
            utils::fs::remove( path );
        }
    }
    catch ( const fs::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to remove thumbnail files: ", ex.what() );
    }
}

InitializeResult MediaLibrary::initialize( IMediaLibraryCb* mlCallback )
{
    assert( !m_dbPath.empty() );
    assert( !m_mlFolderPath.empty() );
    std::unique_lock<compat::Mutex> lock( m_mutex );
    if ( m_initialized == true )
        return InitializeResult::AlreadyInitialized;

    LOG_INFO( "Initializing medialibrary..." );
    LOG_INFO( "Current version is " PROJECT_VERSION ". Database model is ",
              Settings::DbModelVersion );

    auto mlFolder = utils::file::toFolderPath( m_mlFolderPath );
    if ( utils::fs::mkdir( m_thumbnailPath ) == false )
    {
        LOG_ERROR( "Failed to create thumbnail directory (", m_thumbnailPath,
                    ": ", strerror( errno ) );
        return InitializeResult::Failed;
    }
    if ( utils::fs::mkdir( m_playlistPath ) == false )
    {
        LOG_ERROR( "Failed to create playlist export directory (", m_playlistPath,
                    ": ", strerror( errno ) );
        return InitializeResult::Failed;
    }

    m_callback = mlCallback;
    m_dbConnection = sqlite::Connection::connect( m_dbPath );

    onDbConnectionReady( m_dbConnection.get() );

    // Give a chance to test overloads to reject the creation of a notifier
    startDeletionNotifier();
    // Which allows us to register hooks, or not, depending on the presence of a notifier
    registerEntityHooks();

    // Add a local fs factory to be able to flush the thumbnails if required
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // We need to have the device list ready, since we might need it to
    // create backups for playlist on removable devices
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    addLocalFsFactory();

    // This instantiates our own network FsFactory's and adds them to the list of
    // available network factories, but they are not added to m_fsFactories yet.
    // This will be done when the user explicitely
    // invokes IMediaLibrary::setDiscoverNetworkEnabled
    populateNetworkFsFactories();

    auto res = InitializeResult::Success;
    try
    {
        auto t = m_dbConnection->newTransaction();
        Settings::createTable( m_dbConnection.get() );
        if ( m_settings.load() == false )
        {
            LOG_ERROR( "Failed to load settings" );
            return InitializeResult::Failed;
        }
        auto dbModel = m_settings.dbModelVersion();
        if ( dbModel == 0 )
        {
            if ( createAllTables() == false )
                return InitializeResult::Failed;
            createAllTriggers();
            t->commit();
        }
        else
        {
            /*
             * Even though we didn't change anything, ensure we are not running
             * a transaction anymore. Migrations will execute a transaction for
             * each version migration
             */
            t->commit();
            if ( dbModel != Settings::DbModelVersion )
            {
                res = updateDatabaseModel( dbModel );
                if ( res == InitializeResult::Failed )
                {
                    LOG_ERROR( "Failed to update database model" );
                    return res;
                }
            }
        }
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Can't initialize medialibrary: ", ex.what() );
        return InitializeResult::Failed;
    }

    if ( res == InitializeResult::Success || res == InitializeResult::DbReset )
    {
        /* In case the database was detected as corrupted, we don't want to
         * interract with the database, but we want to mark the medialib as
         * initialized, and let the application handle what to do with its
         * corrupted database
         */
        Device::markNetworkAsDeviceMissing( this );
        LOG_INFO( "Successfully initialized" );
    }
    else
    {
        assert( res == InitializeResult::DbCorrupted );
        LOG_WARN( "Initialization complete; Database corruption was detected" );
    }
    m_initialized = true;
    /*
     * Unlock the mutex before invoking the parser. If some tasks are restored,
     * the idle callback will be invoked with the lock held, which would deadlock.
     */
    lock.unlock();
    auto parser = getParser();
    if ( parser != nullptr )
        parser->start();
    return res;
}

void MediaLibrary::setVerbosity( LogLevel v )
{
    Log::setLogLevel( v );
}

MediaPtr MediaLibrary::media( int64_t mediaId ) const
{
    return Media::fetch( this, mediaId );
}

MediaPtr MediaLibrary::media( const std::string& mrl ) const
{
    LOG_DEBUG( "Fetching media from mrl: ", mrl );
    auto file = File::fromExternalMrl( this, mrl );
    if ( file != nullptr )
    {
        LOG_DEBUG( "Found external media: ", mrl );
        return file->media();
    }
    file = File::fromMrl( this, mrl );
    if ( file == nullptr )
        return nullptr;
    return file->media();
}

MediaPtr MediaLibrary::addExternalMedia( const std::string& mrl, int64_t duration )
{
    return Media::createExternal( this, mrl, duration );
}

MediaPtr MediaLibrary::addStream( const std::string& mrl )
{
    return Media::createStream( this, mrl );
}

bool MediaLibrary::removeExternalMedia(MediaPtr media)
{
    if ( media->isDiscoveredMedia() == true )
    {
        assert( !"Invalid (non-external) media provided" );
        return false;
    }
    return Media::destroy( this, media->id() );
}

Query<IMedia> MediaLibrary::audioFiles( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Audio, params, IMedia::SubType::Unknown );
}

Query<IMedia> MediaLibrary::videoFiles( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Video, params, IMedia::SubType::Unknown );
}

Query<IMedia> MediaLibrary::movies( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Video, params, IMedia::SubType::Movie );
}

Query<IMedia> MediaLibrary::inProgressMedia( IMedia::Type type, const QueryParameters *params ) const
{
    return Media::listInProgress( this, type, params );
}

MediaGroupPtr MediaLibrary::createMediaGroup( std::string name )
{
    return MediaGroup::create( this, std::move( name ), true, false );
}

MediaGroupPtr MediaLibrary::createMediaGroup( const std::vector<int64_t>& mediaIds )
{
    return MediaGroup::create( this, mediaIds );
}

bool MediaLibrary::deleteMediaGroup( int64_t id )
{
    auto mg = MediaGroup::fetch( this, id );
    if ( mg == nullptr )
        return false;
    return mg->destroy();
}

MediaGroupPtr MediaLibrary::mediaGroup( int64_t id ) const
{
    return MediaGroup::fetch( this, id );
}

Query<IMediaGroup> MediaLibrary::mediaGroups( IMedia::Type mediaType,
                                              const QueryParameters* params ) const
{
    return MediaGroup::listAll( this, mediaType, params );
}

Query<IMediaGroup> MediaLibrary::searchMediaGroups( const std::string& pattern,
                                                    const QueryParameters* params ) const
{
    return MediaGroup::search( this, pattern, params );
}

bool MediaLibrary::regroupAll()
{
    return Media::regroupAll( this );
}

bool MediaLibrary::isMediaExtensionSupported( const char* ext ) const
{
    return std::binary_search( std::begin( SupportedMediaExtensions ),
        std::end( SupportedMediaExtensions ), ext, [](const char* l, const char* r) {
            return strcasecmp( l, r ) < 0;
        });
}

bool MediaLibrary::isPlaylistExtensionSupported( const char* ext ) const
{
    return std::binary_search( std::begin( SupportedPlaylistExtensions ),
        std::end( SupportedPlaylistExtensions ), ext, [](const char* l, const char* r) {
            return strcasecmp( l, r ) < 0;
    });
}

bool MediaLibrary::isDeviceKnown( const std::string &uuid,
                                  const std::string &mountpoint, bool isRemovable )
{
    auto scheme = utils::url::scheme( mountpoint );
    auto isNetwork = scheme != "file://";
    try
    {
        Device::create( this, uuid, std::move( scheme ), isRemovable, isNetwork );
    }
    catch ( const sqlite::errors::ConstraintUnique& )
    {
        return true;
    }
    return false;
}

bool MediaLibrary::deleteRemovableDevices()
{
    return Device::deleteRemovable( this );
}

const std::vector<const char*>&MediaLibrary::supportedSubtitleExtensions() const
{
    return SupportedSubtitleExtensions;
}

bool MediaLibrary::isSubtitleExtensionSupported(const char* ext) const
{
    return std::binary_search( std::begin( SupportedSubtitleExtensions ),
        std::end( SupportedMediaExtensions ), ext, [](const char* l, const char* r) {
            return strcasecmp( l, r ) < 0;
        });
}

void MediaLibrary::removeOldEntities( MediaLibraryPtr ml )
{
    // Approximate 6 months for old device precision.
    Device::removeOldDevices( ml, std::chrono::seconds{ 3600 * 24 * 30 * 6 } );
    Media::removeOldMedia( ml, std::chrono::seconds{ 3600 * 24 * 30 * 6 } );
}

void MediaLibrary::onDiscoveredFile( std::shared_ptr<fs::IFile> fileFs,
                                     std::shared_ptr<Folder> parentFolder,
                                     std::shared_ptr<fs::IDirectory> parentFolderFs,
                                     IFile::Type fileType )
{
    auto mrl = fileFs->mrl();
    std::unique_ptr<sqlite::Transaction> t;
    auto parser = getParser();
    try
    {
        assert( fileType != IFile::Type::Unknown );
        auto task = parser::Task::create( this, std::move( fileFs ), std::move( parentFolder ),
                              std::move( parentFolderFs ), fileType );
        if ( task != nullptr && parser != nullptr )
            parser->parse( std::move( task ) );
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        // Most likely the file is already scheduled and we restarted the
        // discovery after a crash.
        LOG_INFO( "Failed to insert ", mrl, ": ", ex.what(), ". "
                  "Assuming the file is already scheduled for discovery" );
    }
}

void MediaLibrary::onDiscoveredLinkedFile( const fs::IFile& fileFs,
                                           IFile::Type fileType )
{
    try
    {
        auto task = parser::Task::createLinkTask( this, fileFs.mrl(), fileType,
                                                  fileFs.linkedWith(),
                                                  parser::Task::LinkType::Media, 0 );
        if ( task != nullptr )
        {
            auto parser = getParser();
            if ( parser != nullptr )
                parser->parse( std::move( task ) );
        }
    }
    catch ( const sqlite::errors::ConstraintUnique& ex )
    {
        LOG_INFO( "Failed to create link task for ", fileFs.mrl(), ": ",
                  ex.what(), ". Assuming it was already created before" );
    }
}

void MediaLibrary::onUpdatedFile( std::shared_ptr<File> file,
                                  std::shared_ptr<fs::IFile> fileFs,
                                  std::shared_ptr<Folder> parentFolder,
                                  std::shared_ptr<fs::IDirectory> parentFolderFs )
{
    auto mrl = fileFs->mrl();
    try
    {
        auto task = parser::Task::createRefreshTask( this, std::move( file ),
                                                     std::move( fileFs ),
                                                     std::move( parentFolder ),
                                                     std::move( parentFolderFs ) );
        if ( task != nullptr )
        {
            auto parser = getParser();
            if ( parser != nullptr )
                parser->parse( std::move( task ) );
        }
    }
    catch( const sqlite::errors::ConstraintViolation& ex )
    {
        // Most likely the file is already scheduled and we restarted the
        // discovery after a crash.
        LOG_INFO( "Failed to insert ", mrl, ": ", ex.what(), ". "
                  "Assuming the file is already scheduled for discovery" );
    }
}

LabelPtr MediaLibrary::createLabel( const std::string& label )
{
    try
    {
        return Label::create( this, label );
    }
    catch ( const sqlite::errors::Exception& ex )
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
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to delete label: ", ex.what() );
        return false;
    }
}

AlbumPtr MediaLibrary::album( int64_t id ) const
{
    return Album::fetch( this, id );
}

std::shared_ptr<Album> MediaLibrary::createAlbum( std::string title )
{
    return Album::create( this, std::move( title ) );
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

std::shared_ptr<Show> MediaLibrary::createShow( std::string name )
{
    return Show::create( this, std::move( name ) );
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
    auto t = getConn()->newTransaction();
    auto movie = Movie::create( this, media.id() );
    if ( media.setMovie( movie ) == false )
        return nullptr;
    t->commit();
    return movie;
}

ArtistPtr MediaLibrary::artist( int64_t id ) const
{
    return Artist::fetch( this, id );
}

std::shared_ptr<Artist> MediaLibrary::createArtist( std::string name )
{
    return Artist::create( this, std::move( name ) );
}

Query<IArtist> MediaLibrary::artists( ArtistIncluded included,
                                      const QueryParameters* params ) const
{
    return Artist::listAll( this, included, params );
}

PlaylistPtr MediaLibrary::createPlaylist( std::string name )
{
    try
    {
        auto pl = Playlist::create( this, std::move( name ) );
        if ( pl != nullptr && m_modificationNotifier != nullptr )
            m_modificationNotifier->notifyPlaylistCreation( pl );
        return pl;
    }
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to create a playlist: ", ex.what() );
        return nullptr;
    }
}

Query<IPlaylist> MediaLibrary::playlists( PlaylistType type,
                                          const QueryParameters* params )
{
    return Playlist::listAll( this, type, params );
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
    catch ( const sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to delete playlist: ", ex.what() );
        return false;
    }
}

Query<IMedia> MediaLibrary::history() const
{
    return Media::fetchHistory( this );
}

Query<IMedia> MediaLibrary::history( IMedia::Type type ) const
{
    return Media::fetchHistory( this, type );
}

Query<IMedia> MediaLibrary::streamHistory() const
{
    return Media::fetchStreamHistory( this );
}

bool MediaLibrary::clearHistory()
{
    try
    {
        return Media::clearHistory( this );
    }
    catch ( sqlite::errors::Exception& ex )
    {
        LOG_ERROR( "Failed to clear history: ", ex.what() );
        return false;
    }
}

Query<IMedia> MediaLibrary::searchMedia( const std::string& title,
                                                const QueryParameters* params ) const
{
    return Media::search( this, title, params );
}

Query<IMedia> MediaLibrary::searchAudio( const std::string& pattern, const QueryParameters* params ) const
{
    return Media::search( this, pattern, IMedia::Type::Audio, params );
}

Query<IMedia> MediaLibrary::searchVideo( const std::string& pattern, const QueryParameters* params ) const
{
    return Media::search( this, pattern, IMedia::Type::Video, params );
}

Query<IPlaylist> MediaLibrary::searchPlaylists( const std::string& name,
                                                const QueryParameters* params ) const
{
    return Playlist::search( this, name, params );
}

Query<IAlbum> MediaLibrary::searchAlbums( const std::string& pattern,
                                          const QueryParameters* params ) const
{
    return Album::search( this, pattern, params );
}

Query<IGenre> MediaLibrary::searchGenre( const std::string& genre,
                                         const QueryParameters* params ) const
{
    return Genre::search( this, genre, params );
}

Query<IArtist> MediaLibrary::searchArtists( const std::string& name,
                                            ArtistIncluded included,
                                            const QueryParameters* params ) const
{
    return Artist::search( this, name, included, params );
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
    res.artists = searchArtists( pattern, ArtistIncluded::All, params );
    res.genres = searchGenre( pattern, params );
    res.media = searchMedia( pattern, params );
    res.playlists = searchPlaylists( pattern, params );
    res.shows = searchShows( pattern, params );
    return res;
}

void MediaLibrary::startDeletionNotifier()
{
    m_modificationNotifier.reset( new ModificationNotifier( this ) );
    m_modificationNotifier->start();
}

void MediaLibrary::startThumbnailer() const
{
#ifdef HAVE_LIBVLC
    if ( m_thumbnailer == nullptr )
    {
        m_thumbnailer = std::make_unique<ThumbnailerType>();
    }
#else
    assert( m_thumbnailer != nullptr );
#endif
    m_thumbnailerWorker = std::make_unique<ThumbnailerWorker>( this, m_thumbnailer );
}

void MediaLibrary::populateNetworkFsFactories()
{
#ifdef HAVE_LIBVLC
    m_fsHolder.addFsFactory( std::make_shared<fs::libvlc::FileSystemFactory>( "smb://" ) );
#endif
}

void MediaLibrary::onDbConnectionReady( sqlite::Connection* )
{
}

void MediaLibrary::stopBackgroundJobs()
{
    {
        std::lock_guard<compat::Mutex> lock{ m_thumbnailerWorkerMutex };
        m_thumbnailerWorker.reset();
    }
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    m_discovererWorker.stop();
    m_parser.stop();
}

void MediaLibrary::addLocalFsFactory()
{
#ifdef HAVE_LIBVLC
    if ( m_fsHolder.addFsFactory( std::make_shared<fs::libvlc::FileSystemFactory>( "file://" ) ) == false )
    {
        assert( !"Can't add local file system factory" );
    }
#endif
}

InitializeResult MediaLibrary::updateDatabaseModel( unsigned int previousVersion )
{
    LOG_INFO( "Updating database model from ", previousVersion, " to ", Settings::DbModelVersion );
    auto originalPreviousVersion = previousVersion;

    Playlist::backupPlaylists( this, previousVersion );

    // Up until model 3, it's safer (and potentially more efficient with index changes) to drop the DB
    // It's also way simpler to implement
    // In case of downgrade, just recreate the database
    auto success = false;
    auto needRescan = false;
    for ( auto i = 0u; i < 3; ++i )
    {
        if ( i > 0 )
        {
            LOG_WARN( "Retrying database migration, attempt ", i + 1, " / 3" );
        }
        try
        {
            // Up until model 3, it's safer (and potentially more efficient with index changes) to drop the DB
            // It's also way simpler to implement
            // In case of downgrade, just recreate the database
            // We might also have some special cases for failed upgrade (see
            // comments below for per-version details)
            if ( previousVersion < 15 ||
                 previousVersion > Settings::DbModelVersion )
            {
                if( recreateDatabase() == false )
                    throw std::runtime_error( "Failed to recreate the database" );
                return InitializeResult::DbReset;
            }
            if ( previousVersion == 15 )
            {
                migrateModel15to16();
                previousVersion = 16;
            }
            if ( previousVersion == 16 )
            {
                migrateModel16to17();
                previousVersion = 17;
                needRescan = true;
            }
            if ( previousVersion == 17 )
            {
                migrateModel17to18( originalPreviousVersion );
                needRescan = true;
                previousVersion = 18;
            }
            if ( previousVersion == 18 )
            {
                // Even though this migration doesn't force a rescan, don't
                // override any potential previous request.
                needRescan |= migrateModel18to19();
                previousVersion = 19;
            }
            if ( previousVersion == 19 )
            {
                migrateModel19to20();
                needRescan = true;
                previousVersion = 20;
            }
            if ( previousVersion == 20 )
            {
                migrateModel20to21();
                previousVersion = 21;
            }
            if ( previousVersion == 21 )
            {
                migrateModel21to22();
                // Use this migration to ensure playlists containing the same
                // item twice or more are now properly scanned, and to regenerate
                // the video titles with the new title sanitizer
                needRescan = true;
                previousVersion = 22;
            }
            if ( previousVersion == 22 )
            {
                migrateModel22to23();
                needRescan = true;
                previousVersion = 23;
            }
            if ( previousVersion == 23 )
            {
                migrateModel23to24();
                needRescan = true;
                previousVersion = 24;
            }
            if ( previousVersion == 24 )
            {
                migrateModel24to25();
                /*
                 * Force a rescan to assign all potentially ungrouped media to
                 * a group.
                 * We are now assuming that a media is always grouped, so we
                 * need to ensure this is true
                 */
                needRescan = true;
                previousVersion = 25;
            }
            if ( previousVersion == 25 )
            {
                migrateModel25to26();
                needRescan = true;
                previousVersion = 26;
            }
            if ( previousVersion == 26 )
            {
                migrateModel26to27();
                /*
                 * Force a rescan to account for recent media groups changes
                 * and trigger a regroup that accounts for utf8
                 * This is not directly related to this migration but needs
                 * to be done regardless.
                 */
                needRescan = true;
                previousVersion = 27;
            }
            if ( previousVersion == 27 )
            {
                migrateModel27to28();
                previousVersion = 28;
            }
            if ( previousVersion == 28 )
            {
                migrateModel28to29();
                previousVersion = 29;
            }
            if ( previousVersion == 29 )
            {
                migrateModel29to30();
                previousVersion = 30;
                needRescan = true;
            }
            if ( previousVersion == 30 )
            {
                migrateModel30to31();
                previousVersion = 31;
            }
            if ( previousVersion == 31 )
            {
                migrateModel31to32();
                previousVersion = 32;
                needRescan = true;
            }
            if ( previousVersion == 32 )
            {
                migrateModel32to33();
                previousVersion = 33;
            }
            if ( previousVersion == 33 )
            {
                migrateModel33to34();
                previousVersion = 34;
                needRescan = true;
            }
            if ( previousVersion == 34 )
            {
                migrateModel34to35();
                previousVersion = 35;
            }
            if ( previousVersion == 35 )
            {
                migrateModel35to36();
                previousVersion = 36;
                needRescan = true;
            }
            if ( previousVersion == 36 )
            {
                migrateModel36to37();
                previousVersion = 37;
            }
            // To be continued in the future!

            migrationEpilogue( originalPreviousVersion );

            success = true;
            break;
        }
        catch( const std::exception& ex )
        {
            LOG_ERROR( "An error occurred during the database upgrade: ",
                       ex.what() );
        }
        catch( ... )
        {
            LOG_ERROR( "An unknown error occurred during the database upgrade." );
        }
    }
    // If we failed 3 times to migrate the database, assume the database to be
    // corrupted.
    if ( success == false )
        return InitializeResult::DbCorrupted;

    if ( needRescan == true )
    {
        if ( forceRescan() == false )
        {
            LOG_WARN( "Failed to force a rescan" );
        }
    }

    // Safety check: ensure we didn't forget a migration along the way
    assert( previousVersion == Settings::DbModelVersion );
    assert( previousVersion == m_settings.dbModelVersion() );


    auto ctx = getConn()->acquireReadContext();
    if ( checkDatabaseIntegrity() == false )
        return InitializeResult::DbCorrupted;

    return InitializeResult::Success;
}

bool MediaLibrary::recreateDatabase()
{
    sqlite::Connection::DisableForeignKeyContext ctx{ m_dbConnection.get() };
    {
        auto t = m_dbConnection->newTransaction();
        deleteAllTables( m_dbConnection.get() );
        sqlite::Statement::FlushStatementCache();
        Settings::createTable( m_dbConnection.get() );
        if ( createAllTables() == false )
            return false;
        createAllTriggers();

        // We dropped the database, there is no setting to be read anymore
        if ( m_settings.load() == false )
            return false;
        t->commitNoUnlock();
        /*
         * Now that we removed all the tables, flush all the connections to avoid
         * Database is locked errors. This needs to be done while we still hold the
         * sqlite lock to avoid concurrent access to the database with stalled
         * connections or stalled precompiled requests.
         * See https://www.sqlite.org/c3ref/close.html
         */
        m_dbConnection->flushAll();
    }

    /*
     * We just delete all tables but this won't invoke the thumbnails deletion
     * hooks, so we need to delete the files ourselves
     */
    removeThumbnails();

    return checkDatabaseIntegrity();
}

/**
 * Model 15 to 16 migration:
 * - Remove update_playlist_order trigger
 * - Add update_playlist_order_on_delete
 * - Update trigger update_playlist_order_on_insert
 * - Enforce contiguous position indexes on PlaylistMediaRelation
 * - Changes in MRL encoding
 */
void MediaLibrary::migrateModel15to16()
{
    auto dbConn = getConn();
    auto t = dbConn->newTransaction();
    std::string reqs[] = {
#               include "database/migrations/migration15-16.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );

    // Migrate files mrl encoding
    {
        auto files = File::fetchAll<File>( this );
        for ( auto& f : files )
        {
            auto newMrl = utils::url::encode( utils::url::decode( f->rawMrl() ) );
            f->setMrl( std::move( newMrl ) );
        }
    }

    // Migrate folders
    {
        auto folders = Folder::fetchAll<Folder>( this );
        for ( auto& f : folders )
        {
            auto newMrl = utils::url::encode( utils::url::decode( f->rawMrl() ) );
            f->setMrl( std::move( newMrl ) );
        }
    }

    m_settings.setDbModelVersion( 16 );
    t->commit();
}

/**
 * Model 16 to 17 migration:
 * - Remove Media.thumbnail_id
 * - Remove Album.thumbnail_id
 * - Remove Artist.thumbnail_id
 * - Add ThumbnailLinking table
 * - Move thumbnail origin to the thumbnail linking table
 */
void MediaLibrary::migrateModel16to17()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();
    std::string reqs[] = {
#               include "database/migrations/migration16-17.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 17 );
    t->commit();
}

/**
 * Model 17 to 18 migration:
 * - Add thumbnail.shared_counter
 * - Add Task.type
 * - Add Task.link_to_id
 * - Add Task.link_to_type
 * - Add Task.link_extra
 */
void MediaLibrary::migrateModel17to18( uint32_t originalPreviousVersion )
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration17-18.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    if ( originalPreviousVersion <= 15 )
    {
        // Migrate tasks
        {
            sqlite::Row row;
            int64_t taskId;
            std::string mrl;

            auto batchSize = 100u;
            auto offset = 0u;
            const std::string req = "SELECT id_task, mrl FROM " +
                    parser::Task::Table::Name + " LIMIT ? OFFSET ?";

            while ( true )
            {
                /*
                 * Since we change the parameter along the loop, we need to recreate
                 * the statement for each iteration, in order to reset the bindings
                 */
                sqlite::Statement stmt{ req };
                stmt.execute( batchSize, offset );
                auto nbRow = 0u;
                while ( ( row = stmt.row() ) != nullptr )
                {
                    row >> taskId >> mrl;
                    auto newMrl = utils::url::encode( utils::url::decode( mrl ) );
                    if ( newMrl != mrl )
                        parser::Task::setMrl( this, taskId, newMrl );
                    nbRow++;
                }
                if ( nbRow < batchSize )
                    break;
                offset += nbRow;
            }
        }
    }

    m_settings.setDbModelVersion( 18 );
    t->commit();
}

/**
 * 18 to 19 model migration
 * This is a best guess attempt at replaying a part of the 17/18 migration which
 * didn't go well on some android devices
 */
bool MediaLibrary::migrateModel18to19()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };

    std::string reqs[] = {
#       include "database/migrations/migration18-19.sql"
    };

    try
    {
        auto t = dbConn->newTransaction();
        for ( const auto& req : reqs )
            sqlite::Tools::executeRequest( dbConn, req );
        m_settings.setDbModelVersion( 19 );
        t->commit();
        return true;
    }
    catch ( const sqlite::errors::Exception& )
    {
        // Ignoring, this is because parent_playlist_id column doesn't exist
        // anymore, which means the 17->18 migration completed properly.
    }
    // The previous migration succeeded, we just need to bump the version
    m_settings.setDbModelVersion( 19 );
    return false;
}

/**
 * @brief MediaLibrary::migrateModel19to20
 * - Remove leftover AlbumTrack.is_present field
 * - Remove leftover ShowEpisode.artwork_mrl field
 * - Ensure Genre.name is case insensitive
 * - Enforce Task.link_to_id to be non NULL
 * - Include Task.link_to_id in the task uniqueness constraint
 */
void MediaLibrary::migrateModel19to20()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration19-20.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 20 );
    t->commit();
}

void MediaLibrary::migrateModel20to21()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration20-21.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 21 );
    t->commit();
}

void MediaLibrary::migrateModel21to22()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration21-22.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 22 );
    t->commit();
}

void MediaLibrary::migrateModel22to23()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration22-23.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 23 );
    t->commit();
}

void MediaLibrary::migrateModel23to24()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration23-24.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    /* Recreate the settings table to remove the 2 extra VideoGroups columns */
    Settings::createTable( dbConn );
    m_settings.load();

    {
        sqlite::Row row;
        int64_t mediaId;
        std::string title;
        std::string fileName;

        auto batchSize = 100u;
        auto offset = 0u;
        /* We can't really check if the title was forced for audio media since
         * this would require a preparse of each media.
         * We also don't care about external media (ie. with no parent folder)
         * since those are not analyzed, and we wouldn't override their titles.
         */
        const std::string req = "SELECT id_media, title, filename FROM " +
                Media::Table::Name + " WHERE type != ? AND folder_id IS NOT NULL "
                "LIMIT ? OFFSET ?";

        while ( true )
        {
            /*
             * Since we change the parameter along the loop, we need to recreate
             * the statement for each iteration, in order to reset the bindings
             */
            sqlite::Statement stmt{ req };
            stmt.execute( Media::Type::Audio, batchSize, offset );
            auto nbRow = 0u;
            while ( ( row = stmt.row() ) != nullptr )
            {
                row >> mediaId >> title >> fileName;
                auto sanitizedName = utils::title::sanitize( fileName );
                if ( sanitizedName != title )
                {
                    /*
                     * If the results differ, assume this is because the user
                     * provided a custom title. This should be OK since the latest
                     * changes in the title analyzer have been shipped in all
                     * apps that are using the model 23 which we are currently
                     * migrating from.
                     */
                    Media::setForcedTitle( this, mediaId );
                }
                nbRow++;
            }
            if ( nbRow < batchSize )
                break;
            offset += nbRow;
        }
    }

    m_settings.setDbModelVersion( 24 );
    t->commit();
}

void MediaLibrary::migrateModel24to25()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration24-25.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
    m_settings.setDbModelVersion( 25 );
    t->commit();
}

void MediaLibrary::migrateModel25to26()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration25-26.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    {
        sqlite::Row row;
        int64_t fileId;
        std::string mrl;

        auto batchSize = 100u;
        auto offset = 0u;
        const std::string req = "SELECT id_file, mrl FROM " +
                File::Table::Name + " WHERE is_external = 1 OR is_network = 1 "
                "AND ( mrl LIKE '%#%40%' ESCAPE '#' OR mrl LIKE '%#%3A%' ESCAPE '#')"
                "LIMIT ? OFFSET ?";

        while ( true )
        {
            /*
             * Since we change the parameter along the loop, we need to recreate
             * the statement for each iteration, in order to reset the bindings
             */
            sqlite::Statement stmt{ req };
            stmt.execute( batchSize, offset );
            auto nbRow = 0u;
            while ( ( row = stmt.row() ) != nullptr )
            {
                row >> fileId >> mrl;
                auto newMrl = utils::url::encode( utils::url::decode( mrl ) );
                if ( mrl != newMrl )
                    File::setMrl( this, newMrl, fileId );
                nbRow++;
            }
            if ( nbRow < batchSize )
                break;
            offset += nbRow;
        }
    }

    m_settings.setDbModelVersion( 26 );
    t->commit();
}

void MediaLibrary::migrateModel26to27()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration26-27.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    Settings::createTable( dbConn );
    m_settings.load();

    m_settings.setDbModelVersion( 27 );
    t->commit();
}

void MediaLibrary::migrateModel27to28()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration27-28.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    Settings::createTable( dbConn );
    m_settings.load();

    m_settings.setDbModelVersion( 28 );
    t->commit();
}

void MediaLibrary::migrateModel28to29()
{
    /*
     * This migration doesn't impact the database model but attempts to fix
     * invalid URL encoding for playlists content.
     * See #255
     */
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    const std::string req = "SELECT f.id_file, f.mrl FROM " + File::Table::Name + " f "
            "INNER JOIN " + Media::Table::Name + " m ON f.media_id = m.id_media "
            "WHERE f.type = ? AND m.nb_playlists > 0";
    sqlite::Statement stmt{ req };
    stmt.execute( IFile::Type::Main );
    sqlite::Row row;
    while ( ( row = stmt.row() ) != nullptr )
    {
        int64_t fileId;
        std::string mrl;
        row >> fileId >> mrl;
        assert( row.hasRemainingColumns() == false );
        std::string reEncodedMrl = utils::url::encode( utils::url::decode( mrl ) );
        if ( reEncodedMrl != mrl )
            File::setMrl( this, reEncodedMrl, fileId );
    }

    m_settings.setDbModelVersion( 29 );
    t->commit();
}

void MediaLibrary::migrateModel29to30()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration29-30.sql"
    };

    Playlist::recoverNullMediaID( this );

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 30 );
    t->commit();
}

void MediaLibrary::migrateModel30to31()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration30-31.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 31 );
    t->commit();
}

void MediaLibrary::migrateModel31to32()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration31-32.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );


    m_settings.setDbModelVersion( 32 );
    t->commit();
}

void MediaLibrary::migrateModel32to33()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration32-33.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 33 );
    t->commit();
}

void MediaLibrary::migrateModel33to34()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration33-34.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 34 );
    t->commit();
}

void MediaLibrary::migrateModel34to35()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration34-35.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 35 );
    t->commit();
}

void MediaLibrary::migrateModel35to36()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration35-36.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 36 );
    t->commit();
}

void MediaLibrary::migrateModel36to37()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();

    std::string reqs[] = {
#       include "database/migrations/migration36-37.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );

    m_settings.setDbModelVersion( 37 );
    t->commit();
}

void MediaLibrary::migrationEpilogue( uint32_t )
{
}

void MediaLibrary::reload()
{
    m_discovererWorker.reload();
    /*
     * Initialize the parser and implicitely restart uncompleted parser tasks
     * If the parser was already started, this is a no-op
     */
    getParser();
}

void MediaLibrary::reload( const std::string& entryPoint )
{
    m_discovererWorker.reload( entryPoint );
    getParser();
}

bool MediaLibrary::forceParserRetry()
{
    return parser::Task::resetRetryCount( this );
}

bool MediaLibrary::clearDatabase( bool restorePlaylists )
{
    pauseBackgroundOperations();
    waitForBackgroundTasksIdle();
    auto parser = getParser();
    if ( parser != nullptr )
        parser->flush();
    // If we don't care about playlists, take a shortcut.
    if ( restorePlaylists == false )
    {
        if ( recreateDatabase() == false )
            return false;
        resumeBackgroundOperations();
        return true;
    }

    auto playlistsBackups = Playlist::loadBackups( this );

    // If we have a backup from the last minute, assume this is after a migration
    // and we don't need to try to generate a new backup
    // Otherwise, try to generate a new backup. If it works, cool, if it
    // doesn't, we'll use an old one. Best effort is all we can do if the
    // database is broken
    if ( playlistsBackups.empty() == true ||
         playlistsBackups.rbegin()->first < time(nullptr) - 60 )
    {
        uint32_t currentModel;
        {
            auto ctx = m_dbConnection->acquireReadContext();
            sqlite::Statement s{ ctx.handle(),
                        "SELECT db_model_version FROM Settings"
            };
            auto row = s.row();
            row >> currentModel;
        }
        auto newBackup = Playlist::backupPlaylists( this, currentModel );
        if ( std::get<0>( newBackup ) == true )
            playlistsBackups.emplace( std::get<1>( newBackup ),
                                      std::move( std::get<2>( newBackup ) ) );
    }

    // Create a new playlist backups

    if ( recreateDatabase() == false )
        return false;

    if ( playlistsBackups.empty() == false )
    {
        if ( playlistsBackups.size() > 2 )
        {
            auto playlistFolderMrl = utils::file::toMrl( m_playlistPath );
            while ( playlistsBackups.size() > 2 )
            {
                auto backupPath = utils::file::toFolderPath( playlistFolderMrl +
                        std::to_string( playlistsBackups.begin()->first ) );
                utils::fs::rmdir( backupPath );
                playlistsBackups.erase( playlistsBackups.begin() );
            }
        }
        auto& backup = playlistsBackups.rbegin()->second;
        for ( const auto& mrl : backup )
        {
            LOG_DEBUG( "Queuing restore task for ", mrl );
            auto task = parser::Task::createRestoreTask( this, mrl, IFile::Type::Playlist );
            if ( task != nullptr )
            {
                if ( parser != nullptr )
                    parser->parse( std::move( task ) );
            }
        }
    }
    resumeBackgroundOperations();
    return true;
}

void MediaLibrary::pauseBackgroundOperations()
{
    m_discovererWorker.pause();
    m_parser.pause();
    std::lock_guard<compat::Mutex> thumbLock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker != nullptr )
        m_thumbnailerWorker->pause();
}

void MediaLibrary::resumeBackgroundOperations()
{
    m_discovererWorker.resume();
    m_parser.resume();
    std::lock_guard<compat::Mutex> thumbLock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker != nullptr )
        m_thumbnailerWorker->resume();
}

void MediaLibrary::onBackgroundTasksIdleChanged( bool idle )
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
    LOG_DEBUG( "Setting background idle state to ",
              idle ? "true" : "false" );
    m_callback->onBackgroundTasksIdleChanged( idle );
    m_idleCond.notify_all();
}

void MediaLibrary::onDiscovererIdleChanged( bool idle )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    if ( m_discovererIdle != idle )
    {
        m_discovererIdle = idle;
        // If any idle state changed to false, then we need to trigger the callback.
        // If switching to idle == true, then both background workers need to be idle before signaling.
        LOG_DEBUG( idle ? "Discoverer thread went idle" : "Discover thread was resumed" );
        if ( idle == false || m_parserIdle == true )
            onBackgroundTasksIdleChanged( idle );
    }
}

void MediaLibrary::onParserIdleChanged( bool idle )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    if ( m_parserIdle != idle )
    {
        m_parserIdle = idle;
        LOG_DEBUG( idle ? "All parser services went idle" : "Parse services were resumed" );
        if ( idle == false || m_discovererIdle == true )
            onBackgroundTasksIdleChanged( idle );
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

parser::Parser* MediaLibrary::getParser() const
{
    return const_cast<parser::Parser*>( &m_parser );
}

ThumbnailerWorker* MediaLibrary::thumbnailer() const
{
    std::unique_lock<compat::Mutex> lock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker == nullptr )
        startThumbnailer();
    return m_thumbnailerWorker.get();
}

DeviceListerPtr MediaLibrary::deviceLister( const std::string& scheme ) const
{
    return m_fsHolder.deviceLister( scheme );
}

std::shared_ptr<fs::IFileSystemFactory> MediaLibrary::fsFactoryForMrl( const std::string& mrl ) const
{
    return m_fsHolder.fsFactoryForMrl( mrl );
}

void MediaLibrary::discover( const std::string& entryPoint )
{
    m_discovererWorker.discover( entryPoint );
}

bool MediaLibrary::setDiscoverNetworkEnabled( bool enabled )
{
    return m_fsHolder.setNetworkEnabled( enabled );
}

bool MediaLibrary::isDiscoverNetworkEnabled() const
{
    return m_fsHolder.isNetworkEnabled();
}

Query<IFolder> MediaLibrary::roots( const QueryParameters* params ) const
{
    return Folder::roots( this, false, 0, params );
}

bool MediaLibrary::isIndexed( const std::string& mrl ) const
{
    auto folderMrl = utils::file::directory( mrl );
    auto folder = Folder::fromMrl( this, folderMrl );
    return folder != nullptr;
}

bool MediaLibrary::isBanned( const std::string& mrl ) const
{
    auto folder = Folder::fromMrl( this, mrl, Folder::BannedType::Any );
    if ( folder == nullptr )
        return false;
    return folder->isBanned();
}

Query<IFolder> MediaLibrary::folders( IMedia::Type type,
                                      const QueryParameters* params ) const
{
    return Folder::withMedia( this, type, params );
}

Query<IFolder> MediaLibrary::searchFolders( const std::string& pattern,
                                            IMedia::Type type,
                                            const QueryParameters* params ) const
{
    return Folder::searchWithMedia( this, pattern, type, params );
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
    m_discovererWorker.remove( entryPoint );
}

void MediaLibrary::banFolder( const std::string& entryPoint )
{
    m_discovererWorker.ban( entryPoint );
}

void MediaLibrary::unbanFolder( const std::string& entryPoint )
{
    m_discovererWorker.unban( entryPoint );
}

Query<IFolder> MediaLibrary::bannedEntryPoints() const
{
    return Folder::roots( this, true, 0, nullptr );
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

const std::string& MediaLibrary::playlistPath() const
{
    return m_playlistPath;
}

void MediaLibrary::startFsFactory( fs::IFileSystemFactory &fsFactory ) const
{
    m_fsHolder.startFsFactory( fsFactory );
}

bool MediaLibrary::forceRescan()
{
    if ( m_parser.isRunning() == true )
        m_parser.flush();
    {
        auto t = getConn()->newTransaction();
        // Let the triggers clear out the Fts tables
        if ( Media::resetSubTypes( this ) == false ||
             Genre::deleteAll( this ) == false ||
             Album::deleteAll( this ) == false ||
             Artist::deleteAll( this ) == false ||
             Movie::deleteAll( this ) == false ||
             ShowEpisode::deleteAll( this ) == false ||
             Show::deleteAll( this ) == false ||
             VideoTrack::deleteAll( this ) == false ||
             AudioTrack::deleteAll( this ) == false ||
             SubtitleTrack::deleteAll( this ) == false ||
             Playlist::clearExternalPlaylistContent( this ) == false ||
             parser::Task::removePlaylistContentTasks( this ) == false ||
             parser::Task::resetParsing( this ) == false ||
             Artist::createDefaultArtists( getConn() ) == false ||
             Show::createUnknownShow( getConn() ) == false ||
             Thumbnail::deleteAll( this ) == false ||
             Thumbnail::removeAllCleanupRequests( this ) == false )
        {
                return false;
        }

        t->commit();
    }
    removeThumbnails();
    if ( m_parser.isRunning() == true )
    {
        m_callback->onRescanStarted();
        m_parser.rescan();
    }
    return true;
}

void MediaLibrary::enableFailedThumbnailRegeneration()
{
    Thumbnail::deleteFailureRecords( this );
}

void MediaLibrary::addThumbnailer( std::shared_ptr<IThumbnailer> thumbnailer )
{
    if ( m_thumbnailer != nullptr )
    {
        // We only support a single thumbnailer for videos for now.
        LOG_WARN( "Discarding previous thumbnailer since one has already been provided" );
    }
    m_thumbnailer = std::move( thumbnailer );
}

const std::vector<const char*>& MediaLibrary::supportedMediaExtensions() const
{
    return SupportedMediaExtensions;
}

const std::vector<const char*>& MediaLibrary::supportedPlaylistExtensions() const
{
    return SupportedPlaylistExtensions;
}

bool MediaLibrary::requestThumbnail( int64_t mediaId, ThumbnailSizeType sizeType,
                                     uint32_t desiredWidth, uint32_t desiredHeight,
                                     float position )
{
    auto worker = thumbnailer();
    if ( worker == nullptr )
        return false;
    worker->requestThumbnail( mediaId, sizeType, desiredWidth, desiredHeight, position );
    return true;
}

BookmarkPtr MediaLibrary::bookmark( int64_t bookmarkId ) const
{
    return Bookmark::fetch( this, bookmarkId );
}

bool MediaLibrary::setExternalLibvlcInstance( libvlc_instance_t* inst )
{
#ifndef HAVE_LIBVLC
    (void) inst;
    LOG_ERROR( "Trying to provide a libvlc instance with libvlc disabled" );
    return false;
#else
    LOG_INFO( "Setting external libvlc instance: ", inst );
    VLCInstance::set( inst );
    return true;
#endif
}

/* Defined out of the header for PIMPL to work with unique_ptr */
PriorityAccess::PriorityAccess( std::unique_ptr<PriorityAccessImpl> p )
    : p( std::move( p ) ) {}
PriorityAccess::PriorityAccess( PriorityAccess&& ) = default;
PriorityAccess::~PriorityAccess() = default;

PriorityAccess MediaLibrary::acquirePriorityAccess()
{
    auto dbConn = m_dbConnection.get();
    return std::make_unique<PriorityAccessImpl>( dbConn->acquirePriorityContext() );
}

bool MediaLibrary::flushUserProvidedThumbnails()
{
    return Thumbnail::flushUserProvided( this );
}

}
