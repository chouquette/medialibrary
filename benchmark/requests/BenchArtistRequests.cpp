/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2022 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "BenchRequestsCommon.h"
#include <benchmark/benchmark.h>
#include "Artist.h"

static void ListAllArtists( benchmark::State& state )
{
    auto bml = commonInit();
    QueryParameters params{};
    params.sort = static_cast<SortingCriteria>( state.range( 0 ) );
    for ( auto _ : state )
    {
        auto artists = bml.ml->artists( ArtistIncluded::All, &params )->all();
        benchmark::DoNotOptimize( artists );
    }
}

static void ListAllPublicArtists( benchmark::State& state )
{
    auto bml = commonInit();
    QueryParameters params{};
    params.publicOnly = true;
    for ( auto _ : state )
    {
        auto artists = bml.ml->artists( ArtistIncluded::All, &params )->all();
//        assert(artists.empty() == true);
        benchmark::DoNotOptimize( artists );
    }
}

BENCHMARK( ListAllArtists )
    ->Arg( toInt( SortingCriteria::Duration ) )
    ->Arg( toInt( SortingCriteria::ReleaseDate ) )
    ->Arg( toInt( SortingCriteria::InsertionDate ) )
    ->Arg( toInt( SortingCriteria::Alpha ) )
    ->Arg( toInt( SortingCriteria::Default ) );

BENCHMARK( ListAllPublicArtists );
