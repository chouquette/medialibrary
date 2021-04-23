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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tester.h"

#include "common/util.h"
#include "parser/Parser.h"
#include "Thumbnail.h"
#include "File.h"
#include "utils/Filename.h"
#include "factory/DeviceListerFactory.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/IShow.h"
#include "medialibrary/IShowEpisode.h"
#include "medialibrary/IMediaGroup.h"

#include <algorithm>

MockCallback::MockCallback()
    : m_thumbnailDone( false )
    , m_thumbnailSuccess( false )
    , m_done( false )
    , m_discoveryCompleted( false )
    , m_removalCompleted( false )
{

}

std::unique_lock<compat::Mutex> MockCallback::lock()
{
    return std::unique_lock<compat::Mutex>{ m_parsingMutex };
}

bool MockCallback::waitForParsingComplete( std::unique_lock<compat::Mutex>& lock )
{
    m_done = false;
    m_discoveryCompleted = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
        return m_done;
    });
}

bool MockCallback::waitForRemovalComplete( std::unique_lock<compat::Mutex>& lock )
{
    m_removalCompleted = false;
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
        return m_removalCompleted;
    });
}

void MockCallback::prepareWaitForThumbnail( MediaPtr media )
{
    m_thumbnailMutex.lock();
    m_thumbnailDone = false;
    m_thumbnailSuccess = false;
    m_thumbnailTarget = std::move( media );
}

bool MockCallback::waitForThumbnail()
{
    std::unique_lock<compat::Mutex> lock( m_thumbnailMutex, std::adopt_lock );
    if ( m_thumbnailCond.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
            return m_thumbnailDone;
        }) == false )
        return false;
    return m_thumbnailSuccess;
}

void MockCallback::onDiscoveryCompleted(const std::string& entryPoint, bool )
{
    if ( entryPoint.empty() == true )
        return;
    std::lock_guard<compat::Mutex> lock( m_parsingMutex );
    m_discoveryCompleted = true;
}

void MockCallback::onParsingStatsUpdated(uint32_t percent)
{
    if ( percent == 100 )
    {
        std::lock_guard<compat::Mutex> lock( m_parsingMutex );
        if ( m_discoveryCompleted == false )
            return;
        m_done = true;
        m_parsingCompleteVar.notify_all();
    }
}

void MockCallback::onMediaThumbnailReady( MediaPtr media, ThumbnailSizeType,
                                          bool success )
{
    std::unique_lock<compat::Mutex> lock( m_thumbnailMutex );

    if ( m_thumbnailTarget == nullptr || media->id() != m_thumbnailTarget->id() )
        return;
    m_thumbnailDone = true;
    m_thumbnailSuccess = success;
    m_thumbnailCond.notify_all();
}

void MockCallback::onEntryPointRemoved( const std::string& entryPoint, bool )
{
    assert( entryPoint.empty() == false );
    std::lock_guard<compat::Mutex> lock( m_parsingMutex );
    m_removalCompleted = true;
    m_parsingCompleteVar.notify_all();
}

MockResumeCallback::MockResumeCallback()
{
}

void MockResumeCallback::onDiscoveryCompleted( const std::string& entryPoint, bool )
{
    if ( entryPoint.empty() == true )
        return;

    std::lock_guard<compat::Mutex> lock( m_parsingMutex );
    m_discoveryCompleted = true;
    m_discoveryCompletedVar.notify_all();
}

void MockResumeCallback::reinit()
{
    m_discoveryCompleted = true;
    m_done = false;
}

bool MockResumeCallback::waitForDiscoveryComplete( std::unique_lock<compat::Mutex>& lock )
{
    assert( m_discoveryCompleted == false );
    // Wait for a while, generating snapshots can be heavy...
    return m_discoveryCompletedVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
        return m_discoveryCompleted;
    });
}

bool MockResumeCallback::waitForParsingComplete( std::unique_lock<compat::Mutex>& lock )
{
    // Reimplement without checking for discovery complete. This class is meant to be used
    // in 2 steps: waiting for discovery completed, then for parsing completed
    assert( m_discoveryCompleted == true );
    m_done = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
        return m_done;
    });
}

