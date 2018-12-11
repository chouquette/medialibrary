/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2018 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
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

#include "Strings.h"

#include <algorithm>
#include <cctype>

namespace medialibrary
{
namespace utils
{
namespace str
{

std::string& trim( std::string& value )
{
    value.erase( begin( value ), std::find_if( begin( value ), end( value ), []( char c ) {
            return isspace( c ) == false;
        }));
    value.erase( std::find_if( value.rbegin(), value.rend(), []( char c ) {
            return isspace( c ) == false;
        }).base(), value.end() );
    return value;
}

std::string trim( const std::string& value )
{
    std::string v = value;
    trim( v );
    return v;
}

}
}
}
