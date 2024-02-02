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

#include "Date.h"
#include "Defer.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <locale>

namespace medialibrary
{
namespace utils
{
namespace date
{

time_t mktime( struct tm* t )
{
#ifndef _WIN32
    const auto tz = getenv( "TZ" );
    setenv( "TZ", "UTC", 1 );
    tzset();
    auto d = utils::make_defer([tz](){
        if ( tz != nullptr )
            setenv( "TZ", tz, 1 );
        else
            unsetenv( "TZ" );
        tzset();
    });
    return ::mktime( t );
#else
    return _mkgmtime( t );
#endif
}

bool fromStr( const std::string& str, tm* t )
{
    memset( t, 0, sizeof( *t ) );
    t->tm_isdst = -1;
    std::istringstream ss{ str };
    ss.imbue( std::locale{ "C" } );
    ss >> std::get_time(t, "%a, %d %b %Y %H:%M:%S" );
    if ( ss.fail() == true )
        return false;
    auto pos_ = ss.tellg();
    if ( pos_ < 0 )
        return false;
    auto pos = static_cast<std::string::size_type>( pos_ );
    if ( pos >= str.length() )
        return true;
    /*
     * We need to parse the timezone ourselves as glibc's strptime accepts but
     * ignores it and standard strptime/std::get_time just don't recognize %Z
     * https://datatracker.ietf.org/doc/html/rfc822#section-5.1
     */
    const char* remainder = str.c_str() + pos;
    while ( isspace( *remainder ) != 0 )
    {
        ++remainder;
        ++pos;
    }
    auto hoursIncrement = 0;
    auto minutesIncrement = 0;
    if ( strcmp( remainder, "UT" ) == 0 || strcmp( remainder, "GMT" ) == 0 )
        return true;
    if ( strcmp( remainder, "EST" ) == 0 )
        hoursIncrement = 5;
    else if ( strcmp( remainder, "EDT" ) == 0 )
        hoursIncrement = 5;
    else if ( strcmp( remainder, "CST" ) == 0 )
        hoursIncrement = 6;
    else if ( strcmp( remainder, "CDT" ) == 0 )
        hoursIncrement = 5;
    else if ( strcmp( remainder, "MST" ) == 0 )
        hoursIncrement = 7;
    else if ( strcmp( remainder, "MDT" ) == 0 )
        hoursIncrement = 6;
    else if ( strcmp( remainder, "PST" ) == 0 )
        hoursIncrement = 8;
    else if ( strcmp( remainder, "PDT" ) == 0 )
        hoursIncrement = 7;
    else if ( isalpha( *remainder ) && *( remainder + 1 ) == 0 )
    {
        switch ( *remainder )
        {
        case 'Z':
            break; // Equivalent to UT/GMT
        case 'A':
            hoursIncrement = 1;
            break;
        case 'M':
            hoursIncrement = 12;
            break;
        case 'N':
            hoursIncrement = -1;
            break;
        case 'Y':
            hoursIncrement = -12;
            break;
        default:
            return false;
        }
    }
    else if ( *remainder == '+' || *remainder == '-' )
    {
        /*
         * This is counter intuitive (at least to me) but the sign expressed in
         * the timezone needs to be inverted when it comes to the offset we apply
         * to the hours/seconds.
         * We aim to express the time in UTC, so if it's 1:00pm in GMT +2 we need
         * to subtract 2 hours to the given date to express the time it was in
         * GMT/UTC (in this example, it's 11:00AM GMT)
         */
        int sign = *remainder == '-' ? 1 : -1;
        ++remainder;
        /* We now need 4 digits in addition to the sign */
        if ( str.length() - pos < 5 )
            return false;
        if ( isdigit( remainder[0] ) == false ||
             isdigit( remainder[1] ) == false ||
             isdigit( remainder[2] ) == false ||
             isdigit( remainder[3] ) == false )
        {
            return false;
        }
        hoursIncrement = sign * ( ( remainder[0] - '0' ) * 10 + remainder[1] - '0' );
        minutesIncrement = sign * ( ( remainder[2] - '0' ) * 10 + remainder[3] - '0' );
    }
    else
        return false;
    /*
     * Since the values are normalized by mktime, we can return values with
     * "overflows". 25 hours will be converted to an extra day and 1 hour
     */
    t->tm_hour += hoursIncrement;
    t->tm_min += minutesIncrement;
    return true;
}

}
}
}
