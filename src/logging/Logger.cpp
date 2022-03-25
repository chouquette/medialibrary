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
#include "logging/IostreamLogger.h"

namespace medialibrary
{

std::unique_ptr<ILogger> Log::s_defaultLogger = std::unique_ptr<ILogger>( new IostreamLogger );
std::atomic<ILogger*> Log::s_logger;
std::atomic<LogLevel> Log::s_logLevel;

void Log::doLog( LogLevel lvl, const std::string& msg )
{
    auto l = s_logger.load( std::memory_order_consume );
    if ( l == nullptr )
    {
        l = s_defaultLogger.get();
        // In case we're logging early (as in, before the static default logger has been constructed, don't blow up)
        if ( l == nullptr )
            return;
    }
    switch ( lvl )
    {
    case LogLevel::Error:
        l->Error( msg );
        break;
    case LogLevel::Warning:
        l->Warning( msg );
        break;
    case LogLevel::Info:
        l->Info( msg );
        break;
    case LogLevel::Debug:
        l->Debug( msg );
        break;
    case LogLevel::Verbose:
        l->Verbose( msg );
        break;
    }
}

}