void Tests::SetUp()
{
    InitializeCallback();
    auto mlDir = getTempPath( "ml_folder" );
    InitializeMediaLibrary( "test.db", mlDir );
    m_ml->setVerbosity( LogLevel::Debug );

    auto res = m_ml->initialize( m_cb.get() );
    ASSERT_EQ( InitializeResult::Success, res );
}

void Tests::InitializeCallback()
{
    m_cb.reset( new MockCallback );
}

namespace
{
class MediaLibraryTester : public MediaLibrary
{
    using MediaLibrary::MediaLibrary;
    virtual void onDbConnectionReady( sqlite::Connection* dbConn ) override
    {
        deleteAllTables( dbConn );
    }
};
}

void Tests::InitializeMediaLibrary( const std::string& dbPath,
                                    const std::string& mlFolderDir )

{
    m_ml.reset( new MediaLibraryTester{ dbPath, mlFolderDir } );
}

void Tests::runChecks(const rapidjson::Document& doc)
{
    if ( doc.HasMember( "expected" ) == false )
    {
        // That's a lousy test case with no assumptions, but ok.
        return;
    }
    const auto& expected = doc["expected"];

    if ( expected.HasMember( "albums" ) == true )
    {
        checkAlbums( expected["albums" ], m_ml->albums( nullptr )->all() );
    }
    if ( expected.HasMember( "media" ) == true )
        checkMedias( expected["media"] );
    if ( expected.HasMember( "nbVideos" ) == true )
    {
        const auto videos = m_ml->videoFiles( nullptr )->all();
        ASSERT_EQ( expected["nbVideos"].GetUint(), videos.size() );
    }
    if ( expected.HasMember( "nbAudios" ) == true )
    {
        const auto audios = m_ml->audioFiles( nullptr )->all();
        ASSERT_EQ( expected["nbAudios"].GetUint(), audios.size() );
    }
    if ( expected.HasMember( "nbPlaylists" ) == true )
    {
        const auto playlists = m_ml->playlists( nullptr )->all();
        ASSERT_EQ( expected["nbPlaylists"].GetUint(), playlists.size() );
    }
    if ( expected.HasMember( "playlists" ) == true )
    {
      checkPlaylists( expected["playlists"], m_ml->playlists( nullptr )->all() );
    }
    if ( expected.HasMember( "artists" ) )
    {
        checkArtists( expected["artists"], m_ml->artists( ArtistIncluded::All, nullptr )->all() );
    }
    if ( expected.HasMember( "nbThumbnails" ) )
    {
        sqlite::Statement stmt{
            static_cast<MediaLibrary*>( m_ml.get() )->getConn()->handle(),
            "SELECT COUNT(*) FROM " + Thumbnail::Table::Name
        };
        uint32_t nbThumbnails;
        stmt.execute();
        auto row = stmt.row();
        row >> nbThumbnails;
        ASSERT_EQ( expected["nbThumbnails"].GetUint(), nbThumbnails );
    }
    if ( expected.HasMember( "shows" ) == true )
    {
        checkShows( expected["shows"], m_ml->shows( nullptr )->all() );
    }
    if ( expected.HasMember( "mediaGroups" ) == true )
    {
        checkMediaGroups( expected["mediaGroups"],
                m_ml->mediaGroups( IMedia::Type::Unknown, nullptr )->all() );
    }
}

