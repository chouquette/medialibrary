#include "Tester.h"

static std::string SamplesDirectory = ".";
static std::string TestCaseDirectory = SRC_DIR "/test/samples/testcases";

static const char* testCases[] = {
    "featuring",
    "parse_video",
    "parse_audio",
    "same_album_name_different_artist",
    "same_album_name_same_artist",
    "compilation",
    "release_year_same",
    "notags"
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
    auto casePath = TestCaseDirectory + "/" + GetParam() + ".json";
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
        auto samplesDir = SamplesDirectory + "/" + input[i].GetString();
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
        checkAlbums( expected["albums" ], m_ml->albums() );
    }
    if ( expected.HasMember( "media" ) == true )
        checkMedias( expected["media"] );
    if ( expected.HasMember( "nbVideos" ) == true )
    {
        const auto videos = m_ml->videoFiles();
        ASSERT_EQ( expected["nbVideos"].GetUint(), videos.size() );
    }
    if ( expected.HasMember( "nbAudios" ) == true )
    {
        const auto audios = m_ml->audioFiles();
        ASSERT_EQ( expected["nbAudios"].GetUint(), audios.size() );
    }
    if ( expected.HasMember( "artists" ) )
    {
        checkArtists( expected["artists"], m_ml->artists() );
    }
}

int main(int ac, char** av)
{
    ::testing::InitGoogleTest(&ac, av);
    const std::string samplesArg = "--samples-directory=";
    const std::string testCasesArg = "--testcases-directory=";
    for ( auto i = 1; i < ac; ++i )
    {
        if ( strncmp( samplesArg.c_str(), av[i], samplesArg.length() ) == 0 )
            SamplesDirectory = av[i] + samplesArg.size();
        else if ( strncmp( testCasesArg.c_str(), av[i], testCasesArg.length() ) == 0 )
            TestCaseDirectory = av[i] + testCasesArg.size();
    }
    return RUN_ALL_TESTS();
}

INSTANTIATE_TEST_CASE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases),
                        // gtest default parameter name displayer (testing::PrintToStringParamName)
                        // seems to add " " around the parameter name, making it invalid.
                        [](::testing::TestParamInfo<const char*> i){ return i.param; } );

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
