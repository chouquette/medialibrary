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
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds( 5 ), [this]() {
        return m_done;
    });
}

void MockCallback::onDiscoveryCompleted(const std::string& entryPoint )
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

void Tests::SetUp()
{
    unlink("test.db");
    m_cb.reset( new MockCallback );
    m_ml.reset( new MediaLibrary );
    if ( ExtraVerbose == true )
        m_ml->setVerbosity( LogLevel::Debug );
    else if ( Verbose == true )
        m_ml->setVerbosity( LogLevel::Info );

    auto res = m_ml->initialize( "test.db", "/tmp", m_cb.get() );
    ASSERT_EQ( InitializeResult::Success, res );
    ASSERT_TRUE( m_ml->start() );
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
            ASSERT_STRCASEEQ( expectedTrack["codec"].GetString(), track->codec().c_str() );
        if ( expectedTrack.HasMember( "width" ) )
            ASSERT_EQ( expectedTrack["width"].GetUint(), track->width() );
        if ( expectedTrack.HasMember( "height" ) )
            ASSERT_EQ( expectedTrack["height"].GetUint(), track->height() );
        if ( expectedTrack.HasMember( "fps" ) )
            ASSERT_EQ( expectedTrack["fps"].GetDouble(), track->fps() );
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
            ASSERT_STRCASEEQ( expectedTrack["codec"].GetString(), track->codec().c_str() );
        if ( expectedTrack.HasMember( "sampleRate" ) )
            ASSERT_EQ( expectedTrack["sampleRate"].GetUint(), track->sampleRate() );
        if ( expectedTrack.HasMember( "nbChannels" ) )
            ASSERT_EQ( expectedTrack["nbChannels"].GetUint(), track->nbChannels() );
        if ( expectedTrack.HasMember( "bitrate" ) )
            ASSERT_EQ( expectedTrack["bitrate"].GetUint(), track->bitrate() );
    }
}

void Tests::checkMedias(const rapidjson::Value& expectedMedias)
{
    ASSERT_TRUE( expectedMedias.IsArray() );
    auto medias = m_ml->audioFiles( SortingCriteria::Default, false );
    auto videos = m_ml->videoFiles( SortingCriteria::Default, false );
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
        const auto& media = *it;
        medias.erase( it );
        if ( expectedMedia.HasMember( "nbVideoTracks" ) || expectedMedia.HasMember( "videoTracks" ) )
        {
            auto videoTracks = media->videoTracks();
            if ( expectedMedia.HasMember( "nbVideoTracks" ) )
                ASSERT_EQ( expectedMedia[ "nbVideoTracks" ].GetUint(), videoTracks.size() );
            if ( expectedMedia.HasMember( "videoTracks" ) )
                checkVideoTracks( expectedMedia["videoTracks"], videoTracks );
        }
        if ( expectedMedia.HasMember( "nbAudioTracks" ) || expectedMedia.HasMember( "audioTracks" ) )
        {
            auto audioTracks = media->audioTracks();
            if ( expectedMedia.HasMember( "nbAudioTracks" ) )
                ASSERT_EQ( expectedMedia[ "nbAudioTracks" ].GetUint(), audioTracks.size() );
            if ( expectedMedia.HasMember( "audioTracks" ) )
                checkAudioTracks( expectedMedia[ "audioTracks" ], audioTracks );
        }
        if ( expectedMedia.HasMember( "snapshotExpected" ) == true )
        {
            auto snapshotExpected = expectedMedia["snapshotExpected"].GetBool();
            ASSERT_EQ( !snapshotExpected, media->thumbnail().empty() );
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

        const auto& playlist = *it;
        playlists.erase( it );
        const auto& items = playlist->media();

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
                auto artists = a->artists( false );
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
                const auto tracks = a->tracks( SortingCriteria::Default, false );
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
                if ( expectedAlbum["hasArtwork"].GetBool() == a->artworkMrl().empty()
                  || a->artworkMrl().compare(0, 13, "attachment://") == 0 )
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
            if ( expectedArtist.HasMember( "nbAlbums" ) || expectedArtist.HasMember( "albums" ) )
            {
                auto albums = artist->albums( SortingCriteria::Default, false );
                if ( expectedArtist.HasMember( "nbAlbums" ) )
                {
                    if ( albums.size() != expectedArtist["nbAlbums"].GetUint() )
                        return false;
                    if ( expectedArtist.HasMember( "albums" ) )
                        checkAlbums( expectedArtist["albums"], albums );
                }
            }
            if ( expectedArtist.HasMember( "nbTracks" ) )
            {
                auto expectedNbTracks = expectedArtist["nbTracks"].GetUint();
                auto tracks = artist->media( SortingCriteria::Default, false );
                if ( expectedNbTracks != tracks.size() )
                    return false;
                if ( expectedNbTracks != artist->nbTracks() )
                    return false;
            }
            if ( expectedArtist.HasMember( "hasArtwork" ) )
            {
                auto artwork = artist->artworkMrl();
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
            ASSERT_EQ( expectedTrack["hasArtwork"].GetBool(), track->thumbnail().empty() == false );
            ASSERT_TRUE( track->thumbnail().compare(0, 13, "attachment://") != 0 );
        }
        // Always check if the album link is correct. This isn't part of finding the proper album, so just fail hard
        // if the check fails.
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
    found = true;
}