void Tests::checkVideoTracks( const rapidjson::Value& expectedTracks, const std::vector<VideoTrackPtr>& tracks )
{
    // There is no reliable way of discriminating between tracks, so we just assume the test case will
    // only check for simple cases... like a single track?
    ASSERT_TRUE( expectedTracks.IsArray() );
    ASSERT_EQ( expectedTracks.Size(), tracks.size() );
    for ( auto i = 0u; i < expectedTracks.Size(); ++i )
    {
        const auto& track = tracks[i];
        const auto& expectedTrack = expectedTracks[i];
        ASSERT_TRUE( expectedTrack.IsObject() );
        if ( expectedTrack.HasMember( "codec" ) )
        {
            ASSERT_EQ( strcasecmp( expectedTrack["codec"].GetString(),
                                   track->codec().c_str() ), 0 );
        }
        if ( expectedTrack.HasMember( "width" ) )
        {
            ASSERT_EQ( expectedTrack["width"].GetUint(), track->width() );
        }
        if ( expectedTrack.HasMember( "height" ) )
        {
            ASSERT_EQ( expectedTrack["height"].GetUint(), track->height() );
        }
        if ( expectedTrack.HasMember( "fps" ) )
        {
            ASSERT_EQ( expectedTrack["fps"].GetDouble(), track->fps() );
        }
    }
}

void Tests::checkAudioTracks(const rapidjson::Value& expectedTracks, const std::vector<AudioTrackPtr>& tracks)
{
    ASSERT_TRUE( expectedTracks.IsArray() );
    ASSERT_EQ( expectedTracks.Size(), tracks.size() );
    for ( auto i = 0u; i < expectedTracks.Size(); ++i )
    {
        const auto& track = tracks[i];
        const auto& expectedTrack = expectedTracks[i];
        ASSERT_TRUE( expectedTrack.IsObject() );
        if ( expectedTrack.HasMember( "codec" ) )
        {
            ASSERT_EQ( strcasecmp( expectedTrack["codec"].GetString(),
                                   track->codec().c_str() ), 0 );
        }
        if ( expectedTrack.HasMember( "sampleRate" ) )
        {
            ASSERT_EQ( expectedTrack["sampleRate"].GetUint(), track->sampleRate() );
        }
        if ( expectedTrack.HasMember( "nbChannels" ) )
        {
            ASSERT_EQ( expectedTrack["nbChannels"].GetUint(), track->nbChannels() );
        }
        if ( expectedTrack.HasMember( "bitrate" ) )
        {
            ASSERT_EQ( expectedTrack["bitrate"].GetUint(), track->bitrate() );
        }
    }
}

void Tests::checkSubtitleTracks( const rapidjson::Value& expectedTracks,
                                 const std::vector<SubtitleTrackPtr>& tracks )
{
    ASSERT_TRUE( expectedTracks.IsArray() );
    ASSERT_EQ( expectedTracks.Size(), tracks.size() );
    for ( auto i = 0u; i < expectedTracks.Size(); ++i )
    {
        const auto& track = tracks[i];
        const auto& expectedTrack = expectedTracks[i];
        ASSERT_TRUE( expectedTrack.IsObject() );
        if ( expectedTrack.HasMember( "codec" ) )
        {
            ASSERT_EQ( strcasecmp( expectedTrack["codec"].GetString(),
                                   track->codec().c_str() ), 0 );
        }
        if ( expectedTrack.HasMember( "encoding" ) )
        {
            ASSERT_EQ( strcasecmp( expectedTrack["encoding"].GetString(),
                                   track->encoding().c_str() ), 0 );
        }
    }
}

void Tests::checkMediaFiles( const IMedia *media, const rapidjson::Value& expectedFiles )
{
    ASSERT_TRUE( expectedFiles.IsArray() );
    auto files = media->files();
    ASSERT_EQ( expectedFiles.Size(), files.size() );
    for ( auto i = 0u; i < expectedFiles.Size(); ++i )
    {
        const auto& expectedFile = expectedFiles[i];
        ASSERT_TRUE( expectedFile.HasMember( "filename" ) );
        auto it = std::find_if( begin( files ), end( files ), [&expectedFile]( FilePtr f ) {
            return utils::file::fileName( f->mrl() ) == expectedFile["filename"].GetString();
        });
        ASSERT_TRUE( it != end( files ) );

        if ( expectedFile.HasMember( "type" ) )
        {
            auto expectedType = expectedFile["type"].GetInt();
            ASSERT_EQ( expectedType,
                       static_cast<std::underlying_type_t<IFile::Type>>( (*it)->type() ) );
        }
        files.erase( it );
    }
}

