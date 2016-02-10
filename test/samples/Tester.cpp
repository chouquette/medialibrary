#include "Tester.h"

extern bool Verbose;
extern bool ExtraVerbose;

MockCallback::MockCallback()
{
    // Start locked. The locked will be released when waiting for parsing to be completed
    m_parsingMutex.lock();
}

bool MockCallback::waitForParsingComplete()
{
    std::unique_lock<std::mutex> lock( m_parsingMutex, std::adopt_lock );
    m_done = false;
    m_discoveryCompleted = false;
    // Wait for a while, generating snapshots can be heavy...
    return m_parsingCompleteVar.wait_for( lock, std::chrono::seconds( 30 ), [this]() {
        return m_done;
    });
}

void MockCallback::onDiscoveryCompleted(const std::string&)
{
    std::lock_guard<std::mutex> lock( m_parsingMutex );
    m_discoveryCompleted = true;
}

void MockCallback::onParsingStatsUpdated(uint32_t percent)
{
    if ( percent == 100 )
    {
        std::lock_guard<std::mutex> lock( m_parsingMutex );
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
    m_ml.reset( new MediaLibraryTester );
    if ( ExtraVerbose == true )
        m_ml->setVerbosity( LogLevel::Debug );
    else if ( Verbose == true )
        m_ml->setVerbosity( LogLevel::Info );

    m_ml->initialize( "test.db", "/tmp", m_cb.get() );
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
            ASSERT_EQ( !snapshotExpected, media->thumbnail().empty() );
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
                auto artists = a->artists();
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
                const auto tracks = a->tracks();
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
                auto albums = artist->albums();
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
                auto tracks = artist->media();
                if ( expectedArtist["nbTracks"].GetUint() != tracks.size() )
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
                 artist->id() != medialibrary::UnknownArtistID )
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
            if ( albumTrack->releaseYear() != expectedTrack["releaseYear"].GetUint() )
                return;
        }
        if ( expectedTrack.HasMember( "cd" ) )
        {
            if ( expectedTrack["cd"].GetUint() != albumTrack->discNumber() )
                return;
        }
        // Always check if the album link is correct. This isn't part of finding the proper album, so just fail hard
        // if the check fails.
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
    found = true;
}
