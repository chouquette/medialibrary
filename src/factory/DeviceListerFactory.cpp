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

#include "DeviceListerFactory.h"

#if defined(__linux__) && !defined(__ANDROID__)
# include "filesystem/unix/DeviceLister.h"
# define USE_BUILTIN_DEVICE_LISTER 1
#elif defined(_WIN32)
# include <winapifamily.h>
# include "filesystem/win32/DeviceLister.h"
# if WINAPI_FAMILY_PARTITION (WINAPI_PARTITION_DESKTOP)
#  define USE_BUILTIN_DEVICE_LISTER 1
# endif
#elif __APPLE__
# include <TargetConditionals.h>
# if !TARGET_OS_IPHONE
#  include "filesystem/darwin/DeviceLister.h"
#  define USE_BUILTIN_DEVICE_LISTER 1
# endif
#endif

medialibrary::DeviceListerPtr medialibrary::factory::createDeviceLister()
{
#ifdef USE_BUILTIN_DEVICE_LISTER
    return std::make_shared<fs::DeviceLister>();
#endif
    return nullptr;
}
