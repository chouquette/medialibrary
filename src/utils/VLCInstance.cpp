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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "VLCInstance.h"
#include "logging/Logger.h"
#include "vlcpp/vlc.hpp"

namespace
{
// Define this in the .cpp file to avoid including libvlcpp from the header.
struct Init
{
    Init()
    {
        const char* args[] = {
            "--no-lua",
        };
        instance = VLC::Instance( sizeof(args) / sizeof(args[0]), args );
        // Do not take the string by reference. libvlcpp is constructing the std::string
        // as it calls the log callback, so the string we receive will be move constructed
        instance.logSet([this](int lvl, const libvlc_log_t*, std::string msg) {
            if ( medialibrary::Log::logLevel() != medialibrary::LogLevel::Verbose )
                return;
            if ( lvl == LIBVLC_ERROR )
                medialibrary::Log::Error( msg );
            else if ( lvl == LIBVLC_WARNING )
                medialibrary::Log::Warning( msg );
            else
                medialibrary::Log::Info( msg );
        });
    }

    VLC::Instance instance;
};
}

namespace medialibrary
{

VLC::Instance& VLCInstance::get()
{
    // Instanciate the wrapper only once, and run initialization in its constructor.
    static Init wrapper;
    return wrapper.instance;
}

}
