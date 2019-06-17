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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tester.h"

#include "parser/Parser.h"
#include "Thumbnail.h"
#include "File.h"
#include "utils/Filename.h"

#include <algorithm>

extern bool Verbose;
extern bool ExtraVerbose;

MockCallback::MockCallback()
{
    // Start locked. The locked will be released when waiting for parsing to be completed
    m_parsingMutex.lock();
}

bool MockCallback::waitForParsingComplete()
{
    std::unique_lock<compat::Mutex> lock( m_parsingMutex, std::adopt_lock );
    m_done = false;
    m_discoveryCompleted = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds( 10 ), [this]() {
        return m_done;
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
    if ( m_thumbnailCond.wait_for( lock, std::chrono::seconds{ 10 }, [this]() {
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

void MockCallback::onMediaThumbnailReady( MediaPtr media, bool success )
{
    std::unique_lock<compat::Mutex> lock( m_thumbnailMutex );

    if ( m_thumbnailTarget == nullptr || media->id() != m_thumbnailTarget->id() )
        return;
    m_thumbnailDone = true;
    m_thumbnailSuccess = success;
    m_thumbnailCond.notify_all();
}

MockResumeCallback::MockResumeCallback()
{
    m_discoveryMutex.lock();
}

void MockResumeCallback::onDiscoveryCompleted( const std::string& entryPoint, bool )
{
    if ( entryPoint.empty() == true )
        return;

    std::lock_guard<compat::Mutex> lock( m_discoveryMutex );
    m_discoveryCompleted = true;
    m_discoveryCompletedVar.notify_all();
}

void MockResumeCallback::reinit()
{
    m_parsingMutex.lock();
    m_discoveryCompleted = true;
    m_done = false;
}

bool MockResumeCallback::waitForDiscoveryComplete()
{
    std::unique_lock<compat::Mutex> lock( m_discoveryMutex, std::adopt_lock );
    m_discoveryCompleted = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_discoveryCompletedVar.wait_for( lock, std::chrono::seconds{ 10 }, [this]() {
        return m_discoveryCompleted;
    });
}

bool MockResumeCallback::waitForParsingComplete()
{
    // Reimplement without checking for discovery complete. This class is meant to be used
    // in 2 steps: waiting for discovery completed, then for parsing completed
    assert( m_discoveryCompleted == true );
    std::unique_lock<compat::Mutex> lock( m_parsingMutex, std::adopt_lock );
    m_done = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds{ 10 }, [this]() {
        return m_done;
    });
}

void Tests::SetUp()
{
    unlink("test.db");
    InitializeCallback();
    InitializeMediaLibrary();
    if ( ExtraVerbose == true )
        m_ml->setVerbosity( LogLevel::Debug );
    else if ( Verbose == true )
        m_ml->setVerbosity( LogLevel::Info );

#ifndef _WIN32
    auto thumbnailDir = "/tmp/ml_thumbnails/";
#else
    // This assumes wine for now
    auto thumbnailDir = "Z:\\tmp\\ml_thumbnails\\";
#endif

    auto res = m_ml->initialize( "test.db", thumbnailDir, m_cb.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    ASSERT_TRUE( m_ml->start() );
}

void Tests::InitializeCallback()
{
    m_cb.reset( new MockCallback );
}

void Tests::InitializeMediaLibrary()
{
    m_ml.reset( new MediaLibrary );
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
        checkArtists( expected["artists"], m_ml->artists( true, nullptr )->all() );
    }
    if ( expected.HasMember( "nbThumbnails" ) )
    {
        sqlite::Statement stmt{
            m_ml->getConn()->handle(),
            "SELECT COUNT(*) FROM " + Thumbnail::Table::Name
        };
        uint32_t nbThumbnails;
        stmt.execute();
        auto row = stmt.row();
        row >> nbThumbnails;
        ASSERT_EQ( expected["nbThumbnails"].GetUint(), nbThumbnails );
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
            ASSERT_STRCASEEQ( expectedTrack["codec"].GetString(), track->codec().c_str() );
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
            ASSERT_STRCASEEQ( expectedTrack["codec"].GetString(), track->codec().c_str() );
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
            ASSERT_STRCASEEQ( expectedTrack["codec"].GetString(), track->codec().c_str() );
        }
        if ( expectedTrack.HasMember( "encoding" ) )
        {
            ASSERT_STRCASEEQ( expectedTrack["encoding"].GetString(), track->encoding().c_str() );
        }
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
        ASSERT_NE( end( medias ), it );
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
        if ( expectedMedia.HasMember( "snapshotExpected" ) == true )
        {
            auto snapshotExpected = expectedMedia["snapshotExpected"].GetBool();
            if ( snapshotExpected && media->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() == true )
            {
                m_cb->prepareWaitForThumbnail( media );
                m_ml->requestThumbnail( media, ThumbnailSizeType::Thumbnail, 320, 200 );
                auto res = m_cb->waitForThumbnail();
                ASSERT_TRUE( res );
            }
            ASSERT_EQ( !snapshotExpected, media->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() );
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
        ASSERT_NE( end( playlists ), it );

        const auto playlist = *it;
        playlists.erase( it );
        const auto& items = playlist->media()->all();

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
            if ( expectedAlbum.HasMember( "hasArtwork" ) )
            {
                if ( expectedAlbum["hasArtwork"].GetBool() ==
                     a->thumbnailMrl( ThumbnailSizeType::Thumbnail ).empty() ||
                     a->thumbnailMrl( ThumbnailSizeType::Thumbnail )
                        .compare( 0, 13, "attachment://") == 0 )
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
        ASSERT_NE( end( albums ), it );
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
        ASSERT_NE( it, end( artists ) );
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

void ResumeTests::InitializeMediaLibrary()
{
    m_ml.reset( new MediaLibraryResumeTest );
}

void ResumeTests::InitializeCallback()
{
    m_cb.reset( new MockResumeCallback );
}

void ResumeTests::MediaLibraryResumeTest::forceParserStart()
{
    MediaLibrary::startParser();
}

bool ResumeTests::MediaLibraryResumeTest::startParser()
{
    return true;
}


void RefreshTests::forceRefresh()
{
    auto files = medialibrary::File::fetchAll( m_ml.get() );
    for ( const auto& f : files )
    {
        if ( f->isExternal() == true )
            continue;
        auto mrl = f->mrl();
        auto fsFactory = m_ml->fsFactoryForMrl( mrl );
        auto folderMrl = utils::file::directory( mrl );
        auto fileName = utils::file::fileName( mrl );
        auto folderFs = fsFactory->createDirectory( folderMrl );
        auto filesFs = folderFs->files();
        auto fileFsIt = std::find_if( cbegin( filesFs ), cend( filesFs ),
            [&fileName]( const std::shared_ptr<fs::IFile> f ) {
                return f->name() == fileName;
            });
        assert( fileFsIt != cend( filesFs ) );
        m_ml->onUpdatedFile( f, *fileFsIt );
    }
}

void RefreshTests::InitializeCallback()
{
    m_cb.reset( new MockResumeCallback );
}
