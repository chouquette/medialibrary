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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "JpegCompressor.h"

#include "logging/Logger.h"

#include <jpeglib.h>

#include <cerrno>
#include <cstring>
#include <memory>
#include <setjmp.h>

namespace medialibrary
{

const char* JpegCompressor::fourCC() const
{
    return "RV24";
}

uint32_t JpegCompressor::bpp() const
{
    return 3;
}

struct jpegError : public jpeg_error_mgr
{
    jmp_buf buff;
    char message[JMSG_LENGTH_MAX];

    static void jpegErrorHandler( j_common_ptr common )
    {
        auto error = reinterpret_cast<jpegError*>( common->err );
        (*error->format_message)( common, error->message );
        longjmp( error->buff, 1 );
    }
};

bool JpegCompressor::compress( const uint8_t* buffer, const std::string& outputFile,
                              uint32_t inputWidth, uint32_t,
                              uint32_t outputWidth, uint32_t outputHeight,
                              uint32_t hOffset, uint32_t vOffset )
{
    const auto stride = inputWidth * bpp();

    //FIXME: Abstract this away, though libjpeg requires a FILE*...
    auto fOut = std::unique_ptr<FILE, int(*)(FILE*)>( fopen( outputFile.c_str(), "wb" ), &fclose );
    if ( fOut == nullptr )
    {
        LOG_ERROR( "Failed to open thumbnail file ", outputFile, '(', strerror( errno ), ')' );
        return false;
    }

    jpeg_compress_struct compInfo;
    JSAMPROW row_pointer[1];

    //libjpeg's default error handling is to call exit(), which would
    //be slightly problematic...
    jpegError err;
    compInfo.err = jpeg_std_error(&err);
    err.error_exit = jpegError::jpegErrorHandler;

    if ( setjmp( err.buff ) )
    {
        LOG_ERROR("JPEG failure: ", err.message);
        jpeg_destroy_compress(&compInfo);
        return false;
    }

    jpeg_create_compress(&compInfo);
    jpeg_stdio_dest(&compInfo, fOut.get());

    compInfo.image_width = outputWidth;
    compInfo.image_height = outputHeight;
    compInfo.input_components = bpp();
    compInfo.in_color_space = JCS_RGB;
    jpeg_set_defaults( &compInfo );
    jpeg_set_quality( &compInfo, 80, TRUE );

    jpeg_start_compress( &compInfo, TRUE );

    while ( compInfo.next_scanline < outputHeight )
    {
        row_pointer[0] = const_cast<uint8_t*>( &buffer[(compInfo.next_scanline + vOffset ) * stride + hOffset * bpp()] );
        jpeg_write_scanlines(&compInfo, row_pointer, 1);
    }
    jpeg_finish_compress(&compInfo);
    jpeg_destroy_compress(&compInfo);
    return true;
}

}
