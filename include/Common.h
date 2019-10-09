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

#ifdef __GNUC__
# define ML_FORCE_USED __attribute__ ((warn_unused_result))
#else
#define ML_FORCE_USED
#endif

#ifdef NDEBUG
# define ML_UNHANDLED_EXCEPTION_INIT try
# define ML_UNHANDLED_EXCEPTION_BODY(ctx) \
    catch ( const sqlite::errors::Exception& ex ) \
    { \
        if ( m_ml->getCb()->onUnhandledException( ctx, \
                                                  ex.what(), \
                                                  ex.requiresDbReset() ) == true ) \
            return; \
        throw; \
    } \
    catch ( const std::exception& ex ) \
    { \
        if ( m_ml->getCb()->onUnhandledException( ctx, \
                                                  ex.what(), false ) == true ) \
            return; \
        throw; \
    }
#else
# define ML_UNHANDLED_EXCEPTION_INIT
# define ML_UNHANDLED_EXCEPTION_BODY(ctx)
#endif
