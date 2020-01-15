/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2020 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <utility>

namespace medialibrary
{

namespace utils
{

template <typename Func>
class Defer final
{
public:
    template <typename FuncFwd>
    Defer( FuncFwd&& f )
        : m_func( std::forward<FuncFwd>( f ) )
    {
    }

    ~Defer()
    {
        m_func();
    }

    Defer( const Defer& ) = delete;
    Defer& operator=( const Defer& ) = delete;
    Defer( Defer&& ) = default;
    Defer& operator=( Defer&& ) = default;

private:
    Func m_func;
};

template <typename T>
Defer<T> make_defer( T&& func )
{
    return Defer<T>( std::forward<T>( func ) );
}

}

}
