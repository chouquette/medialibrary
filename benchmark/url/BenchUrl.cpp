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
#include "utils/Url.h"

static void BenchUrlSplit( benchmark::State& state )
{
    std::string input = "foo://example.com:8042/over/there?name=ferret#nose";
    while ( state.KeepRunning() )
    {
        benchmark::DoNotOptimize( medialibrary::utils::url::split( input ) );
    }
}

static void BenchUrlDecode( benchmark::State& state )
{
    std::string input = "file://%22url%22%20%21benchmark%23%20with%20sp%E2%82%ACci%40l%20c%21%21%24%23%25aracters";
    while ( state.KeepRunning() )
    {
        benchmark::DoNotOptimize( medialibrary::utils::url::decode( input ) );
    }
}


// Register the function as a benchmark
BENCHMARK(BenchUrlSplit);
BENCHMARK(BenchUrlDecode);

// Run the benchmark
BENCHMARK_MAIN();
