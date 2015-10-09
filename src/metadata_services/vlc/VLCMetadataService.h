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

#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include <condition_variable>
#include <vlcpp/vlc.hpp>
#include <mutex>

#include "IMetadataService.h"

class VLCMetadataService : public IMetadataService
{
    struct Context
    {
        Context(FilePtr file_)
            : file( file_ )
        {
        }

        FilePtr file;
        VLC::Media media;
    };

    public:
        VLCMetadataService(const VLC::Instance& vlc);

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        ServiceStatus handleMediaMeta(FilePtr file , VLC::Media &media) const;
        bool parseAudioFile(FilePtr file , VLC::Media &media) const;
        bool parseVideoFile(FilePtr file , VLC::Media &media) const;

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
        std::vector<Context*> m_keepAlive;
        std::mutex m_mutex;
        std::condition_variable m_cond;
};

#endif // VLCMETADATASERVICE_H
