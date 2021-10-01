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
#include "AlbumTrack.h"
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
    "wpl",
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
                                                    bool lockFile )
{
    std::unique_ptr<LockFile> lock;

    if ( lockFile )
    {
        lock = LockFile::lock( mlFolderPath );
        if ( !lock )
            return {};
    }

    return std::make_unique<MediaLibrary>( dbPath, mlFolderPath, std::move( lock ) );
}

MediaLibrary::MediaLibrary( const std::string& dbPath,
                            const std::string& mlFolderPath,
                            std::unique_ptr<LockFile> lockFile )
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
{
    Log::setLogLevel( LogLevel::Error );
}

MediaLibrary::~MediaLibrary()
{
    // Explicitely stop the discoverer, to avoid it writting while tearing down.
    if ( m_discovererWorker != nullptr )
    {
        m_fsHolder.unregisterCallback( m_discovererWorker.get() );
        m_discovererWorker->stop();
    }
    if ( m_parser != nullptr )
    {
        m_fsHolder.unregisterCallback( m_parser.get() );
        m_parser->stop();
    }
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
    AlbumTrack::createTable( dbConn );
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
    auto tables = sqlite::Tools::listTables( dbConn );
    assert( sqlite::Transaction::isInProgress() == false );
    sqlite::Connection::WeakDbContext ctx{ dbConn };
    auto t = dbConn->newTransaction();
    for ( const auto& table : tables )
        sqlite::Tools::executeRequest( dbConn, "DROP TABLE " + table );
    t->commit();
}