void Tests::checkMedias(const rapidjson::Value& expectedMedias)
{
    ASSERT_TRUE( expectedMedias.IsArray() );
    auto medias = m_ml->audioFiles( nullptr )->all();
    auto videos = m_ml->videoFiles( nullptr )->all();
    medias.insert( begin( medias ), begin( videos ), end( videos ) );
    for ( auto i = 0u; i < expectedMedias.Size(); ++i )
    {
        const auto& expectedMedia = expectedMedias[i];
        ASSERT_TRUE( expectedMedia.HasMember( "title" ) );
        const auto expectedTitle = expectedMedia["title"].GetString();
        auto it = std::find_if( begin( medias ), end( medias ), [expectedTitle](const MediaPtr& m) {
            return strcasecmp( expectedTitle, m->title().c_str() ) == 0;
        });
        ASSERT_TRUE( end( medias ) != it );
        const auto media = *it;
        medias.erase( it );
        if ( expectedMedia.HasMember( "nbVideoTracks" ) || expectedMedia.HasMember( "videoTracks" ) )
        {
            auto videoTracks = media->videoTracks()->all();
            if ( expectedMedia.HasMember( "nbVideoTracks" ) )
            {
                ASSERT_EQ( expectedMedia[ "nbVideoTracks" ].GetUint(), videoTracks.size() );
            }
            if ( expectedMedia.HasMember( "videoTracks" ) )
            {
                checkVideoTracks( expectedMedia["videoTracks"], videoTracks );
            }
        }
        if ( expectedMedia.HasMember( "nbAudioTracks" ) || expectedMedia.HasMember( "audioTracks" ) )
        {
            auto audioTracks = media->audioTracks()->all();
            if ( expectedMedia.HasMember( "nbAudioTracks" ) )
            {
                ASSERT_EQ( expectedMedia[ "nbAudioTracks" ].GetUint(), audioTracks.size() );
            }
            if ( expectedMedia.HasMember( "audioTracks" ) )
                checkAudioTracks( expectedMedia[ "audioTracks" ], audioTracks );
        }
        if ( expectedMedia.HasMember( "nbSubtitleTracks" ) || expectedMedia.HasMember( "subtitleTracks" ) )
        {
            auto subtitleTracks = media->subtitleTracks()->all();
            if ( expectedMedia.HasMember( "nbSubtitleTracks" ) )
            {
                ASSERT_EQ( expectedMedia[ "nbSubtitleTracks" ].GetUint(), subtitleTracks.size() );
            }
            if ( expectedMedia.HasMember( "subtitleTracks" ) )
                checkSubtitleTracks( expectedMedia[ "subtitleTracks" ], subtitleTracks );
        }
        if ( expectedMedia.HasMember( "snapshotExpected" ) == true )
        {
            auto snapshotExpected = expectedMedia["snapshotExpected"].GetBool();
            if ( snapshotExpected && media->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() == true )
            {
                m_cb->prepareWaitForThumbnail( media );
                media->requestThumbnail( ThumbnailSizeType::Thumbnail, 320, 200, .3f );
                auto res = m_cb->waitForThumbnail();
                ASSERT_TRUE( res );
            }
            ASSERT_EQ( !snapshotExpected, media->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() );
        }
        if ( expectedMedia.HasMember( "files" ) )
        {
            checkMediaFiles( media.get(), expectedMedia["files"] );
        }
    }
}

