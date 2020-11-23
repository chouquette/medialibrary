/*****************************************************************************
 * md5.c: not so strong MD5 hashing
 *****************************************************************************
 * Copyright (C) 1995,1996,1998,1999,2001,2002,
 *               2003  Free Software Foundation, Inc.
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

/* md5.c - MD5 Message-Digest Algorithm
 *
 * According to the definition of MD5 in RFC 1321 from April 1992.
 * NOTE: This is *not* the same file as the one from glibc.
 * Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.
 * heavily modified for GnuPG by Werner Koch <wk@gnupg.org>
 */

/* Test values:
 * ""                  D4 1D 8C D9 8F 00 B2 04  E9 80 09 98 EC F8 42 7E
 * "a"                 0C C1 75 B9 C0 F1 B6 A8  31 C3 99 E2 69 77 26 61
 * "abc                90 01 50 98 3C D2 4F B0  D6 96 3F 7D 28 E1 7F 72
 * "message digest"    F9 6B 69 7D 7C B7 93 8D  52 5A 2F 31 AA F1 61 D0
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <memory>

#include "Md5.h"

#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )

/* These are the four functions used in the four steps of the MD5 algorithm
   and defined in the RFC 1321.  The first function is a little bit optimized
   (as found in Colin Plumbs public domain implementation).  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF (d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))

namespace medialibrary
{
namespace utils
{

Md5Hasher::Md5Hasher()
    : m_A( 0x67452301 )
    , m_B( 0xefcdab89 )
    , m_C( 0x98badcfe )
    , m_D( 0x10325476 )
    , m_nblocks( 0 )
    , m_count( 0 )
{
}

void Md5Hasher::update( const uint8_t* buff, size_t size )
{
    write( buff, size );
}

std::string Md5Hasher::finalize()
{
    final();
    return toString( m_buf, HashDigestSize );
}

std::string Md5Hasher::fromBuff( const uint8_t* buff, size_t size )
{
    Md5Hasher hasher{};

    hasher.update( buff, size );

    return hasher.finalize();
}

std::string Md5Hasher::fromFile( const std::string& path )
{
    std::unique_ptr<FILE, decltype(&fclose)> file{
        fopen( path.c_str(), "rb" ), &fclose
    };
    uint8_t buff[ 64 * 64 ];
    Md5Hasher hasher{};
    while ( feof( file.get() ) == false )
    {
        auto read = fread( buff, 1, sizeof( buff ), file.get() );
        hasher.update( buff, read );
    }
    return hasher.finalize();
}

void Md5Hasher::transform( const unsigned char* data )
{
    uint32_t correct_words[16];
    uint32_t A = m_A;
    uint32_t B = m_B;
    uint32_t C = m_C;
    uint32_t D = m_D;
    uint32_t *cwp = correct_words;

  #ifdef WORDS_BIGENDIAN
    {
      int i;
      uint8_t *p2, *p1;
      for(i=0, p1=data, p2=(uint8_t*)correct_words; i < 16; i++, p2 += 4 )
        {
          p2[3] = *p1++;
      p2[2] = *p1++;
      p2[1] = *p1++;
      p2[0] = *p1++;
        }
    }
  #else
    memcpy( correct_words, data, 64 );
  #endif


  #define OP(a, b, c, d, s, T) \
    do			         	   \
      {					   \
        a += FF (b, c, d) + (*cwp++) + T;    \
        a = rol(a, s);			   \
        a += b;				   \
      }					   \
    while (0)

    /* Before we start, one word about the strange constants.
       They are defined in RFC 1321 as

       T[i] = (int) (4294967296.0 * fabs (sin (i))), i=1..64
    */

    /* Round 1.  */
    OP (A, B, C, D,  7, 0xd76aa478);
    OP (D, A, B, C, 12, 0xe8c7b756);
    OP (C, D, A, B, 17, 0x242070db);
    OP (B, C, D, A, 22, 0xc1bdceee);
    OP (A, B, C, D,  7, 0xf57c0faf);
    OP (D, A, B, C, 12, 0x4787c62a);
    OP (C, D, A, B, 17, 0xa8304613);
    OP (B, C, D, A, 22, 0xfd469501);
    OP (A, B, C, D,  7, 0x698098d8);
    OP (D, A, B, C, 12, 0x8b44f7af);
    OP (C, D, A, B, 17, 0xffff5bb1);
    OP (B, C, D, A, 22, 0x895cd7be);
    OP (A, B, C, D,  7, 0x6b901122);
    OP (D, A, B, C, 12, 0xfd987193);
    OP (C, D, A, B, 17, 0xa679438e);
    OP (B, C, D, A, 22, 0x49b40821);

  #undef OP
  #define OP(f, a, b, c, d, k, s, T)  \
      do								      \
        { 							      \
      a += f (b, c, d) + correct_words[k] + T;		      \
      a = rol(a, s);						      \
      a += b; 						      \
        } 							      \
      while (0)

    /* Round 2.  */
    OP (FG, A, B, C, D,  1,  5, 0xf61e2562);
    OP (FG, D, A, B, C,  6,  9, 0xc040b340);
    OP (FG, C, D, A, B, 11, 14, 0x265e5a51);
    OP (FG, B, C, D, A,  0, 20, 0xe9b6c7aa);
    OP (FG, A, B, C, D,  5,  5, 0xd62f105d);
    OP (FG, D, A, B, C, 10,  9, 0x02441453);
    OP (FG, C, D, A, B, 15, 14, 0xd8a1e681);
    OP (FG, B, C, D, A,  4, 20, 0xe7d3fbc8);
    OP (FG, A, B, C, D,  9,  5, 0x21e1cde6);
    OP (FG, D, A, B, C, 14,  9, 0xc33707d6);
    OP (FG, C, D, A, B,  3, 14, 0xf4d50d87);
    OP (FG, B, C, D, A,  8, 20, 0x455a14ed);
    OP (FG, A, B, C, D, 13,  5, 0xa9e3e905);
    OP (FG, D, A, B, C,  2,  9, 0xfcefa3f8);
    OP (FG, C, D, A, B,  7, 14, 0x676f02d9);
    OP (FG, B, C, D, A, 12, 20, 0x8d2a4c8a);

    /* Round 3.  */
    OP (FH, A, B, C, D,  5,  4, 0xfffa3942);
    OP (FH, D, A, B, C,  8, 11, 0x8771f681);
    OP (FH, C, D, A, B, 11, 16, 0x6d9d6122);
    OP (FH, B, C, D, A, 14, 23, 0xfde5380c);
    OP (FH, A, B, C, D,  1,  4, 0xa4beea44);
    OP (FH, D, A, B, C,  4, 11, 0x4bdecfa9);
    OP (FH, C, D, A, B,  7, 16, 0xf6bb4b60);
    OP (FH, B, C, D, A, 10, 23, 0xbebfbc70);
    OP (FH, A, B, C, D, 13,  4, 0x289b7ec6);
    OP (FH, D, A, B, C,  0, 11, 0xeaa127fa);
    OP (FH, C, D, A, B,  3, 16, 0xd4ef3085);
    OP (FH, B, C, D, A,  6, 23, 0x04881d05);
    OP (FH, A, B, C, D,  9,  4, 0xd9d4d039);
    OP (FH, D, A, B, C, 12, 11, 0xe6db99e5);
    OP (FH, C, D, A, B, 15, 16, 0x1fa27cf8);
    OP (FH, B, C, D, A,  2, 23, 0xc4ac5665);

    /* Round 4.  */
    OP (FI, A, B, C, D,  0,  6, 0xf4292244);
    OP (FI, D, A, B, C,  7, 10, 0x432aff97);
    OP (FI, C, D, A, B, 14, 15, 0xab9423a7);
    OP (FI, B, C, D, A,  5, 21, 0xfc93a039);
    OP (FI, A, B, C, D, 12,  6, 0x655b59c3);
    OP (FI, D, A, B, C,  3, 10, 0x8f0ccc92);
    OP (FI, C, D, A, B, 10, 15, 0xffeff47d);
    OP (FI, B, C, D, A,  1, 21, 0x85845dd1);
    OP (FI, A, B, C, D,  8,  6, 0x6fa87e4f);
    OP (FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
    OP (FI, C, D, A, B,  6, 15, 0xa3014314);
    OP (FI, B, C, D, A, 13, 21, 0x4e0811a1);
    OP (FI, A, B, C, D,  4,  6, 0xf7537e82);
    OP (FI, D, A, B, C, 11, 10, 0xbd3af235);
    OP (FI, C, D, A, B,  2, 15, 0x2ad7d2bb);
    OP (FI, B, C, D, A,  9, 21, 0xeb86d391);

    /* Put checksum in context given as argument.  */
    m_A += A;
    m_B += B;
    m_C += C;
    m_D += D;
}

