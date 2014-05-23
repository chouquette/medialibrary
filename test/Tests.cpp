#include "gtest/gtest.h"

class TestEnv : public ::testing::Environment
{
    public:
        virtual void SetUp()
        {
            // Always clean the DB in case a previous test crashed
            unlink("test.db");
        }
};

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new TestEnv);
