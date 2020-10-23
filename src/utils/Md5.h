/*****************************************************************************
 * Md5.h: Hash functions
 *****************************************************************************
 * Copyright © 2004-2020 VLC authors and VideoLAN
 *
 * Authors: Rémi Denis-Courmont
 *          Rafaël Carré
 *          Marvin Scholz
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

#include <cstddef>
#include <cstdint>
#include <string>

namespace medialibrary
{
namespace utils
{
class Md5Hasher
{
public:
    Md5Hasher();
    void update( const uint8_t* buff, size_t size );
    std::string finalize();

    static std::string fromBuff( const uint8_t* buff, size_t size );
    static std::string fromFile( const std::string& path );

private:
    void transform( const unsigned char* data );
    void write( const uint8_t *inbuf_arg , size_t inlen );
    void final();

    static std::string toString( const uint8_t* buff, size_t size );
    static constexpr size_t HashDigestSize = 16;

private:
    uint32_t m_A;
    uint32_t m_B;
    uint32_t m_C;
    uint32_t m_D;
    uint32_t m_nblocks;
    uint8_t m_buf[64];
    int m_count;
};

}

}

