/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
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

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <vlcpp/vlc.hpp>

namespace medialibrary
{

class Media;
class File;

namespace parser
{

struct Task
{
    enum class Status
    {
        /// Default value.
        /// Also, having success = 0 is not the best idea ever.
        Unknown,
        /// All good
        Success,
        /// We can't compute this file for now (for instance the file was on a network drive which
        /// isn't connected anymore)
        TemporaryUnavailable,
        /// Something failed and we won't continue
        Fatal
    };

    Task( std::shared_ptr<Media> media, std::shared_ptr<File> file )
        : media( media )
        , file( file )
        , currentService( 0 )
    {
    }

    Task( const std::string& mrl )
            : mrl{ mrl }
            , currentService{ 0 }
    {
    }

    std::shared_ptr<Media>  media;
    std::shared_ptr<File>   file;
    std::string             mrl;
    VLC::Media              vlcMedia;
    unsigned int            currentService;
};

}

}
