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

#pragma once

#include <string>

namespace medialibrary
{
namespace utils
{
namespace url
{

std::string decode( const std::string& str );
std::string encode( const std::string& str );

/**
 * @brief path Returns the MRL path, ie. the mrl without the host and scheme parts
 * @param mrl The MRL to remove the host from
 *
 * For instance, removeHost( foo://1.2.3.4/path/to/file.bar will return
 * "/path/to/file.bar"
 */
std::string path( const std::string& mrl );

/**
 * @brief stripScheme Remove the scheme from a MRL
 * @param mrl
 */
std::string stripScheme( const std::string& mrl );

/**
 * @brief scheme Returns the scheme used in an MRL
 * ie. for seaOtter://foo.bar it will return seaOtter://
 */
std::string  scheme( const std::string& mrl );

/**
 * @brief toLocalPath Converts an MRL to a local path.
 * This will strip the file:// scheme and URL decode the MRL
 */
std::string  toLocalPath( const std::string& mrl );

/**
 * @brief schemeIs Check if a mrl start with a specific scheme
 */
bool schemeIs( const std::string& scheme, const std::string& mrl );

struct parts
{
    std::string scheme;
    std::string userInfo;
    bool hostMarker;
    std::string host;
    std::string port;
    std::string path;
    std::string query;
    std::string fragments;
};

/**
 * @brief split Splits an URL as per RFC 3986
 * @param url The URL to split
 * @return A parts struct representing the different URL segments
 *
 * See https://tools.ietf.org/html/rfc3986#section-3
 */
parts split( const std::string& url );

}
}
}
