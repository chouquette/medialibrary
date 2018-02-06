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

#include "compat/ConditionVariable.h"

#include <vlcpp/vlc.hpp>

#include "imagecompressors/IImageCompressor.h"
#include "parser/ParserService.h"

namespace medialibrary
{

class VLCThumbnailer
{
    struct Task
    {
        Task( MediaPtr media );

        compat::Mutex mutex;
        compat::ConditionVariable cond;
        MediaPtr media;
        std::string mrl;
        uint32_t width;
        uint32_t height;
        VLC::MediaPlayer mp;
        std::atomic_bool thumbnailRequired;
    };

public:
    explicit VLCThumbnailer( MediaLibraryPtr ml );
    virtual ~VLCThumbnailer();
    void requestThumbnail( MediaPtr media );

private:
    void run();
    void stop();

    bool generateThumbnail( Task& task );
    bool seekAhead( Task& task );
    void setupVout( Task& task );
    bool takeThumbnail( Task& task );
    bool compress( Task& task );

private:
    // Force a base width, let height be computed depending on A/R
    static const uint32_t DesiredWidth = 320;
    static const uint32_t DesiredHeight = 200; // Aim for a 16:10 thumbnail

private:
    MediaLibraryPtr m_ml;
    compat::Mutex m_mutex;
    compat::ConditionVariable m_cond;
    std::queue<std::unique_ptr<Task>> m_tasks;
    std::atomic_bool m_run;
    compat::Thread m_thread;

    std::unique_ptr<IImageCompressor> m_compressor;
    std::unique_ptr<uint8_t[]> m_buff;
    uint32_t m_prevSize;
};

}
