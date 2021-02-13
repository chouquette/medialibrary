/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "UnitTests.h"

#include "utils/TitleAnalyzer.h"

class TitleAnalyzerTests : public Tests
{
};

#define CHECK(out, in) \
    do \
    { \
        auto s = utils::title::sanitize( in ); \
        ASSERT_EQ( out, s ); \
    } \
    while ( false );

static void SimpleTests( Tests* )
{
    // Check that we manage to remove patterns
    CHECK( "movie", "720p movie" );
    // Ensure we don't remove patterns in the middle of a word
    CHECK( "blablax264blabla", "blablax264blabla" );
    // Check that we correctly remove multiple patterns
    CHECK( "sea otter", "xvid sea 1080p otter bluray" );
    // Case insensitive
    CHECK( "sea otter", "sea RIP otter bLuRAy" );
    // Check for separators removal
    CHECK( "word word", "word-word" );
    CHECK( "word word", "word - word" );
    // Check that we don't remove "..."
    CHECK( "Once upon a time in... Hollywood",
           "Once upon a time in... Hollywood" );
    // Check that we abort sanitization if we were to return an empty
    // string, which most likely means we've removed too much content
    CHECK( "720p", "720p" );
    CHECK( "(Trimmed)", "(   Trimmed  )" );
    CHECK( "[Trimmed]", "[   Trimmed  ]" );
    CHECK( "test", "test.toto" );
    CHECK( "foo", "foo.123" );
    CHECK( "trailing separators nospace", "trailing separators nospace." );
    CHECK( "trailing separators", "trailing separators ." );
    CHECK( "some words", "some words -" );
    CHECK( "a file", "a 123GB file" );
    CHECK( "a file", "a 1.23GB file" );
    CHECK( "a file", "a 123MB file" );
    CHECK( "a 1 2 3GB file", "a 1.2.3GB file" );
    CHECK( "something something 12GB file", "something something.12GB file" );
    CHECK( "Audio track is", "Audio track is DDP2.0" );
    CHECK( "Audio track is", "Audio track is DDP2.1" );
    CHECK( "Audio track is", "Audio track is DDP.5.1" );
    CHECK( "Weird writing for", "Weird writing for H-264" );
    CHECK( "Weird writing for", "Weird writing for H_265" );
    CHECK( "A movie about the web", "A movie about the web.mkv" );
    CHECK( "The dark web", "The dark web" );
}

static void RemovePatterns( Tests* )
{
    CHECK( "Deadly Still 2019",
           "Deadly.Still.2019.BDRip.XviD.AC3-EVO" );
    CHECK( "Avengers Endgame 2019 SPECIAL EDITION GalaxyRG",
           "Avengers.Endgame.2019.HDTC.SPECIAL-1337x-EDITION.x264-GalaxyRG" );
    CHECK( "The Avengers 2012",
           "The Avengers 2012 720p BRrip X264 - 1GB - YIFY" );
    CHECK( "Avengers: Age of Ultron (2015)",
           "Avengers: Age of Ultron (2015) 720p BrRip x264 - YIFY" );
    CHECK( "Avengers Infinity War 2018",
           "Avengers.Infinity.War.2018.1080p.WEB-DL.H264.AC3-EVO[EtHD]" );
    CHECK( "Avengers Infinity War 2018 English",
           "Avengers Infinity War 2018 English 720p HD-TS x264 AAC - xRG" );
    CHECK( "Game of Thrones S08E02",
           "Game.of.Thrones.S08E02.1080p.WEB.x264-ADRENALiNE[ettv]" );
    CHECK( "Game of Thrones S07 Complete Season 7",
           "Game of Thrones S07 Complete Season 7 720p x264 AC3 5.1 (MP4)" );
    CHECK( "Friends S02E19 The One Where Eddie Won't Go (Joy)",
           "Friends S02E19 The One Where Eddie Won't Go (1080p x265 10bit Joy).mkv");
    CHECK( "Friends S02E19 The One Where Eddie Won't Go (Joy)",
           "Friends S02E19 The One Where Eddie Won't Go (1080p x265 10 bits Joy).mkv");
    CHECK( "Radiant S2 01",
           "[HorribleSubs] Radiant S2 - 01 [480p].mkv" );
    CHECK( "Desperate Housewives 4x01 Now You Know",
           "Desperate Housewives - 4x01 - Now You Know VOST FR.avi" );
    CHECK( "Tampopo Juzo Itami (1987)",
           "Tampopo - Juzo Itami (1987) - vost En.avi" );
    CHECK( "Shinchou Yuusha Kono Yuusha ga Ore Tueee Kuse ni Shinchou Sugiru 01",
           "[Ohys-Raws] Shinchou Yuusha Kono Yuusha ga Ore Tueee Kuse ni Shinchou Sugiru - 01 (AT-X 1280x720 x264 AAC).mp4");
    CHECK( "The Walking Dead S08 E02",
           "The.Walking.Dead.S08.E02.1080p.BluRay.x264-ROVERS AMC.mkv" );
    CHECK( "Ant Man And The Wasp 2018",
           "Ant-Man.And.The.Wasp.2018.720p.WEBRip.x264-[YTS.AM].mp4" );
    CHECK( "Enter The Void 2009",
           "Enter.The.Void.2009.720p.BluRay.H264.AAC-RARBG.mp4" );
    CHECK( "Jurassic World Fallen Kingdom 2018",
           "Jurassic.World.Fallen.Kingdom.2018.1080p.BluRay.x264-[YTS.AM].mp4" );
    CHECK( "Kong Skull Island 2017",
           "Kong.Skull.Island.2017.720p.BluRay.x264-[YTS.AG].mp4" );
    CHECK( "Coffee and Cigarettes 2003",
           "Coffee.and.Cigarettes.2003.1080p.BluRay.x264.anoXmous__.mp4" );
    CHECK( "chernobyl s01e02 internal",
           "chernobyl.s01e02.internal.1080p.web.h264-memento.mkv" );
    CHECK( "Doctor Strange 2016",
           "Doctor.Strange.2016.1080p.HDRip.X264.AC3-EVO[EtHD].mkv" );
    CHECK( "Du Jour Au Lendemain FRENCH",
           "Du.Jour.Au.Lendemain.FRENCH.DVDRip.XviD-LOST-UGM.avi" );
    CHECK( "Escape From New York 1981",
           "Escape.From.New.York.1981.1080p.BrRip.x264.BOKUTOX.YIFY.mp4" );
    CHECK( "Memento (2000)",
           "Memento (2000) 1080p BrRip x264 - 1.6GB - YIFY" );
    CHECK( "Uchuu Patrol Luluco 10 [967D0521]",
           "[PuyaSubs!] Uchuu Patrol Luluco - 10 [720p][967D0521].mkv" );
    CHECK( "Code Geass Lelouch of the Rebellion R2 15 [DCA806F7]",
           "[Eclipse] Code Geass - Lelouch of the Rebellion R2 - 15 (1280x720 h264) [DCA806F7].mkv" );
    CHECK( "Youkoso Japari Park 19 ~ 22 [Multiple Subtitle]",
           "[Erai-raws] Youkoso Japari Park - 19 ~ 22 [1080p][Multiple Subtitle]" );
    CHECK( "Nanatsu no Taizai Kamigami no Gekirin 01",
           "[Ohys-Raws] Nanatsu no Taizai Kamigami no Gekirin - 01 (TX 1280x720 x264 AAC).mp4" );
    CHECK( "snow raws unwrapped 第09話",
           "[Snow-Raws] snow-raws-unwrapped 第09話 (BD 1920x1080 HEVC-YUV420P10 FLAC)" );
    CHECK( "American Horror Story 1984 S09E09 Final Girl",
           "American.Horror.Story.1984.S09E09.Final.Girl.HDTV.x264-CRiMSON" );
    CHECK( "Mr Robot S04E01 401 Unauthorized",
           "Mr.Robot.S04E01.401.Unauthorized.1080p.AMZN.WEB-DL.DDP5.1.H.264-.mkv" );
    CHECK( "Mr Robot S04E02",
           "Mr.Robot.S04E02.720p.WEB.x265-MiNX[TGx].mkv" );
}

