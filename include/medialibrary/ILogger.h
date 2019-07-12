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

#include <string>

namespace medialibrary
{

enum class LogLevel
{
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
};

class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void Error( const std::string& msg ) = 0;
    virtual void Warning( const std::string& msg ) = 0;
    virtual void Info( const std::string& msg ) = 0;
    virtual void Debug( const std::string& msg ) = 0;
    virtual void Verbose( const std::string& msg ) = 0;
};

}
