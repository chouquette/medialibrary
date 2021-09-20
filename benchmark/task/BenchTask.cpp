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
#include "parser/Task.h"

static void BenchSetMeta( benchmark::State& state )
{
    while ( state.KeepRunning() )
    {
        medialibrary::parser::Task t{ nullptr, "dummy://task",
                                      medialibrary::IFile::Type::Main };
        t.setMeta( medialibrary::parser::Task::Metadata::Title, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::ArtworkUrl, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::ShowName, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Episode, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Album, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Genre, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Date, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::AlbumArtist, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Artist, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::TrackNumber, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::DiscNumber, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::DiscTotal, "value" );
        benchmark::DoNotOptimize( t );
    }
}

static void BenchSetAndReadMeta( benchmark::State& state )
{
    while ( state.KeepRunning() )
    {
        medialibrary::parser::Task t{ nullptr, "dummy://task",
                                      medialibrary::IFile::Type::Main };
        t.setMeta( medialibrary::parser::Task::Metadata::Title, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::ArtworkUrl, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::ShowName, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Episode, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Album, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Genre, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Date, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::AlbumArtist, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::Artist, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::TrackNumber, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::DiscNumber, "value" );
        t.setMeta( medialibrary::parser::Task::Metadata::DiscTotal, "value" );

        std::string v;
        v = t.meta( medialibrary::parser::Task::Metadata::Title );
        v = t.meta( medialibrary::parser::Task::Metadata::ArtworkUrl );
        v = t.meta( medialibrary::parser::Task::Metadata::ShowName );
        v = t.meta( medialibrary::parser::Task::Metadata::Episode );
        v = t.meta( medialibrary::parser::Task::Metadata::Album );
        v = t.meta( medialibrary::parser::Task::Metadata::Genre );
        v = t.meta( medialibrary::parser::Task::Metadata::Date );
        v = t.meta( medialibrary::parser::Task::Metadata::AlbumArtist );
        v = t.meta( medialibrary::parser::Task::Metadata::Artist );
        v = t.meta( medialibrary::parser::Task::Metadata::TrackNumber );
        v = t.meta( medialibrary::parser::Task::Metadata::DiscNumber );
        v = t.meta( medialibrary::parser::Task::Metadata::DiscTotal );

        benchmark::DoNotOptimize( t );
    }
}

// Register the function as a benchmark
BENCHMARK(BenchSetMeta);
BENCHMARK(BenchSetAndReadMeta);

// Run the benchmark
BENCHMARK_MAIN();
