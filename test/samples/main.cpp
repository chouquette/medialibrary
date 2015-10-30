#include <condition_variable>
#include "gtest/gtest.h"
#include <memory>
#include <mutex>
#include <rapidjson/document.h>

#include "MediaLibrary.h"
#include "IAlbum.h"
#include "IArtist.h"
#include "IMedia.h"
#include "IAlbumTrack.h"

static const char* testCases[] = {
    "simple",

};

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

class MockCallback : public IMediaLibraryCb
{
public:
    MockCallback();
    bool waitForParsingComplete();

private:
    void onFileAdded(MediaPtr) {}
    void onFileUpdated(MediaPtr) {}
    void onDiscoveryStarted(const std::string&) {}
    void onDiscoveryCompleted(const std::string&) {}
    void onReloadStarted() {}
    void onReloadCompleted() {}
    void onParsingStatsUpdated(uint32_t nbParsed, uint32_t nbToParse);

    std::condition_variable m_parsingCompleteVar;
    std::mutex m_parsingMutex;
    bool m_done;
};

class Tests : public ::testing::TestWithParam<const char*>
{
protected:
    std::unique_ptr<MediaLibrary> m_ml;
    std::unique_ptr<MockCallback> m_cb;

    virtual void SetUp() override
    {
        m_cb.reset( new MockCallback );
        m_ml.reset( new MediaLibrary );
        m_ml->setVerbosity( LogLevel::Error );
        m_ml->initialize( "test.db", "/tmp", m_cb.get() );
    }

    virtual void TearDown() override
    {
        m_ml.reset();
        m_cb.reset();
    }

    void checkAlbums( const rapidjson::Value& expectedAlbums);
    void checkTracks( const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks );
};

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
        if ( expectedAlbum.HasMember( "nbTracks" ) || expectedAlbum.HasMember( "tracks" ) )
        {
            const auto tracks = album->tracks();
            if ( expectedAlbum.HasMember( "nbTracks" ) )
            {
                ASSERT_EQ( expectedAlbum["nbTracks"].GetUint(), tracks.size() );
            }
            if ( expectedAlbum.HasMember( "tracks" ) )
            {
                checkTracks( album.get(), tracks, expectedAlbum["tracks"] );
            }
        }
    }
}

void Tests::checkTracks( const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks)
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
        const auto trackAlbum = albumTrack->album();
        ASSERT_NE( nullptr, trackAlbum );
        ASSERT_EQ( album->id(), trackAlbum->id() );
    }
}


TEST_P( Tests, Parse )
{
    auto casePath = std::string("testcases/") + GetParam() + ".json";
    std::unique_ptr<FILE, int(*)(FILE*)> f( fopen( casePath.c_str(), "rb" ), &fclose );
    ASSERT_NE( nullptr, f );
    char buff[65536]; // That's how ugly I am!
    auto ret = fread( buff, sizeof(buff[0]), sizeof(buff), f.get() );
    ASSERT_NE( 0u, ret );
    buff[ret] = 0;
    rapidjson::Document doc;
    doc.Parse( buff );

    ASSERT_TRUE( doc.HasMember( "input" ) );
    const auto& input = doc["input"];
    for ( auto i = 0u; i < input.Size(); ++i )
    {
        m_ml->discover( "media/" + std::string( input[i].GetString() ) );
    }
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    if ( doc.HasMember( "expected" ) == false )
    {
        // That's a lousy test case with no assumptions, but ok.
        return;
    }
    const auto& expected = doc["expected"];

    if ( expected.HasMember( "albums" ) == true )
        checkAlbums( expected["albums" ] );
}

INSTANTIATE_TEST_CASE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases),
                        // gtest default parameter name displayer (testing::PrintToStringParamName)
                        // seems to add " " around the parameter name, making it invalid.
                        [](::testing::TestParamInfo<const char*> i){ return i.param; } );

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
