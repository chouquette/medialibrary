/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
#include "utils/XxHasher.h"

template <typename HASH>
static void DoBench( benchmark::State& state )
{
    std::string::size_type stringLength = state.range( 0 );
    std::string input( stringLength, 'x' );

    while ( state.KeepRunning() )
    {
        std::unordered_map<std::string, int, HASH> um;
        um.emplace( input, 1234 );
        auto res = um.find( input );
        benchmark::DoNotOptimize( res );
    }
}

static void BenchNormal( benchmark::State& state )
{
    DoBench<std::hash<std::string>>( state );
}

static void BenchXXHash( benchmark::State& state )
{
    DoBench<medialibrary::utils::hash::xxhasher>( state );
}


BENCHMARK(BenchNormal)->RangeMultiplier( 2 )->Range( 16, 2048 );
BENCHMARK(BenchXXHash)->RangeMultiplier( 2 )->Range( 16, 2048 );

BENCHMARK_MAIN();