void Tests::checkPlaylists( const rapidjson::Value& expectedPlaylists, std::vector<PlaylistPtr> playlists )
{
    ASSERT_TRUE( expectedPlaylists.IsArray() );
    for ( auto i = 0u; i < expectedPlaylists.Size(); ++i )
    {
        const auto& expectedPlaylist = expectedPlaylists[i];
        ASSERT_TRUE( expectedPlaylist.HasMember( "name" ) );
        const auto expectedName = expectedPlaylist["name"].GetString();
        auto it = std::find_if( begin( playlists ), end( playlists ), [expectedName](const PlaylistPtr& p) {
            return strcasecmp( expectedName, p->name().c_str() ) == 0;
        });
        ASSERT_TRUE( end( playlists ) != it );

        const auto playlist = *it;
        playlists.erase( it );
        const auto& items = playlist->media( nullptr )->all();

        ASSERT_TRUE( playlist->isReadOnly() );
        ASSERT_FALSE( playlist->mrl().empty() );

        if ( expectedPlaylist.HasMember( "nbItems" ) )
        {
            ASSERT_EQ( expectedPlaylist["nbItems"].GetUint(), items.size() );
        }

        if ( expectedPlaylist.HasMember( "items" ) )
        {
            ASSERT_TRUE( expectedPlaylist["items"].IsArray() );
            ASSERT_EQ( items.size(), expectedPlaylist["items"].Size() );
            for ( auto i = 0u; i < items.size(); ++i )
            {
                if ( expectedPlaylist["items"][i].HasMember( "index" ) )
                {
                    ASSERT_EQ( expectedPlaylist["items"][i]["index"].GetUint(), i );
                }
                if ( expectedPlaylist["items"][i].HasMember( "title" ) )
                {
                    ASSERT_EQ( expectedPlaylist["items"][i]["title"].GetString(), items[i]->title() );
                }
                if ( expectedPlaylist["items"][i].HasMember( "mrl" ) )
                {
                    auto files = items[i]->files();
                    auto mainFileIt = std::find_if( cbegin( files ), cend( files ),
                        [](const FilePtr& f ) {
                            return f->isMain() == true;
                    });
                    ASSERT_TRUE( mainFileIt != cend( files ) );
                    ASSERT_EQ( expectedPlaylist["items"][i]["mrl"].GetString(),
                            (*mainFileIt)->mrl() );
                }
            }
        }
    }
}

void Tests::checkAlbums( const rapidjson::Value& expectedAlbums, std::vector<AlbumPtr> albums )
{
    ASSERT_TRUE( expectedAlbums.IsArray() );
    ASSERT_EQ( expectedAlbums.Size(), albums.size() );
    for ( auto i = 0u; i < expectedAlbums.Size(); ++i )
    {
        const auto& expectedAlbum = expectedAlbums[i];
        ASSERT_TRUE( expectedAlbum.HasMember( "title" ) );
        // Start by checking if the album was found
        auto it = std::find_if( begin( albums ), end( albums ), [this, &expectedAlbum](const AlbumPtr& a) {
            const auto expectedTitle = expectedAlbum["title"].GetString();
            if ( strcasecmp( a->title().c_str(), expectedTitle ) != 0 )
                return false;
            if ( expectedAlbum.HasMember( "artist" ) )
            {
                const auto expectedArtist = expectedAlbum["artist"].GetString();
                auto artist = a->albumArtist();
                if ( artist != nullptr && strcasecmp( artist->name().c_str(), expectedArtist ) != 0 )
                    return false;
            }
            if ( expectedAlbum.HasMember( "artists" ) )
            {
                const auto& expectedArtists = expectedAlbum["artists"];
                auto artists = a->artists( nullptr )->all();
                if ( expectedArtists.Size() != artists.size() )
                    return false;
                for ( auto i = 0u; i < expectedArtists.Size(); ++i )
                {
                    auto expectedArtist = expectedArtists[i].GetString();
                    auto it = std::find_if( begin( artists ), end( artists), [expectedArtist](const ArtistPtr& a) {
                        return strcasecmp( expectedArtist, a->name().c_str() ) == 0;
                    });
                    if ( it == end( artists ) )
                        return false;
                }
            }
            if ( expectedAlbum.HasMember( "hasArtwork" ) )
            {
                if ( expectedAlbum["hasArtwork"].GetBool() ==
                     a->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() ||
                     a->thumbnailMrl( ThumbnailSizeType::Thumbnail )
                        .compare( 0, 13, "attachment://") == 0 )
                    return false;
            }
            if ( expectedAlbum.HasMember( "nbTracks" ) || expectedAlbum.HasMember( "tracks" ) )
            {
                const auto tracks = a->tracks( nullptr )->all();
                if ( expectedAlbum.HasMember( "nbTracks" ) )
                {
                    if ( expectedAlbum["nbTracks"].GetUint() != tracks.size() )
                        return false;
                }
                if ( expectedAlbum.HasMember( "tracks" ) )
                {
                    bool tracksOk = false;
                    checkAlbumTracks( a.get(), tracks, expectedAlbum["tracks"], tracksOk );
                    if ( tracksOk == false )
                        return false;
                }
            }
            if ( expectedAlbum.HasMember( "releaseYear" ) )
            {
                const auto releaseYear = expectedAlbum["releaseYear"].GetUint();
                if ( a->releaseYear() != releaseYear )
                    return false;
            }
            if ( expectedAlbum.HasMember( "nbDiscs" ) )
            {
                const auto nbDiscs = expectedAlbum["nbDiscs"].GetUint();
                if ( a->nbDiscs() != nbDiscs )
                    return false;
            }
            return true;
        });
        ASSERT_TRUE( end( albums ) != it );
        albums.erase( it );
    }
}

