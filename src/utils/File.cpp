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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "File.h"

#include <memory>
#include <cstdio>
#include <unistd.h>

namespace medialibrary
{

namespace utils
{

namespace fs
{

bool copy( const std::string& from, const std::string& to )
{
    std::unique_ptr<FILE, decltype(&fclose)> input{
        fopen( from.c_str(), "rb" ), &fclose
    };
    std::unique_ptr<FILE, decltype(&fclose)> output{
        fopen( to.c_str(), "wb" ), &fclose
    };
    if ( input == nullptr || output == nullptr )
        return false;
    unsigned char buff[4096];
    size_t nbRead;
    do
    {
        nbRead = fread( buff, 1, 4096, input.get() );
        if ( nbRead == 0 )
        {
            if ( ferror( input.get() ) )
                return false;
            break;
        }
        if ( fwrite( buff, 1, nbRead, output.get() ) == 0 )
            return false;

    } while ( nbRead > 0 );
    return true;
}

bool remove( const std::string& path )
{
    return unlink( path.c_str() ) == 0;
}

}

}

}
