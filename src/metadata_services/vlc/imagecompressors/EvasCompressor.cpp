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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "EvasCompressor.h"

#include <Evas_Engine_Buffer.h>

#include <cstring>

namespace medialibrary
{

EvasCompressor::EvasCompressor()
    : m_canvas( nullptr, &evas_free )
    , m_buffSize( 0 )
{
    static int fakeBuffer;
#ifndef TIZEN
    evas_init();
#endif
    auto method = evas_render_method_lookup("buffer");
    m_canvas.reset( evas_new() );
    if ( m_canvas == nullptr )
        throw std::runtime_error( "Failed to allocate canvas" );
    evas_output_method_set( m_canvas.get(), method );
    evas_output_size_set(m_canvas.get(), 1, 1 );
    evas_output_viewport_set( m_canvas.get(), 0, 0, 1, 1 );
    auto einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get( m_canvas.get() );
    einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
    einfo->info.dest_buffer = &fakeBuffer;
    einfo->info.dest_buffer_row_bytes = 4;
    einfo->info.use_color_key = 0;
    einfo->info.alpha_threshold = 0;
    einfo->info.func.new_update_region = NULL;
    einfo->info.func.free_update_region = NULL;
    evas_engine_info_set( m_canvas.get(), (Evas_Engine_Info *)einfo );
}

EvasCompressor::~EvasCompressor()
{
#if !defined(TIZEN)
    evas_shutdown();
#endif
}

const char* EvasCompressor::extension() const
{
    return "png";
}

const char* EvasCompressor::fourCC() const
{
    return "RV32";
}

uint32_t EvasCompressor::bpp() const
{
    return 4;
}

bool EvasCompressor::compress( const uint8_t* buffer, const std::string& output,
                               uint32_t inputWidth, uint32_t inputHeight,
                               uint32_t outputWidth, uint32_t outputHeight,
                               uint32_t hOffset, uint32_t vOffset )
{
    const auto stride = inputWidth * 4u;
    auto evas_obj = std::unique_ptr<Evas_Object, void(*)(Evas_Object*)>( evas_object_image_add( m_canvas.get() ), evas_object_del );
    if ( evas_obj == nullptr )
        return false;

    auto buffSize = outputWidth * outputHeight * bpp();
    if ( m_buffSize < buffSize )
    {
        m_cropBuffer.reset( new uint8_t[ buffSize ] );
        m_buffSize = buffSize;
    }

    auto p_buff = buffer;
    if ( outputWidth != inputWidth )
    {
        p_buff += vOffset * stride;
        for ( auto y = 0u; y < outputHeight; ++y )
        {
            memcpy( m_cropBuffer.get() + y * outputWidth * 4, p_buff + (hOffset * 4),
                    outputWidth * 4 );
            p_buff += stride;
        }
        vOffset = 0;
        p_buff = m_cropBuffer.get();
    }

    evas_object_image_colorspace_set( evas_obj.get(), EVAS_COLORSPACE_ARGB8888 );
    evas_object_image_size_set( evas_obj.get(), outputWidth, outputHeight );
    evas_object_image_data_set( evas_obj.get(), const_cast<uint8_t*>( p_buff + vOffset * stride ) );
    evas_object_image_save( evas_obj.get(), output.c_str(), NULL, "quality=100 compress=9");
    return true;
}

}