void Tests::checkArtists(const rapidjson::Value& expectedArtists, std::vector<ArtistPtr> artists)
{
    ASSERT_TRUE( expectedArtists.IsArray() );
    ASSERT_EQ( expectedArtists.Size(), artists.size() );
    for ( auto i = 0u; i < expectedArtists.Size(); ++i )
    {
        const auto& expectedArtist = expectedArtists[i];
        auto it = std::find_if( begin( artists ), end( artists ), [&expectedArtist, this](const ArtistPtr& artist) {
            if ( expectedArtist.HasMember( "name" ) )
            {
                if ( strcasecmp( expectedArtist["name"].GetString(), artist->name().c_str() ) != 0 )
                    return false;
            }
            if ( expectedArtist.HasMember( "id" ) )
            {
                if ( expectedArtist["id"].GetUint() != artist->id() )
                    return false;
            }
            if ( expectedArtist.HasMember( "nbAlbums" ) )
            {
                if ( artist->nbAlbums() != expectedArtist["nbAlbums"].GetUint() )
                    return false;
            }
            if ( expectedArtist.HasMember( "albums" ) )
            {
                auto albums = artist->albums( nullptr )->all();
                checkAlbums( expectedArtist["albums"], albums );
            }
            if ( expectedArtist.HasMember( "nbTracks" ) )
            {
                auto expectedNbTracks = expectedArtist["nbTracks"].GetUint();
                auto tracks = artist->tracks( nullptr )->all();
                if ( expectedNbTracks != tracks.size() )
                    return false;
                if ( expectedNbTracks != artist->nbTracks() )
                    return false;
            }
            if ( expectedArtist.HasMember( "hasArtwork" ) )
            {
                auto artwork = artist->thumbnailMrl( ThumbnailSizeType::Thumbnail );
                if ( artwork.empty() == expectedArtist["hasArtwork"].GetBool() ||
                     artwork.compare( 0, 13, "attachment://" ) == 0 )
                    return false;
            }
            return true;
        });
        ASSERT_TRUE( it != end( artists ) );
    }
}