#undef CHECK

#define CHECK( input, success, season, episode ) \
    do \
    { \
        auto sanitized = utils::title::sanitize( input ); \
        auto res = utils::title::analyze( sanitized ); \
        if ( success == false ) \
            ASSERT_FALSE( std::get<0>( res ) ); \
        else \
        { \
            ASSERT_TRUE( std::get<0>( res ) ); \
            ASSERT_EQ( season, std::get<1>( res ) ); \
            ASSERT_EQ( episode, std::get<2>( res ) ); \
        } \
    } \
    while ( false );

static void EpisodeNumber( Tests* )
{
    // Simple tests for season/episode number extraction
    CHECK( "S02e03", true, 2u, 3u );
    CHECK( "S02x03", true, 2u, 3u );
    CHECK( "S02   03", true, 2u, 3u );
    CHECK( "S12  E123", true, 12u, 123u );
    // Because some people use "B(ooks)"
    CHECK( "B12E123", true, 12u, 123u );

    CHECK( "S02xx03", false, 0u, 0u );
}

#undef CHECK

#define CHECK( input, season, episode, showName, episodeTitle ) \
    do \
    { \
        auto sanitized = utils::title::sanitize( input ); \
        auto res = utils::title::analyze( sanitized ); \
        ASSERT_TRUE( std::get<0>( res ) ); \
        ASSERT_EQ( season, std::get<1>( res ) ); \
        ASSERT_EQ( episode, std::get<2>( res ) ); \
        ASSERT_EQ( showName, std::get<3>( res ) ); \
        ASSERT_EQ( episodeTitle, std::get<4>( res ) ); \
    } \
    while ( false );

static void FullExtraction( Tests* )
{
    CHECK( "The.Walking.Dead.S08.E02.1080p.BluRay.x264-ROVERS AMC.mkv",
           8u, 2u, "The Walking Dead", "" );
    CHECK( "Friends S02E19 The One Where Eddie Won't Go (1080p x265 10bit Joy).mkv",
           2u, 19u, "Friends", "The One Where Eddie Won't Go (Joy)" );
    CHECK( "Desperate Housewives - 4x01 - Now You Know VOST FR.avi",
           4u, 1u, "Desperate Housewives", "Now You Know" );
    CHECK( "Underground.Marvels.S01E05.Cave.of.the.Body.Snatchers.480p.x264-",
           1u, 5u, "Underground Marvels", "Cave of the Body Snatchers" );
    CHECK( "MasterChef The Professionals S12E04 1080p HEVC x265-MeGusta",
           12u, 4u, "MasterChef The Professionals", "" );
    CHECK( "American.Horror.Story.1984.S09E09.Final.Girl.HDTV.x264-CRiMSON",
           9u, 9u, "American Horror Story 1984", "Final Girl" );
}

int main( int ac, char** av )
{
    INIT_TESTS;

    ADD_TEST( SimpleTests );
    ADD_TEST( RemovePatterns );
    ADD_TEST( EpisodeNumber );
    ADD_TEST( FullExtraction );

    END_TESTS;
}
