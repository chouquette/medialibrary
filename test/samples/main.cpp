#include "Tester.h"

static std::string TestDirectory = SRC_DIR "/test/samples/";
bool Verbose = false;
bool ExtraVerbose = false;

static const char* testCases[] = {
    "featuring",
    "parse_video",
    "parse_audio",
    "same_album_name_different_artist",
    "same_album_name_same_artist",
    "compilation",
    "compilation_no_albumartist",
    "release_year_same",
    "notags",
    "multi_cd",
    "no_album_artist",
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

TEST_P( Tests, Parse )
{
    auto casePath = TestDirectory + "testcases/" + GetParam() + ".json";
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
        // Quick and dirty check to ensure we're discovering something that exists
        auto samplesDir = TestDirectory + "samples/" + input[i].GetString();
        struct stat s;
        auto res = stat( samplesDir.c_str(), &s );
        ASSERT_EQ( 0, res );

        m_ml->discover( samplesDir );
    }
    ASSERT_TRUE( m_cb->waitForParsingComplete() );

    if ( doc.HasMember( "expected" ) == false )
    {
        // That's a lousy test case with no assumptions, but ok.
        return;
    }
    const auto& expected = doc["expected"];

    if ( expected.HasMember( "albums" ) == true )
    {
        checkAlbums( expected["albums" ], m_ml->albums( SortingCriteria::Default, false ) );
    }
    if ( expected.HasMember( "media" ) == true )
        checkMedias( expected["media"] );
    if ( expected.HasMember( "nbVideos" ) == true )
    {
        const auto videos = m_ml->videoFiles( SortingCriteria::Default, false );
        ASSERT_EQ( expected["nbVideos"].GetUint(), videos.size() );
    }
    if ( expected.HasMember( "nbAudios" ) == true )
    {
        const auto audios = m_ml->audioFiles( SortingCriteria::Default, false );
        ASSERT_EQ( expected["nbAudios"].GetUint(), audios.size() );
    }
    if ( expected.HasMember( "artists" ) )
    {
        checkArtists( expected["artists"], m_ml->artists( SortingCriteria::Default, false ) );
    }
}

int main(int ac, char** av)
{
    ::testing::InitGoogleTest(&ac, av);
    const std::string verboseArg = "-v";
    const std::string extraVerboseArg = "-vv";
    for ( auto i = 1; i < ac; ++i )
    {
        if ( av[i] == verboseArg )
            Verbose = true;
        else if ( av[i] == extraVerboseArg )
            ExtraVerbose = true;
    }
    return RUN_ALL_TESTS();
}

INSTANTIATE_TEST_CASE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases) );

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