void Tests::checkAlbumTracks( const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks, bool& found ) const
{
    found = false;
    // Don't mandate all tracks to be defined
    for ( auto i = 0u; i < expectedTracks.Size(); ++i )
    {
        const auto& expectedTrack = expectedTracks[i];
        ASSERT_TRUE( expectedTrack.HasMember( "title" ) );
        auto expectedTitle = expectedTrack["title"].GetString();
        auto it = std::find_if( begin( tracks ), end( tracks ), [expectedTitle](const MediaPtr& media) {
            return strcasecmp( expectedTitle, media->title().c_str() ) == 0;
        });
        if ( it == end( tracks ) )
            return ;
        const auto track = *it;
        const auto albumTrack = track->albumTrack();
        ASSERT_NE( nullptr, albumTrack );
        if ( expectedTrack.HasMember( "number" ) )
        {
            if ( expectedTrack["number"].GetUint() != albumTrack->trackNumber() )
                return ;
        }
        if ( expectedTrack.HasMember( "artist" ) )
        {
            auto artist = albumTrack->artist();
            if ( artist == nullptr )
                return ;
            if ( strlen( expectedTrack["artist"].GetString() ) == 0 &&
                 artist->id() != UnknownArtistID )
                return ;
            else if ( strcasecmp( expectedTrack["artist"].GetString(), artist->name().c_str() ) != 0 )
                return ;
        }
        if ( expectedTrack.HasMember( "genre" ) )
        {
            auto genre = albumTrack->genre();
            if ( genre == nullptr || strcasecmp( expectedTrack["genre"].GetString(), genre->name().c_str() ) != 0 )
                return ;
        }
        if ( expectedTrack.HasMember( "releaseYear" ) )
        {
            if ( track->releaseDate() != expectedTrack["releaseYear"].GetUint() )
                return;
        }
        if ( expectedTrack.HasMember( "cd" ) )
        {
            if ( expectedTrack["cd"].GetUint() != albumTrack->discNumber() )
                return;
        }
        if ( expectedTrack.HasMember( "hasArtwork" ) )
        {
            ASSERT_EQ( expectedTrack["hasArtwork"].GetBool(),
                       track->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() == false );
            ASSERT_TRUE( track->thumbnailMrl( ThumbnailSizeType::Thumbnail )
                                    .compare(0, 13, "attachment://") != 0 );
        }
        // Always check if the album link is correct. This isn't part of finding the proper album, so just fail hard
        // if the check fails.
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
    found = true;
}

void Tests::checkShows(const rapidjson::Value& expectedShows, std::vector<ShowPtr> shows)
{
    for ( auto i = 0u; i < expectedShows.Size(); ++i )
    {
        auto& expectedShow = expectedShows[i];
        ASSERT_TRUE( expectedShow.HasMember( "name" ) );
        auto showName = expectedShow["name"].GetString();
        auto showIt = std::find_if( cbegin( shows ), cend( shows ),
                                  [showName]( const ShowPtr& s ) {
            if ( strlen( showName ) == 0 )
                return s->id() == UnknownShowID;
            return s->title() == showName;
        });
        ASSERT_TRUE( showIt != cend( shows ) );
        auto show = *showIt;
        if ( expectedShow.HasMember( "nbEpisodes" ) == true )
        {
            ASSERT_EQ( expectedShow["nbEpisodes"].GetUint(), show->nbEpisodes() );
        }
        if ( expectedShow.HasMember( "episodes" ) == true )
        {
            auto episodes = show->episodes( nullptr )->all();
            ASSERT_FALSE( episodes.empty() );
            checkShowEpisodes( expectedShow["episodes"], std::move( episodes ) );
        }
    }
}

void Tests::checkShowEpisodes( const rapidjson::Value& expectedEpisodes,
                               std::vector<MediaPtr> episodes )
{
    for ( auto i = 0u; i < expectedEpisodes.Size(); ++i )
    {
        auto& expectedEpisode = expectedEpisodes[i];
        ASSERT_TRUE( expectedEpisode.HasMember( "seasonId" ) );
        ASSERT_TRUE( expectedEpisode.HasMember( "episodeId" ) );
        auto seasonId = expectedEpisode["seasonId"].GetUint();
        auto episodeId = expectedEpisode["episodeId"].GetUint();
        auto episodeIt = std::find_if( cbegin( episodes ), cend( episodes ),
                                       [seasonId, episodeId](const MediaPtr m) {
            auto showEp = m->showEpisode();
            if ( showEp == nullptr )
                return false;
            return showEp->seasonId() == seasonId && showEp->episodeId() == episodeId;
        });
        ASSERT_TRUE( episodeIt != cend( episodes ) );
        auto media = *episodeIt;
        auto showEp = media->showEpisode();
        if ( expectedEpisode.HasMember( "title" ) == true )
        {
            ASSERT_EQ( expectedEpisode["title"].GetString(), showEp->title() );
        }
        if ( expectedEpisode.HasMember( "mediaTitle" ) == true )
        {
            ASSERT_EQ( expectedEpisode["mediaTitle"].GetString(), media->title() );
        }
    }
}

void Tests::checkMediaGroups( const rapidjson::Value &expectedMediaGroups,
                              std::vector<MediaGroupPtr> mediaGroups)
{
    ASSERT_EQ( expectedMediaGroups.Size(), mediaGroups.size() );
    for ( auto i = 0u; i < expectedMediaGroups.Size(); ++i )
    {
        auto& expectedGroup = expectedMediaGroups[i];
        ASSERT_TRUE( expectedGroup.HasMember( "name" ) );
        auto it = std::find_if( cbegin( mediaGroups ), cend( mediaGroups ),
                                [&expectedGroup]( const MediaGroupPtr grp ) {
            return strcasecmp( grp->name().c_str(), expectedGroup["name"].GetString() ) == 0;
        });
        ASSERT_TRUE( cend( mediaGroups ) != it );
        auto group = *it;
        if ( expectedGroup.HasMember( "nbAudio" ) )
        {
            ASSERT_EQ( expectedGroup["nbAudio"].GetUint(), group->nbPresentAudio() );
        }
        if ( expectedGroup.HasMember( "nbVideo" ) )
        {
            ASSERT_EQ( expectedGroup["nbVideo"].GetUint(), group->nbPresentVideo() );
        }
        if ( expectedGroup.HasMember( "nbUnknown" ) )
        {
            ASSERT_EQ( expectedGroup["nbUnknown"].GetUint(), group->nbPresentUnknown() );
        }
    }
}

void ResumeTests::InitializeMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir )
{
    m_ml.reset( new MediaLibraryResumeTest( dbPath, mlFolderDir ) );
}

