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

class Media;
class File;

#include <memory>
#include <vector>

namespace parser
{

struct Task
{
    enum class Status
    {
        /// Default value.
        /// Also, having success = 0 is not the best idea ever.
        Unknown,
        /// All good
        Success,
        /// Something failed, but it's not critical (For instance, no internet connection for a
        /// module that uses an online database)
        Error,
        /// We can't compute this file for now (for instance the file was on a network drive which
        /// isn't connected anymore)
        TemporaryUnavailable,
        /// Something failed and we won't continue
        Fatal
    };

    Task( std::shared_ptr<Media> media, std::shared_ptr<File> file )
        : media( media )
        , file( file )
        , currentService( 0 )
        , trackNumber( 0 )
        , discNumber( 0 )
        , discTotal( 0 )
        , episode( 0 )
        , duration( 0 )
    {
    }

    struct VideoTrackInfo
    {
        VideoTrackInfo( const std::string& fcc, float fps, unsigned int width, unsigned int height )
            : fcc( fcc ), fps( fps ), width( width ), height( height )
        {
        }

        std::string fcc;
        float fps;
        unsigned int width;
        unsigned int height;
    };

    struct AudioTrackInfo
    {
        AudioTrackInfo( const std::string& fcc, unsigned int bitrate, unsigned int samplerate,
                        unsigned int nbChannels, const std::string& language, const std::string& description )
            : fcc( fcc ), bitrate( bitrate ), samplerate( samplerate ), nbChannels( nbChannels ),
              language( language ), description( description )
        {
        }
        std::string fcc;
        unsigned int bitrate;
        unsigned int samplerate;
        unsigned int nbChannels;
        std::string language;
        std::string description;
    };

    std::shared_ptr<Media>  media;
    std::shared_ptr<File>   file;
    unsigned int            currentService;

    std::vector<VideoTrackInfo> videoTracks;
    std::vector<AudioTrackInfo> audioTracks;

    std::string albumArtist;
    std::string artist;
    std::string albumName;
    std::string title;
    std::string artworkMrl;
    std::string genre;
    std::string releaseDate;
    std::string showName;
    int trackNumber;
    int discNumber;
    int discTotal;
    int episode;
    int64_t duration;
};

}
