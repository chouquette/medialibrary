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

#include "Logger.h"

namespace medialibrary
{

std::shared_ptr<ILogger> Log::s_logger;
std::atomic<LogLevel> Log::s_logLevel;

void Log::doLog( LogLevel lvl, const std::string& msg )
{
    switch ( lvl )
    {
    case LogLevel::Error:
        s_logger->Error( msg );
        break;
    case LogLevel::Warning:
        s_logger->Warning( msg );
        break;
    case LogLevel::Info:
        s_logger->Info( msg );
        break;
    case LogLevel::Debug:
        s_logger->Debug( msg );
        break;
    case LogLevel::Verbose:
        s_logger->Verbose( msg );
        break;
    }
}

}
