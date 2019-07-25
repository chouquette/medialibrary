/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "SqliteTools.h"

#include <algorithm>

namespace medialibrary
{

namespace sqlite
{
std::unordered_map<Connection::Handle,
                    std::unordered_map<std::string, Statement::CachedStmtPtr>> Statement::StatementsCache;

compat::Mutex Statement::StatementsCacheLock;

std::string Tools::sanitizePattern( const std::string& pattern )
{
    // This assumes the input pattern is a regular user input, ie. that it is not
    // supposed to contain anything that ressembles SQL. What this actually means
    // is that if the user provides an escaped double quote «""» it will result in 2
    // escaped double quotes («""""»)
    std::string res;
    res.reserve( pattern.size() + 3 );
    res.append( "\"" );
    for ( const auto c : pattern )
    {
        if ( c == '"' || c == '\'' )
            res += c;
        res += c;
    }
    res.append( "*\"" );
    return res;
}

}

}
