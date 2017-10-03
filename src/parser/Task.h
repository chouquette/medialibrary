/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *          Alexandre Fernandez <nerf@boboop.fr>
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

#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <vlcpp/vlc.hpp>

namespace medialibrary
{

namespace fs
{
class IDirectory;
class IFile;
}

class Media;
class File;
class Folder;

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
        /// We can't compute this file for now (for instance the file was on a network drive which
        /// isn't connected anymore)
        TemporaryUnavailable,
        /// Something failed and we won't continue
        Fatal
    };

    enum class ParserStep : uint8_t
    {
        None = 0,
        MetadataExtraction = 1,
        MetadataAnalysis = 2,
        Thumbnailer = 4,

        Completed = 1 | 2 | 4,
    };

    Task( std::shared_ptr<Media> media, std::shared_ptr<File> file );
    Task( std::shared_ptr<fs::IFile> fileFs,
          std::shared_ptr<Folder> parentFolder,
          std::shared_ptr<fs::IDirectory> parentFolderFs );

    void markStepCompleted( ParserStep stepCompleted );
    void markStepUncompleted( ParserStep stepUncompleted );

    std::shared_ptr<Media>          media;
    std::shared_ptr<File>           file;
    std::shared_ptr<fs::IFile>      fileFs;
    std::shared_ptr<Folder>         parentFolder;
    std::shared_ptr<fs::IDirectory> parentFolderFs;
    std::string                     mrl;
    VLC::Media                      vlcMedia;
    unsigned int                    currentService;
    ParserStep                      step;
};

}

}
