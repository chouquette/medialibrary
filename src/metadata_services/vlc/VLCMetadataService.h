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
#include "MediaLibrary.h"

class VLCMetadataService : public IMetadataService
{
    struct Context
    {
        explicit Context(std::shared_ptr<Media> file_)
            : file( file_ )
        {
        }

        std::shared_ptr<Media> file;
        VLC::Media media;
    };

    public:
        explicit VLCMetadataService(const VLC::Instance& vlc, DBConnection dbConnection, std::shared_ptr<factory::IFileSystem> fsFactory);

        virtual bool initialize( IMetadataServiceCb *callback, MediaLibrary* ml ) override;
        virtual unsigned int priority() const override;
        virtual void run( std::shared_ptr<Media> file, void *data ) override;

private:
        Status handleMediaMeta( std::shared_ptr<Media> media , VLC::Media &vlcMedia ) const;
        std::shared_ptr<Album> findAlbum(Media* media, const std::string& title, Artist* albumArtist ) const;
        bool parseAudioFile( std::shared_ptr<Media> media, VLC::Media &vlcMedia ) const;
        bool parseVideoFile( std::shared_ptr<Media> file, VLC::Media &media ) const;
        std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> handleArtists( std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const;
        std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, std::shared_ptr<Media> media, VLC::Media& vlcMedia ) const;
        bool link(std::shared_ptr<Media> media, std::shared_ptr<Album> album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist ) const;
        std::shared_ptr<Album> handleAlbum( std::shared_ptr<Media> media, VLC::Media& vlcMedia, Artist* albumArtist ) const;

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        MediaLibrary* m_ml;
        std::vector<Context*> m_keepAlive;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        DBConnection m_dbConn;
        std::shared_ptr<factory::IFileSystem> m_fsFactory;
};

#endif // VLCMETADATASERVICE_H
