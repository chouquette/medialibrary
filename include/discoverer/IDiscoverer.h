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

#ifndef IDISCOVERER_H
# define IDISCOVERER_H

#include <string>
#include "Types.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "medialibrary/IMediaLibrary.h"

namespace medialibrary
{

class IDiscoverer
{
public:
    virtual ~IDiscoverer() = default;
    // We assume the media library will always outlive the discoverers.
    //FIXME: This is currently false since there is no way of interrupting
    //a discoverer thread
    virtual bool discover( const std::string& entryPoint ) = 0;
    virtual void reload() = 0;
    virtual void reload( const std::string& entryPoint ) = 0;
};

}

#endif // IDISCOVERER_H
