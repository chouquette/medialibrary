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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

//#define XXH_NO_INLINE_HINTS 1

#include "XxHasher.h"

#define XXH_INLINE_ALL 1
#define XXH_STATIC_LINKING_ONLY 1

# include <xxhash.h>

#if defined(XXHASH_DYNAMIC_DISPATCH)
/*
 * The function is defined in another compile unit, but XXH_PUBLIC_API will
 * define is a static inline, which causes a static inline declaration to be
 * emited in this file while there's no definition available.
 * To work around this, we enforce an empty XXH_PUBLIC_API define before
 * including the dispatch header.
 */
# undef XXH_PUBLIC_API
# define XXH_PUBLIC_API
# include <xxh_x86dispatch.h>
#endif


#include <memory>
#include <cstdio>
#include <cinttypes>

namespace medialibrary
{
namespace utils
{
namespace hash
{

std::string toString( uint64_t hash )
{
    char buff[17];
    snprintf( buff, sizeof(buff), "%" PRIX64, hash );

    return std::string{ buff };
}

uint64_t xxFromBuff( const uint8_t* buff, size_t size )
{
    return XXH3_64bits( buff, size );
}

uint64_t xxFromFile( const std::string& path )
{
    std::unique_ptr<FILE, decltype(&fclose)> file{
        fopen( path.c_str(), "rb" ), &fclose
    };
    std::unique_ptr<XXH3_state_t, decltype(&XXH3_freeState)> state{
        XXH3_createState(), &XXH3_freeState
    };
    if ( state == nullptr )
        throw std::bad_alloc{};
    XXH3_64bits_reset( state.get() );
    uint8_t buff[ 64 * 64 ];
    while ( feof( file.get() ) == false )
    {
        auto read = fread( buff, 1, sizeof( buff ), file.get() );
        XXH3_64bits_update( state.get(), buff, read );
    }
    return XXH3_64bits_digest( state.get() );
}

}
}
}