void MediaLibrary::createAllTriggers()
{
    auto dbConn = m_dbConnection.get();

    Folder::createTriggers( dbConn );
    Folder::createIndexes( dbConn );
    Album::createTriggers( dbConn );
    Album::createIndexes( dbConn );
    AlbumTrack::createIndexes( dbConn );
    Artist::createTriggers( dbConn );
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
            AlbumTrack::checkDbModel( this ) &&
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
    m_dbConnection->registerUpdateHook( Playlist::MediaRelationTable::Name,
                                        [this]( sqlite::Connection::HookReason, int64_t rowId ) {
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
    std::lock_guard<compat::Mutex> lock( m_mutex );
    if ( m_initialized == true )
        return InitializeResult::AlreadyInitialized;

    LOG_INFO( "Initializing medialibrary..." );

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
        Settings::createTable( m_dbConnection.get() );
        if ( m_settings.load() == false )
        {
            LOG_ERROR( "Failed to load settings" );
            return InitializeResult::Failed;
        }
        auto dbModel = m_settings.dbModelVersion();
        if ( dbModel == 0 )
        {
            auto t = m_dbConnection->newTransaction();
            if ( createAllTables() == false )
                return InitializeResult::Failed;
            createAllTriggers();
            t->commit();
        }
        else if ( dbModel != Settings::DbModelVersion )
        {
            res = updateDatabaseModel( dbModel );
            if ( res == InitializeResult::Failed )
            {
                LOG_ERROR( "Failed to update database model" );
                return res;
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
    return Media::listAll( this, IMedia::Type::Audio, params );
}

Query<IMedia> MediaLibrary::videoFiles( const QueryParameters* params ) const
{
    return Media::listAll( this, IMedia::Type::Video, params );
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
    auto movie = Movie::create( this, media.id() );
    media.setMovie( movie );
    media.save();
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

void MediaLibrary::startParser()
{
    assert( m_parser == nullptr );
    auto parser = std::make_unique<parser::Parser>( this );

    if ( m_services.empty() == true )
    {
#ifdef HAVE_LIBVLC
        parser->addService( std::make_shared<parser::VLCMetadataService>() );
#else
        return;
#endif
    }
    else
    {
        assert( m_services[0]->targetedStep() == parser::Step::MetadataExtraction );
        parser->addService( m_services[0] );
    }
    parser->addService( std::make_shared<parser::MetadataAnalyzer>() );
    parser->addService( std::make_shared<parser::LinkService>() );
    m_fsHolder.registerCallback( parser.get() );
    parser->start();
    m_parser = std::move( parser );
}

void MediaLibrary::startDiscovererLocked()
{
    if ( m_discovererWorker != nullptr )
        return;
    auto discoverer = std::make_unique<FsDiscoverer>( this, m_fsHolder, m_callback );
    m_discovererWorker.reset( new DiscovererWorker( this, &m_fsHolder,
                                                    std::move( discoverer ) ) );
    m_fsHolder.registerCallback( m_discovererWorker.get() );
}

void MediaLibrary::startDiscoverer()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    startDiscovererLocked();
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
    m_fsHolder.addFsFactory( std::make_shared<fs::libvlc::FileSystemFactory>( this, "smb://" ) );
#endif
}

void MediaLibrary::onDbConnectionReady( sqlite::Connection* )
{
}

void MediaLibrary::addLocalFsFactory()
{
#ifdef HAVE_LIBVLC
    m_fsHolder.addFsFactory( std::make_shared<fs::libvlc::FileSystemFactory>( this, "file://" ) );
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
            if ( previousVersion < 3 ||
                 previousVersion > Settings::DbModelVersion ||
                 previousVersion == 4 )
            {
                if( recreateDatabase() == false )
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
                needRescan = true;
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
            if ( previousVersion == 14 )
            {
                migrateModel14to15();
                previousVersion = 15;
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
        if ( forceRescanLocked() == false )
        {
            LOG_WARN( "Failed to force a rescan" );
        }
    }

    // Safety check: ensure we didn't forget a migration along the way
    assert( previousVersion == Settings::DbModelVersion );
    assert( previousVersion == m_settings.dbModelVersion() );

    auto ctx = getConn()->acquireWriteContext();
    if ( checkDatabaseIntegrity() == false )
        return InitializeResult::DbCorrupted;

    return InitializeResult::Success;
}

bool MediaLibrary::recreateDatabase()
{
    deleteAllTables( m_dbConnection.get() );
    auto t = m_dbConnection->newTransaction();
    Settings::createTable( m_dbConnection.get() );
    if ( createAllTables() == false )
        return false;
    createAllTriggers();

    // We dropped the database, there is no setting to be read anymore
    if ( m_settings.load() == false )
        return false;
    t->commit();

    /*
     * We just delete all tables but this won't invoke the thumbnails deletion
     * hooks, so we need to delete the files ourselves
     */
    removeThumbnails();

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
    m_settings.setDbModelVersion( 5 );
    t->commit();
}

void MediaLibrary::migrateModel5to6()
{
    // We can't create a transaction here since it would make the weak context
    // creation fail.
    std::string req = "DELETE FROM " + Media::Table::Name + " WHERE type = ?";
    sqlite::Tools::executeRequest( getConn(), req, Media::Type::Unknown );

    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    req = "UPDATE " + Media::Table::Name + " SET is_present = 1 WHERE is_present != 0";
    sqlite::Tools::executeRequest( getConn(), req );
    m_settings.setDbModelVersion( 6 );
}

void MediaLibrary::migrateModel7to8()
{
    sqlite::Connection::WeakDbContext weakConnCtx{ getConn() };
    auto t = getConn()->newTransaction();
    std::string reqs[] = {
        parser::Task::schema( parser::Task::Table::Name, 8 ),
#       include "database/migrations/migration7-8.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( getConn(), req );
    // Re-create triggers removed in the process
    m_settings.setDbModelVersion( 8 );
    t->commit();
}

bool MediaLibrary::migrateModel8to9()
{
    // A bug in a previous migration caused our triggers to be missing for the
    // first application run (after the migration).
    // This could have caused media associated to deleted files not to be
    // deleted as well, so let's do that now.
    auto t = getConn()->newTransaction();
    const std::string req = "DELETE FROM " + Media::Table::Name + " "
            "WHERE id_media IN "
            "(SELECT id_media FROM " + Media::Table::Name + " m LEFT JOIN " +
                File::Table::Name + " f ON f.media_id = m.id_media "
                "WHERE f.media_id IS NULL)";

    if ( sqlite::Tools::executeDelete( getConn(), req ) == false )
        return false;
    m_settings.setDbModelVersion( 9 );
    t->commit();
    return false;
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
        LOG_DEBUG( "Converting ", f->rawMrl(), " to ", newMrl );
        f->setMrl( std::move( newMrl ) );
    }
    m_settings.setDbModelVersion( 10 );
    t->commit();
}

void MediaLibrary::migrateModel10to11()
{
    // Now empty because migration 15 to 16 is doing the same thing again
    // We updated the MRL encoding in 10 to 11, but again from 15 to 16, so we
    // might as well re encode everything only once.
    m_settings.setDbModelVersion( 11 );
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
bool MediaLibrary::migrateModel12to13()
{
    auto t = getConn()->newTransaction();
    const std::string reqs[] = {
        "DROP TRIGGER IF EXISTS is_track_presentAFTER",
        "DROP TRIGGER has_album_present",
        "DROP TRIGGER is_album_present",
        Album::trigger( Album::Triggers::IsPresent, 13 ),
        Artist::trigger( Artist::Triggers::HasTrackPresent, 13 ),
    };

    for ( const auto& req : reqs )
    {
        if ( sqlite::Tools::executeDelete( getConn(), req ) == false )
            return false;
    }

    // Leave the weak context as we now need to update is_present fields, which
    // are propagated through recursive triggers
    const std::string migrateData = "UPDATE " + AlbumTrack::Table::Name +
            " SET is_present = (SELECT is_present FROM " + Media::Table::Name +
            " WHERE id_media = media_id)";
    if ( sqlite::Tools::executeUpdate( getConn(), migrateData ) == false )
        return false;
    m_settings.setDbModelVersion( 13 );
    t->commit();
    return true;
}

/*
 * Model v13 to v14 migration:
 * - Media:
 *      - Remove
 *          .thumbnail
 *          .
 *      - Add
 *          .nb_playlists
 *          .real_last_played_date
 *          .device_id
 *          .folder_id
 *      - Fix filename being url encoded
 *      - Fix playlist external media type being Unknown
 *  - Playlist:
 *      - Add .file_id
 *  - PlaylistMediaRelation:
 *      - Add .mrl
 *  - Add a playlist FTS table
 *  - Add Thumbnail table
 *  - Task:
 *      - Add .is_refresh
 *  - Device:
 *      - Add last_seen
 *  - Folder:
 *      - Add:
 *          .nb_audio
 *          .nb_video
 *      - Remove .is_present
 *      - Rename is_blacklisted to is_banned
 *  _ Add a Folder FTS table
 *  - File:
 *      - Add .is_network
 *      - Remove .is_present
 *  - Add SubtitleTrack table
 *  - Removed History table
 *  - Removed on_track_genre_changed trigger
 *  - Removed is_folder_present trigger
 *  - Removed has_files_present trigger
 *  - Removed is_device_present trigger
 *  - Update is_album_present trigger
 *  - Update add_album_track trigger
 *  - Update delete_album_track trigger
 *  - New chapters table
 */
void MediaLibrary::migrateModel13to14( uint32_t originalPreviousVersion )
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();
    SubtitleTrack::createTable( dbConn );
    SubtitleTrack::createIndexes( dbConn );
    Chapter::createTable( dbConn );
    std::string reqs[] = {
#               include "database/migrations/migration13-14.sql"
        Thumbnail::schema( Thumbnail::Table::Name, 14 ),
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

            parser::Task::schema( parser::Task::Table::Name, 14 ),

            "INSERT INTO " + parser::Task::Table::Name + " SELECT "
            "id_task, step, retry_count, mrl, " +
            std::to_string( static_cast<std::underlying_type<IFile::Type>::type>(
                                IFile::Type::Main ) ) + ","
            "file_id, parent_folder_id, parent_playlist_id,"
            "parent_playlist_index, 0 FROM " + parser::Task::Table::Name + "_backup",

            "DROP TABLE " + parser::Task::Table::Name + "_backup",
        };
        for ( const auto& req : migrateTaskReqs )
            sqlite::Tools::executeRequest( dbConn, req );
    }
    // Re-create tables that we just removed
    // We will run a re-scan, so we don't care about keeping their content
    std::string recreateReqs[] = {
        Album::schema( Album::Table::Name, 14 ),
        Artist::schema( Artist::Table::Name, 14 ),
    };
    for ( const auto& req : recreateReqs )
        sqlite::Tools::executeRequest( dbConn, req );
    Show::createTable( dbConn );
    VideoTrack::createTable( dbConn );
    auto folders = Folder::fetchAll<Folder>( this );
    for ( const auto& f : folders )
    {
        f->setName( utils::file::directoryName( f->rawMrl() ) );
    }
    m_settings.setDbModelVersion( 14 );

    t->commit();
}

/**
 * Model 14 to 15 migration:
 * - Folder.name is now case insensitive
 */
void MediaLibrary::migrateModel14to15()
{
    auto dbConn = getConn();
    sqlite::Connection::WeakDbContext weakConnCtx{ dbConn };
    auto t = dbConn->newTransaction();
    std::string reqs[] = {
#               include "database/migrations/migration14-15.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( dbConn, req );
    m_settings.setDbModelVersion( 15 );
    t->commit();
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
                sqlite::Statement stmt{ dbConn->handle(), req };
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
            sqlite::Statement stmt{ dbConn->handle(), req };
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
            sqlite::Statement stmt{ dbConn->handle(), req };
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
    sqlite::Statement stmt{ dbConn->handle(), req };
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

void MediaLibrary::migrationEpilogue( uint32_t originalPreviousVersion )
{
    // Since we just changed the media model, the code that was used to migrate
    // model 13 to 14 isn't valid anymore (as is any code that relies on the C++
    // side of things instead of plain SQL requests). The filename would be loaded
    // in the previous member variable, and the migration wouldn't behave as expected.
    // We move it here, since this is only a problem when migrating from
    // 13 or before to 17. Should a user perform a migration from 13 to 17, they
    // wouldn't have the issue, as the new v17 model will not be implemented in
    // their version.
    if ( originalPreviousVersion <= 13 )
    {
        const std::string req = "SELECT * FROM " + Media::Table::Name +
                " WHERE filename LIKE '%#%%' ESCAPE '#'";
        auto media = Media::fetchAll<Media>( this, req );
        for ( const auto& m : media )
        {
            // We must not call mrl() from here. We might not have all devices yet,
            // and calling mrl would crash for files stored on removable devices.
            auto newFileName = utils::url::decode( m->fileName() );
            LOG_DEBUG( "Converting ", m->fileName(), " to ", newFileName );
            m->setFileName( std::move( newFileName ) );
        }
    }
}

void MediaLibrary::reload()
{
    startDiscoverer();
    m_discovererWorker->reload();
    /*
     * Initialize the parser and implicitely restart uncompleted parser tasks
     * If the parser was already started, this is a no-op
     */
    getParser();
}

void MediaLibrary::reload( const std::string& entryPoint )
{
    startDiscoverer();
    m_discovererWorker->reload( entryPoint );
    getParser();
}

bool MediaLibrary::forceParserRetry()
{
    return parser::Task::resetRetryCount( this );
}

bool MediaLibrary::clearDatabase( bool restorePlaylists )
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    pauseBackgroundOperationsLocked();
    auto parser = getParserLocked();
    if ( parser != nullptr )
        parser->flush();
    // If we don't care about playlists, take a shortcut.
    if ( restorePlaylists == false )
    {
        recreateDatabase();
        resumeBackgroundOperationsLocked();
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
            sqlite::Statement s{ m_dbConnection->handle(),
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

    recreateDatabase();

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
    resumeBackgroundOperationsLocked();
    return true;
}

void MediaLibrary::pauseBackgroundOperations()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    pauseBackgroundOperationsLocked();
}

void MediaLibrary::resumeBackgroundOperations()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    resumeBackgroundOperationsLocked();
}

void MediaLibrary::pauseBackgroundOperationsLocked()
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->pause();
    if ( m_parser != nullptr )
        m_parser->pause();
    std::lock_guard<compat::Mutex> thumbLock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker != nullptr )
        m_thumbnailerWorker->pause();
}

void MediaLibrary::resumeBackgroundOperationsLocked()
{
    if ( m_discovererWorker != nullptr )
        m_discovererWorker->resume();
    if ( m_parser != nullptr )
        m_parser->resume();
    std::lock_guard<compat::Mutex> thumbLock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker != nullptr )
        m_thumbnailerWorker->resume();
}

void MediaLibrary::onDiscovererIdleChanged( bool idle )
{
    bool expected = !idle;
    if ( m_discovererIdle.compare_exchange_strong( expected, idle ) == true )
    {
        // If any idle state changed to false, then we need to trigger the callback.
        // If switching to idle == true, then both background workers need to be idle before signaling.
        LOG_DEBUG( idle ? "Discoverer thread went idle" : "Discover thread was resumed" );
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
            LOG_DEBUG( "Setting background idle state to ",
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
        LOG_DEBUG( idle ? "All parser services went idle" : "Parse services were resumed" );
        if ( idle == false || m_discovererIdle == true )
        {
            if ( idle == true && m_modificationNotifier != nullptr )
            {
                // See comments above
                m_modificationNotifier->flush();
            }
            LOG_DEBUG( "Setting background idle state to ",
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

parser::Parser *MediaLibrary::tryGetParser()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    return m_parser.get();
}

parser::Parser *MediaLibrary::getParserLocked() const
{
    if ( m_parser == nullptr )
    {
        auto self = const_cast<MediaLibrary*>( this );
        self->startParser();
    }
    return m_parser.get();
}

parser::Parser* MediaLibrary::getParser() const
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    return getParserLocked();
}

ThumbnailerWorker* MediaLibrary::thumbnailer() const
{
    std::unique_lock<compat::Mutex> lock{ m_thumbnailerWorkerMutex };
    if ( m_thumbnailerWorker == nullptr )
        startThumbnailer();
    return m_thumbnailerWorker.get();
}

void MediaLibrary::registerDeviceLister( DeviceListerPtr lister,
                                         const std::string& scheme )
{
    assert( m_initialized == false );
    m_fsHolder.registerDeviceLister( scheme, std::move( lister ) );
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
    startDiscoverer();
    m_discovererWorker->discover( entryPoint );
}

bool MediaLibrary::addFileSystemFactory( std::shared_ptr<fs::IFileSystemFactory> fsFactory )
{
    return m_fsHolder.addFsFactory( std::move( fsFactory ) );
}

bool MediaLibrary::setDiscoverNetworkEnabled( bool enabled )
{
    return m_fsHolder.setNetworkEnabled( enabled );
}

bool MediaLibrary::isDiscoverNetworkEnabled() const
{
    return m_fsHolder.isNetworkEnabled();
}

Query<IFolder> MediaLibrary::entryPoints() const
{
    return Folder::entryPoints( this, false, 0 );
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
    startDiscoverer();
    m_discovererWorker->remove( entryPoint );
}

void MediaLibrary::banFolder( const std::string& entryPoint )
{
    startDiscoverer();
    m_discovererWorker->ban( entryPoint );
}

void MediaLibrary::unbanFolder( const std::string& entryPoint )
{
    startDiscoverer();
    m_discovererWorker->unban( entryPoint );
}

Query<IFolder> MediaLibrary::bannedEntryPoints() const
{
    return Folder::entryPoints( this, true, 0 );
}

const std::string& MediaLibrary::thumbnailPath() const
{
    return m_thumbnailPath;
}

const std::string& MediaLibrary::playlistPath() const
{
    return m_playlistPath;
}

void MediaLibrary::setLogger( ILogger* logger )
{
    Log::SetLogger( logger );
}

void MediaLibrary::startFsFactory( fs::IFileSystemFactory &fsFactory ) const
{
    m_fsHolder.startFsFactory( fsFactory );
}

bool MediaLibrary::forceRescan()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    return forceRescanLocked();
}

bool MediaLibrary::forceRescanLocked()
{
    /*
     * If the parser hasn't been initialized yet, don't force it to start until
     * we are done flushing entities from the database.
     * This saves us from starting it only to pause/resume it immediately after
     * Additionnaly, in the case of a migration from <24 to a later version, we
     * don't create the unknown show during the migration, which causes the
     * MetadataParser to refuse to initialize since it can't fetch & cache the
     * unknown show
     */
    if ( m_parser != nullptr )
        m_parser->prepareRescan();
    {
        auto t = getConn()->newTransaction();
        // Let the triggers clear out the Fts tables
        if ( AlbumTrack::deleteAll( this ) == false ||
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
             Media::resetSubTypes( this ) == false ||
             Thumbnail::deleteAll( this ) == false ||
             Thumbnail::removeAllCleanupRequests( this ) == false )
        {
                return false;
        }

        t->commit();
    }
    removeThumbnails();
    if ( m_parser != nullptr )
    {
        m_callback->onRescanStarted();
        m_parser->rescan();
    }
    else
    {
        /*
         * Implicitely create & start the parser, which will invoke restore()
         * as its first operation, which will start the reset tasks parsing.
         */
        getParserLocked();
    }
    return true;
}

void MediaLibrary::enableFailedThumbnailRegeneration()
{
    Thumbnail::deleteFailureRecords( this );
}

void MediaLibrary::addParserService( std::shared_ptr<parser::IParserService> service )
{
    // For now we only support 1 external service of type MetadataExtraction
    if ( service->targetedStep() != parser::Step::MetadataExtraction )
        return;
    if ( m_services.empty() == false )
        return;
    m_services.emplace_back( std::move( service ) );
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
    if ( VLCInstance::isSet() == false )
    {
        /*
         * If we never set the instance, we don't have to bother about background
         * tasks using the previous one: take a shortcut.
         */
        VLCInstance::set( inst );
        return true;
    }
    auto restartDiscoverer = false;
    {
        std::lock_guard<compat::Mutex> lock{ m_mutex };
        if ( m_discovererWorker != nullptr )
        {
            m_fsHolder.unregisterCallback( m_discovererWorker.get() );
            m_discovererWorker->stop();
            m_discovererWorker.reset();
            restartDiscoverer = true;
        }

        /*
         * The VLCMetadataService will fetch the new instance during its next run
         * The thumbnailer also fetches the instance before using it
         * However the discoverer
         * The discoverer and all FS related code is highly likely to use a
         * cached/queued instance that will hold the old libvlc instance which
         * we don't want, as the libvlc instance provided by an external user
         * is our only way of leveraging libvlc's keystore and accessing password
         * protected folders/shares
         * Now that it is stopped, we can replace the old libvlc instance while
         * holding the media library lock to avoid a concurrent call to a
         * discovery related function to reuse the old instance behind our backs.
         */
        VLCInstance::set( inst );
    }

    if ( restartDiscoverer == true )
        startDiscoverer();

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
