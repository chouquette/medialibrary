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

#include <condition_variable>

#include <vlcpp/vlc.hpp>

#if defined(WITH_EVAS)
#include <Evas.h>
#endif

#include "parser/ParserService.h"

#ifdef WITH_JPEG
#define BPP 3
#define VLC_FOURCC "RV24"
#elif defined(WITH_EVAS)
#define BPP 4
#define VLC_FOURCC "RV32"
#else
#error No compression strategy
#endif

class VLCThumbnailer : public ParserService
{
public:
    explicit VLCThumbnailer( const VLC::Instance& vlc );
    virtual ~VLCThumbnailer();
    virtual parser::Task::Status run( parser::Task& task ) override;
    virtual bool initialize() override;

private:
    parser::Task::Status startPlayback( VLC::MediaPlayer& mp );
    parser::Task::Status seekAhead( VLC::MediaPlayer &mp );
    void setupVout( VLC::MediaPlayer &mp );
    parser::Task::Status takeThumbnail( std::shared_ptr<Media> media, std::shared_ptr<File> file, VLC::MediaPlayer &mp );
    parser::Task::Status compress( std::shared_ptr<Media> media, std::shared_ptr<File> file );

    virtual const char* name() const;

private:
    // Force a base width, let height be computed depending on A/R
    static const uint32_t DesiredWidth = 320;
    static const uint32_t DesiredHeight = 200; // Aim for a 16:10 thumbnail
    static const uint8_t Bpp = BPP;

private:
    VLC::Instance m_instance;
    MediaLibrary* m_ml;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    // Per thumbnail variables
#ifdef WITH_EVAS
    std::unique_ptr<Evas, void(*)(Evas*)> m_canvas;
    std::unique_ptr<uint8_t[]> m_cropBuffer;
#endif
    std::unique_ptr<uint8_t[]> m_buff;
    std::atomic_bool m_thumbnailRequired;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_prevSize;
};
