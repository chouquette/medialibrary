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

#include <atomic>
#include <memory>
#include <sstream>

#include "medialibrary/ILogger.h"
#include "compat/Thread.h"

namespace medialibrary
{

class Log
{
private:
    template <typename T>
    static void createMsg( std::stringstream& s, T&& t )
    {
        s << std::forward<T>( t );
    }

    template <typename T, typename... Args>
    static void createMsg( std::stringstream& s, T&& t, Args&&... args )
    {
        s << std::forward<T>( t );
        createMsg( s, std::forward<Args>( args )... );
    }

    template <typename... Args>
    static std::string createMsg( Args&&... args )
    {
        std::stringstream stream;
        stream << "[T#" << compat::this_thread::get_id() << "] ";
        createMsg( stream, std::forward<Args>( args )... );
        return stream.str();
    }

    static void doLog( LogLevel lvl, const std::string& msg );

    template <typename... Args>
    static void log( LogLevel lvl, Args&&... args)
    {
        auto msg = createMsg( std::forward<Args>( args )... );
        doLog( lvl, msg );
    }

public:
    static void SetLogger( std::shared_ptr<ILogger> logger )
    {
        s_logger = std::move( logger );
    }

    static void setLogLevel( LogLevel level )
    {
        s_logLevel.store( level, std::memory_order_relaxed );
    }

    static LogLevel logLevel()
    {
        return s_logLevel.load( std::memory_order_relaxed );
    }

    template <typename... Args>
    static void Error( Args&&... args )
    {
        log( LogLevel::Error, std::forward<Args>( args )... );
    }

    template <typename... Args>
    static void Warning( Args&&... args )
    {
        if ( s_logLevel.load( std::memory_order_relaxed ) > LogLevel::Warning )
            return;
        log( LogLevel::Warning, std::forward<Args>( args )... );
    }

    template <typename... Args>
    static void Info( Args&&... args )
    {
        if ( s_logLevel.load( std::memory_order_relaxed ) > LogLevel::Info )
            return;
        log( LogLevel::Info, std::forward<Args>( args )... );
    }

    template <typename... Args>
    static void Debug( Args&&... args )
    {
        if ( s_logLevel.load( std::memory_order_relaxed ) > LogLevel::Debug )
            return;
        log( LogLevel::Debug, std::forward<Args>( args )... );
    }

    template <typename... Args>
    static void Verbose( Args&&... args )
    {
        if ( s_logLevel.load( std::memory_order_relaxed ) > LogLevel::Verbose )
            return;
        log( LogLevel::Verbose, std::forward<Args>( args )... );
    }

private:

private:
    static std::shared_ptr<ILogger> s_logger;
    static std::atomic<LogLevel> s_logLevel;
};

}

#if defined(_MSC_VER)
# define LOG_ORIGIN __FUNCDNAME__
#else
# define LOG_ORIGIN __FILE__, ":", __LINE__, ' ', __func__
#endif

#define LOG_ERROR( ... ) medialibrary::Log::Error( LOG_ORIGIN, ' ', __VA_ARGS__ )
#define LOG_WARN( ... ) medialibrary::Log::Warning( LOG_ORIGIN, ' ', __VA_ARGS__ )
#define LOG_INFO( ... ) medialibrary::Log::Info( LOG_ORIGIN, ' ', __VA_ARGS__ )
#define LOG_DEBUG( ... ) medialibrary::Log::Debug( LOG_ORIGIN, ' ', __VA_ARGS__ )
#define LOG_VERBOSE( ... ) medialibrary::Log::Verbose( LOG_ORIGIN, ' ', __VA_ARGS__ )
