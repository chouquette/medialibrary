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

#ifndef IFILE_H
#define IFILE_H

#include <vector>
#include <memory>

#include "IMediaLibrary.h"

class IAlbumTrack;
class IShowEpisode;
class ITrackInformation;

class IMedia
{
    public:
        enum class Type : uint8_t
        {
            VideoType, // Any video file, not being a tv show episode
            AudioType, // Any kind of audio file, not being an album track
            UnknownType
        };
        virtual ~IMedia() {}

        virtual unsigned int id() const = 0;
        virtual Type type() = 0;
        virtual const std::string& title() = 0;
        virtual AlbumTrackPtr albumTrack() = 0;
        /**
         * @brief duration Returns the file duration in ms
         */
        virtual int64_t duration() const = 0;
        virtual std::shared_ptr<IShowEpisode> showEpisode() = 0;
        virtual int playCount() const = 0;
        virtual void increasePlayCount() = 0;
        virtual const std::string& mrl() const = 0;
        virtual bool addLabel( LabelPtr label ) = 0;
        virtual bool removeLabel( LabelPtr label ) = 0;
        virtual MoviePtr movie() = 0;
        virtual const std::string& artist() const = 0;
        virtual std::vector<LabelPtr> labels() = 0;
        virtual std::vector<VideoTrackPtr> videoTracks() = 0;
        virtual std::vector<AudioTrackPtr> audioTracks() = 0;
        // Returns the location of this file snapshot.
        // This is likely to be used for album arts as well.
        virtual const std::string& snapshot() = 0;
        virtual unsigned int insertionDate() const = 0;
        virtual bool isAvailable() const = 0;
};

#endif // IFILE_H
