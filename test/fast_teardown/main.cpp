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

#include "medialibrary/IMediaLibrary.h"
#include "mocks/NoopCallback.h"

#include <iostream>
#include <condition_variable>
#include <mutex>

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        std::cerr << "usage: " << argv[0] << " <entrypoint>" << std::endl;
        return 1;
    }

    for ( auto i = 0; i < 1000; ++i )
    {
        auto testCb = std::make_unique<mock::NoopCallback>();
        std::unique_ptr<medialibrary::IMediaLibrary> ml( NewMediaLibrary() );
        ml->initialize( "/tmp/test.db", "/tmp/ml_folder", testCb.get() );
        ml->start();
        ml->discover( argv[1] );
        ml->reload();
    }
    return 0;
}