/* The routine updates the message-digest context to
 * account for the presence of each of the characters inBuf[0..inLen-1]
 * in the message whose digest is being computed.
 */
void Md5Hasher::write( const uint8_t* inbuf, size_t inlen )
{
    if( m_count == 64 )  /* flush the buffer */
    {
        transform( m_buf );
        m_count = 0;
        m_nblocks++;
    }
    if( !inbuf )
        return;

    if( m_count )
    {
        for( ; inlen && m_count < 64; inlen-- )
            m_buf[m_count++] = *inbuf++;
        write( nullptr, 0 );
        if( !inlen )
            return;
    }

    while( inlen >= 64 )
    {
        transform( inbuf );
        m_count = 0;
        m_nblocks++;
        inlen -= 64;
        inbuf += 64;
    }
    for( ; inlen && m_count < 64; inlen-- )
        m_buf[m_count++] = *inbuf++;
}

/* The routine final terminates the message-digest computation and
 * ends with the desired message digest in mdContext->digest[0...15].
 * The handle is prepared for a new MD5 cycle.
 * Returns 16 bytes representing the digest.
 */
void Md5Hasher::final()
{
    uint32_t t, msb, lsb;
    uint8_t *p;

    write(nullptr, 0); /* flush */;

    t = m_nblocks;
    /* multiply by 64 to make a byte count */
    lsb = t << 6;
    msb = t >> 26;
    /* add the count */
    t = lsb;
    if( (lsb += m_count) < t )
        msb++;
    /* multiply by 8 to make a bit count */
    t = lsb;
    lsb <<= 3;
    msb <<= 3;
    msb |= t >> 29;

    if( m_count < 56 )  /* enough room */
    {
        m_buf[m_count++] = 0x80; /* pad */
        while( m_count < 56 )
            m_buf[m_count++] = 0;  /* pad */
    }
    else  /* need one extra block */
    {
        m_buf[m_count++] = 0x80; /* pad character */
        while( m_count < 64 )
            m_buf[m_count++] = 0;
        write( nullptr, 0 );  /* flush */;
        memset(m_buf, 0, 56 ); /* fill next block with zeroes */
    }
    /* append the 64 bit count */
    m_buf[56] = lsb	   ;
    m_buf[57] = lsb >>  8;
    m_buf[58] = lsb >> 16;
    m_buf[59] = lsb >> 24;
    m_buf[60] = msb	   ;
    m_buf[61] = msb >>  8;
    m_buf[62] = msb >> 16;
    m_buf[63] = msb >> 24;
    transform( m_buf );

    p = m_buf;

#define X(VAR) do { memcpy( p, &VAR, 4 ); p += 4; } while(0)
    X(m_A);
    X(m_B);
    X(m_C);
    X(m_D);
#undef X
}

std::string Md5Hasher::toString( const uint8_t* buff, size_t size )
{
    assert( size == HashDigestSize );

    static const char hexchars[] = { "0123456789ABCDEF" };

    std::string res;
    res.reserve( 32 );
    for ( auto i = 0u; i < HashDigestSize; ++i )
        res.append( { hexchars[buff[i] >> 4], hexchars[buff[i] & 0xF] } );
    return res;
}

}
}