void ResumeTests::InitializeCallback()
{
    m_cb.reset( new MockResumeCallback );
}

void MediaLibraryResumeTest::forceParserStart()
{
    std::unique_lock<compat::Mutex> lock{ m_mutex };
    MediaLibrary::startParser();
}

void MediaLibraryResumeTest::onDbConnectionReady( sqlite::Connection *dbConn )
{
    deleteAllTables( dbConn );
}

void MediaLibraryResumeTest::startParser()
{
}

void RefreshTests::forceRefresh()
{
    auto ml = static_cast<MediaLibrary*>( m_ml.get() );
    auto files = medialibrary::File::fetchAll( ml );
    for ( const auto& f : files )
    {
        if ( f->isExternal() == true )
            continue;
        auto mrl = f->mrl();
        auto folder = Folder::fetch( ml, f->folderId() );
        auto fsFactory = ml->fsFactoryForMrl( mrl );
        auto folderMrl = utils::file::directory( mrl );
        auto fileName = utils::file::fileName( mrl );
        auto folderFs = fsFactory->createDirectory( folderMrl );
        auto filesFs = folderFs->files();
        auto fileFsIt = std::find_if( cbegin( filesFs ), cend( filesFs ),
            [&fileName]( const std::shared_ptr<fs::IFile> f ) {
                return f->name() == fileName;
            });
        assert( fileFsIt != cend( filesFs ) );
        ml->onUpdatedFile( f, *fileFsIt, std::move( folder ), std::move( folderFs ) );
    }
}

void RefreshTests::InitializeCallback()
{
    m_cb.reset( new MockResumeCallback );
}

void MockCallback::prepareForPlaylistReload()
{
    // We need to force the discover to appear as complete, as we won't do any
    // discovery for this test. Otherwise, we'd receive the parsing completed
    // event and just ignore it.
    m_discoveryCompleted = true;
}

bool MockCallback::waitForPlaylistReload( std::unique_lock<compat::Mutex>& lock )
{
    m_done = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 20 }, [this]() {
        return m_done;
    });
}
