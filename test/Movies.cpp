#include "Tests.h"

#include "IMediaLibrary.h"
#include "IMovie.h"
#include "IFile.h"

class Movies : public Tests
{
};

TEST_F( Movies, Create )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_NE( m, nullptr );
    ASSERT_EQ( m->title(), "movie" );
}

TEST_F( Movies, Fetch )
{
    auto m = ml->createMovie( "movie" );
    auto m2 = ml->movie( "movie" );

    ASSERT_EQ( m, m2 );

    Reload();

    m2 = ml->movie( "movie" );
    ASSERT_NE( m2, nullptr );
    ASSERT_EQ( m2->title(), "movie" );
}

TEST_F( Movies, SetReleaseDate )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->releaseDate(), 0u );
    m->setReleaseDate( 1234 );
    ASSERT_EQ( m->releaseDate(), 1234u );

    Reload();

    m = ml->movie( "movie" );
    ASSERT_EQ( m->releaseDate(), 1234u );
}

TEST_F( Movies, SetShortSummary )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->shortSummary().length(), 0u );
    m->setShortSummary( "great movie" );
    ASSERT_EQ( m->shortSummary(), "great movie" );

    Reload();

    m = ml->movie( "movie" );
    ASSERT_EQ( m->shortSummary(), "great movie" );
}

TEST_F( Movies, SetArtworkUrl )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->artworkUrl().length(), 0u );
    m->setArtworkUrl( "artwork" );
    ASSERT_EQ( m->artworkUrl(), "artwork" );

    Reload();

    m = ml->movie( "movie" );
    ASSERT_EQ( m->artworkUrl(), "artwork" );
}

TEST_F( Movies, SetImdbId )
{
    auto m = ml->createMovie( "movie" );
    ASSERT_EQ( m->imdbId().length(), 0u );
    m->setImdbId( "id" );
    ASSERT_EQ( m->imdbId(), "id" );

    Reload();

    m = ml->movie( "movie" );
    ASSERT_EQ( m->imdbId(), "id" );
}

TEST_F( Movies, AssignToFile )
{
    auto f = ml->addFile( "file" );
    auto m = ml->createMovie( "movie" );

    ASSERT_EQ( f->movie(), nullptr );
    f->setMovie( m );
    ASSERT_EQ( f->movie(), m );

    Reload();

    f = ml->file( "file" );
    m = f->movie();
    ASSERT_NE( m, nullptr );
    ASSERT_EQ( m->title(), "movie" );
}

TEST_F( Movies, DestroyMovie )
{
    auto f = ml->addFile( "file" );
    auto m = ml->createMovie( "movie" );

    f->setMovie( m );
    m->destroy();

    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );

    Reload();

    f = ml->file( "file" );
    ASSERT_EQ( f, nullptr );
}
