/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2017-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Xml.h"

namespace medialibrary
{
namespace utils
{
namespace xml
{

std::string encode( const std::string& input )
{
    std::string res;
    res.reserve( input.length() );
    for ( auto i = 0u; i < input.length(); ++i )
    {
        switch ( input[i] )
        {
        case '&':
            res += "&amp;";
            break;
        case '<':
            res += "&lt;";
            break;
        case '>':
            res += "&gt;";
            break;
        case '"':
            res += "&quot;";
            break;
        case '\'':
            res += "&apos;";
            break;
        default:
            res += input[i];
            break;
        }
    }
    return res;
}

}
}
}
