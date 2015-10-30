#include "gtest/gtest.h"
#include <memory>

#include "MediaLibrary.h"
#include "Tester.h"

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

class Tests : public ::testing::TestWithParam<const char*>
{
protected:
    std::unique_ptr<MediaLibrary> ml;

    virtual void SetUp() override
    {
        ml.reset( new MediaLibrary );
        ml->setVerbosity( LogLevel::Error );
    }
    virtual void TearDown() override
    {
        ml.reset();
    }
};

TEST_P( Tests, Parse )
{
    std::unique_ptr<Tester> t( new Tester( ml.get(), GetParam() ) );
    t->run();
}

INSTANTIATE_TEST_CASE_P(SamplesTests, Tests,
                        ::testing::ValuesIn(testCases),
                        // gtest default parameter name displayer (testing::PrintToStringParamName)
                        // seems to add " " around the parameter name, making it invalid.
                        [](::testing::TestParamInfo<const char*> i){ return i.param; } );

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
