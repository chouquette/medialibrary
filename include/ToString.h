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

#include <string>

#ifndef __ANDROID__

template <typename T>
inline std::string toString(T t)
{
    static_assert(std::is_fundamental<T>::value, "This only supports fundamental types");
    return std::to_string(t);
}

#else

#include <sstream>

template <typename T>
inline std::string toString(T t)
{
    static_assert(std::is_fundamental<T>::value, "This only supports fundamental types");
    std::ostringstream ss;
    ss << std::forward<T>(t);
    return ss.str();
}

#endif
