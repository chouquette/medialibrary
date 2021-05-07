/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Videolabs, VideoLAN
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
# include <fileapi.h>
#endif

#include <string.h>
#include "utils/Charsets.h"

static inline std::string getTempDir()
{
    auto forcedPath = getenv( "MEDIALIB_TEST_FOLDER" );
    if ( forcedPath != nullptr )
        return forcedPath;
#ifdef _WIN32
    WCHAR path[MAX_PATH];
    GetTempPathW( MAX_PATH, path );
    auto utf8 = medialibrary::charset::FromWide( path );
    return utf8.get();
#else
    return "/tmp/";
#endif
}

static inline std::string getTempPath( std::string filename )
{
    return getTempDir() + "medialib/" + std::move(filename) + "/";
}
