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
        auto it = std::find_if( begin( albums ), end( albums ), [title](const AlbumPtr& a) {
            return strcasecmp( a->title().c_str(), title ) == 0;
        });
        ASSERT_NE( end( albums ), it );
        auto album = *it;
        // Now check if we have matching metadata
        if ( expectedAlbum.HasMember( "artist" ) )
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
        // Always check if the album link is correct
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
}
