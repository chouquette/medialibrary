#include "Tester.h"

MockCallback::MockCallback()
{
    // Start locked. The locked will be released when waiting for parsing to be completed
    m_parsingMutex.lock();
}

bool MockCallback::waitForParsingComplete()
{
    std::unique_lock<std::mutex> lock( m_parsingMutex, std::adopt_lock );
    m_done = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds( 30 ), [this]() {
        return m_done;
    });
}

void MockCallback::onParsingStatsUpdated(uint32_t nbParsed, uint32_t nbToParse)
{
    if ( nbParsed == nbToParse && nbToParse > 0 )
    {
        std::lock_guard<std::mutex> lock( m_parsingMutex );
        m_done = true;
        m_parsingCompleteVar.notify_all();
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
    auto medias = m_ml->files();
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
            ASSERT_EQ( !snapshotExpected, media->snapshot().empty() );
        }
    }
}

void Tests::checkAlbums(const rapidjson::Value& expectedAlbums )
{
    ASSERT_TRUE( expectedAlbums.IsArray() );
    auto albums = m_ml->albums();
    ASSERT_EQ( expectedAlbums.Size(), albums.size() );
    for ( auto i = 0u; i < expectedAlbums.Size(); ++i )
    {
        const auto& expectedAlbum = expectedAlbums[i];
        ASSERT_TRUE( expectedAlbum.HasMember( "title" ) );
        // Start by checking if the album was found
        const auto title = expectedAlbum["title"].GetString();
        const char* artist = nullptr;
        if ( expectedAlbum.HasMember( "artist" ) )
            artist = expectedAlbum["artist"].GetString();
        auto it = std::find_if( begin( albums ), end( albums ), [title, artist](const AlbumPtr& a) {
            return strcasecmp( a->title().c_str(), title ) == 0 &&
                    ( artist == nullptr || strcasecmp( a->albumArtist()->name().c_str(), artist ) == 0 );
        });
        ASSERT_NE( end( albums ), it );
        auto album = *it;
        // Now check if we have matching metadata
        if ( artist != nullptr )
        {
            auto expectedArtist = expectedAlbum["artist"].GetString();
            auto artist = album->albumArtist();
            ASSERT_NE( nullptr, artist );
            ASSERT_STRCASEEQ( expectedArtist, artist->name().c_str() );
        }
        if ( expectedAlbum.HasMember( "artists" ) )
        {
            const auto& expectedArtists = expectedAlbum["artists"];
            auto artists = album->artists();
            ASSERT_EQ( expectedArtists.Size(), artists.size() );
            for ( auto i = 0u; i < expectedArtists.Size(); ++i )
            {
                auto expectedArtist = expectedArtists[i].GetString();
                auto it = std::find_if( begin( artists ), end( artists), [expectedArtist](const ArtistPtr& a) {
                    return strcasecmp( expectedArtist, a->name().c_str() ) == 0;
                });
                ASSERT_NE( end( artists ), it );
            }
        }
        if ( expectedAlbum.HasMember( "nbTracks" ) || expectedAlbum.HasMember( "tracks" ) )
        {
            const auto tracks = album->tracks();
            if ( expectedAlbum.HasMember( "nbTracks" ) )
            {
                ASSERT_EQ( expectedAlbum["nbTracks"].GetUint(), tracks.size() );
            }
            if ( expectedAlbum.HasMember( "tracks" ) )
            {
                checkAlbumTracks( album.get(), tracks, expectedAlbum["tracks"] );
            }
        }
        if ( expectedAlbum.HasMember( "releaseYear" ) )
        {
            ASSERT_EQ( expectedAlbum["releaseYear"].GetUint(), album->releaseYear() );
        }
    }
}

void Tests::checkAlbumTracks( const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks)
{
    // Don't mandate all tracks to be defined
    for ( auto i = 0u; i < expectedTracks.Size(); ++i )
    {
        auto& expectedTrack = expectedTracks[i];
        ASSERT_TRUE( expectedTrack.HasMember( "title" ) );
        auto expectedTitle = expectedTrack["title"].GetString();
        auto it = std::find_if( begin( tracks ), end( tracks ), [expectedTitle](const MediaPtr& media) {
            return strcasecmp( expectedTitle, media->title().c_str() ) == 0;
        });
        ASSERT_NE( end( tracks ), it );
        const auto track = *it;
        const auto albumTrack = track->albumTrack();
        if ( expectedTrack.HasMember( "number" ) )
        {
            ASSERT_EQ( expectedTrack["number"].GetUint(), albumTrack->trackNumber() );
        }
        if ( expectedTrack.HasMember( "artist" ) )
        {
            ASSERT_STRCASEEQ( expectedTrack["artist"].GetString(), track->artist().c_str() );
        }
        if ( expectedTrack.HasMember( "genre" ) )
        {
            ASSERT_STRCASEEQ( expectedTrack["genre"].GetString(), albumTrack->genre().c_str() );
        }
        // Always check if the album link is correct
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
}
