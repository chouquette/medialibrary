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

#include "medialibrary/ILogger.h"
#include <iostream>

namespace medialibrary
{

class IostreamLogger : public ILogger
{
public:
    virtual void Error(const std::string& msg) override
    {
        std::cout << "Error: " << msg << '\n';
    }

    virtual void Warning(const std::string& msg) override
    {
        std::cout << "Warning: " << msg << '\n';
    }

    virtual void Info(const std::string& msg) override
    {
        std::cout << "Info: " << msg << '\n';
    }

    virtual void Debug(const std::string& msg) override
    {
        std::cout << "Debug: " << msg << '\n';
    }

    virtual void Verbose(const std::string& msg) override
    {
        std::cout << "Verbose: " << msg << '\n';
    }
};

}
