#include "Tests.h"

#include "IArtist.h"

class Artists : public Tests
{
};

TEST_F( Artists, Create )
{
    auto a = ml->createArtist( "Flying Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "Flying Otters" );

    Reload();

    a = ml->artist( "Flying Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->name(), "Flying Otters" );
}

TEST_F( Artists, ShortBio )
{
    auto a = ml->createArtist( "Raging Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->shortBio(), "" );

    std::string bio("An otter based post-rock band");
    a->setShortBio( bio );
    ASSERT_EQ( a->shortBio(), bio );

    Reload();

    a = ml->artist( "Raging Otters" );
    ASSERT_NE( a, nullptr );
    ASSERT_EQ( a->shortBio(), bio );
}
