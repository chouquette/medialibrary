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

#pragma once

#ifdef _WIN32

#include <windows.h>
#include <memory>

namespace medialibrary
{
namespace charset
{

static inline std::unique_ptr<char[]> FromWide( const wchar_t *wide )
{
    size_t len = WideCharToMultiByte( CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr );
    if (len == 0)
        return nullptr;

    char *out = new char[len];
    WideCharToMultiByte( CP_UTF8, 0, wide, -1, out, len, nullptr, nullptr );
    return std::unique_ptr<char[]>( out );
}

static inline std::unique_ptr<wchar_t[]> ToWide( const char *utf8 )
{
    int len = MultiByteToWideChar( CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len == 0)
        return nullptr;

    wchar_t *out = new wchar_t[len];
    MultiByteToWideChar( CP_UTF8, 0, utf8, -1, out, len );
    return std::unique_ptr<wchar_t[]>( out );
}

}
}

#endif
