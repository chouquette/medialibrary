#include "Tests.h"

#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IArtist.h"

class AlbumTracks : public Tests
{
};

TEST_F( AlbumTracks, Artists )
{
    auto album = ml->createAlbum( "album" );
    auto artist1 = ml->createArtist( "artist 1" );
    auto artist2 = ml->createArtist( "artist 2" );
    album->addTrack( "track 1", 1 );
    album->addTrack( "track 2", 2 );
    album->addTrack( "track 3", 3 );

    ASSERT_NE( artist1, nullptr );
    ASSERT_NE( artist2, nullptr );

    auto tracks = album->tracks();
    for ( auto& t : tracks )
    {
        t->addArtist( artist1 );
        t->addArtist( artist2 );

        auto artists = t->artists();
        ASSERT_EQ( artists.size(), 2u );
    }

    auto artists = ml->artists();
    for ( auto& a : artists )
    {
        auto tracks = a->tracks();
        ASSERT_EQ( tracks.size(), 3u );
    }

    Reload();

    album = ml->album( "album" );
    tracks = album->tracks();
    for ( auto& t : tracks )
    {
        auto artists = t->artists();
        ASSERT_EQ( artists.size(), 2u );
    }

    artists = ml->artists();
    for ( auto& a : artists )
    {
        auto tracks = a->tracks();
        ASSERT_EQ( tracks.size(), 3u );
    }
}
