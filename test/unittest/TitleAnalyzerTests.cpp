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

#include "Tests.h"

#include "utils/TitleAnalyzer.h"

/* This test causes wine to crash, but is now fixed upstream.
 * See https://github.com/mstorsjo/llvm-mingw/issues/52
 */
#ifndef _WIN32

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

TEST_F( TitleAnalyzerTests, SimpleTests )
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
}

TEST_F( TitleAnalyzerTests, RemovePatterns )
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
    CHECK( "Game of Thrones S08E02 WEB ADRENALiNE",
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
    CHECK( "chernobyl s01e02 internal web",
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
}

#endif
