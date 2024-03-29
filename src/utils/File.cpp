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

#include "medialibrary/filesystem/Errors.h"

#include <memory>
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

namespace medialibrary
{

namespace utils
{

namespace fs
{

bool copy( const std::string& from, const std::string& to )
{
#ifdef HAVE_LINK
    if ( link( from.c_str(), to.c_str() ) == 0 )
        return true;
    if ( errno == EEXIST )
    {
        remove( to );
        if ( link( from.c_str(), to.c_str() ) == 0 )
            return true;
    }
    switch ( errno )
    {
        case EXDEV: // No cross device link, too bad then, let's copy
        case EPERM: // In case the user isn't allowed to create links, or the
                    // filesystem doesn't support it, let's try with a deep copy.
            break;
        default:
            return false;
    }
#endif
    FILE *input = fopen( from.c_str(), "rb" );
    if ( input == nullptr )
        return false;

    FILE *output = fopen( to.c_str(), "wb" );
    if ( output == nullptr )
    {
        fclose( input );
        return false;
    }
    unsigned char buff[4096];

    bool success = true;
    do
    {
        auto nbRead = fread( buff, 1, 4096, input );
        if ( nbRead == 0 )
            break;
        if ( fwrite( buff, 1, nbRead, output ) == 0 )
        {
            success = false;
            break;
        }
    } while ( feof( input ) == 0 );

    if ( ferror( input ) )
        return false;
    fclose( input );

    return fclose( output ) == 0 && success;
}

bool remove( const std::string& path )
{
    return unlink( path.c_str() ) == 0;
}

uint64_t fileSize( const std::string& path )
{
    struct stat s;
    if ( stat( path.c_str(), &s ) != 0 )
    {
        throw ::medialibrary::fs::errors::Exception{ std::string{
            "Failed to compute file size for " + path } };
    }
    return s.st_size;
}

}

}

}
