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
    File::clear();
    Playlist::clear();
    History::clear();
    Genre::clear();
}

bool MediaLibrary::createAllTables()
{
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

bool MediaLibrary::validateSearchPattern( const std::string& pattern )
{
    return pattern.size() >= 3;
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
    auto mptr = Media::create( m_dbConnection.get(), type, fileFs );
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
        Media::destroy( m_dbConnection.get(), mptr->id() );
        return nullptr;
    }
    if ( m_callback != nullptr )
        m_callback->onMediaAdded( mptr );
    if ( m_parser != nullptr )
        m_parser->parse( mptr, file );
    return mptr;
}

bool MediaLibrary::deleteFolder( const Folder& folder )
{
    if ( Folder::destroy( m_dbConnection.get(), folder.id() ) == false )
        return false;
    Media::clear();
    return true;
}

std::shared_ptr<Device> MediaLibrary::device( const std::string& uuid )
{
    return Device::fromUuid( m_dbConnection.get(), uuid );
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

std::vector<GenrePtr> MediaLibrary::genres() const
{
    return Genre::fetchAll<IGenre>( m_dbConnection.get() );
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

std::shared_ptr<Movie> MediaLibrary::createMovie( Media& media, const std::string& title )
{
    auto movie = Movie::create( m_dbConnection.get(), media.id(), title );
    media.setMovie( movie );
    media.save();
    return movie;
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
    try
    {
        return Artist::create( m_dbConnection.get(), name );
    }
    catch( sqlite::errors::ConstraintViolation &ex )
    {
        LOG_WARN( "ContraintViolation while creating an artist (", ex.what(), ") attempting to fetch it instead" );
        return std::static_pointer_cast<Artist>( artist( name ) );
    }
}

std::vector<ArtistPtr> MediaLibrary::artists() const
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name +
            " WHERE nb_albums > 0 AND is_present = 1";
    return Artist::fetchAll<IArtist>( m_dbConnection.get(), req );
}

PlaylistPtr MediaLibrary::createPlaylist( const std::string& name )
{
    return Playlist::create( m_dbConnection.get(), name );
}

std::vector<PlaylistPtr> MediaLibrary::playlists()
{
    return Playlist::fetchAll<IPlaylist>( m_dbConnection.get() );
}

bool MediaLibrary::deletePlaylist( unsigned int playlistId )
{
    return Playlist::destroy( m_dbConnection.get(), playlistId );
}

bool MediaLibrary::addToHistory( const std::string& mrl )
{
    return History::insert( m_dbConnection.get(), mrl );
}

std::vector<HistoryPtr> MediaLibrary::lastStreamsPlayed() const
{
    return History::fetch( m_dbConnection.get() );
}

std::vector<MediaPtr> MediaLibrary::lastMediaPlayed() const
{
    return Media::fetchHistory( m_dbConnection.get() );
}

medialibrary::MediaSearchAggregate MediaLibrary::searchMedia( const std::string& title ) const
{
    if ( validateSearchPattern( title ) == false )
        return {};
    auto tmp = Media::search( m_dbConnection.get(), title );
    medialibrary::MediaSearchAggregate res;
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
    return Playlist::search( m_dbConnection.get(), name );
}

std::vector<AlbumPtr> MediaLibrary::searchAlbums( const std::string& pattern ) const
{
    if ( validateSearchPattern( pattern ) == false )
        return {};
    return Album::search( m_dbConnection.get(), pattern );
}

std::vector<GenrePtr> MediaLibrary::searchGenre( const std::string& genre ) const
{
    if ( validateSearchPattern( genre ) == false )
        return {};
    return Genre::search( m_dbConnection.get(), genre );
}

std::vector<ArtistPtr> MediaLibrary::searchArtists(const std::string& name ) const
{
    if ( validateSearchPattern( name ) == false )
        return {};
    return Artist::search( m_dbConnection.get(), name );
}

medialibrary::SearchAggregate MediaLibrary::search( const std::string& pattern ) const
{
    medialibrary::SearchAggregate res;
    res.albums = searchAlbums( pattern );
    res.artists = searchArtists( pattern );
    res.genres = searchGenre( pattern );
    res.media = searchMedia( pattern );
    res.playlists = searchPlaylists( pattern );
    return res;
}

void MediaLibrary::startParser()
{
    m_parser.reset( new Parser( m_dbConnection.get(), this, m_callback ) );

    auto vlcService = std::unique_ptr<VLCMetadataService>( new VLCMetadataService );
    auto metadataService = std::unique_ptr<MetadataParser>( new MetadataParser( m_dbConnection.get() ) );
    auto thumbnailerService = std::unique_ptr<VLCThumbnailer>( new VLCThumbnailer );
    m_parser->addService( std::move( vlcService ) );
    m_parser->addService( std::move( metadataService ) );
    m_parser->addService( std::move( thumbnailerService ) );
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

void MediaLibrary::discover( const std::string &entryPoint )
{
    if ( m_discoverer != nullptr )
        m_discoverer->discover( entryPoint );
}

bool MediaLibrary::banFolder( const std::string& path )
{
    return Folder::blacklist( m_dbConnection.get(), path );
}

bool MediaLibrary::unbanFolder( const std::string& path )
{
    auto folder = Folder::blacklistedFolder( m_dbConnection.get(), path );
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

