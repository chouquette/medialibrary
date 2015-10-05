#include "Tests.h"

#include "IArtist.h"
#include "IAlbum.h"

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

TEST_F( Artists, Albums )
{
    auto artist = ml->createArtist( "Cannibal Otters" );
    auto album1 = ml->createAlbum( "album1" );
    auto album2 = ml->createAlbum( "album2" );

    ASSERT_NE( artist, nullptr );
    ASSERT_NE( album1, nullptr );
    ASSERT_NE( album2, nullptr );

    album1->addArtist( artist );
    album2->addArtist( artist );

    auto albums = artist->albums();
    ASSERT_EQ( albums.size(), 2u );

    Reload();

    artist = ml->artist( "Cannibal Otters" );
    albums = artist->albums();
    ASSERT_EQ( albums.size(), 2u );
}

TEST_F( Artists, GetAll )
{
    for ( int i = 0; i < 5; i++ )
    {
        auto a = ml->createArtist( std::to_string( i ) );
        ASSERT_NE( a, nullptr );
    }
    auto artists = ml->artists();
    ASSERT_EQ( artists.size(), 5u );

    Reload();

    artists = ml->artists();
    ASSERT_EQ( artists.size(), 5u );
}
