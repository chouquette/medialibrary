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

#include <string>
#include "IQuery.h"
#include "IMedia.h"
#include "IMediaLibrary.h"

namespace medialibrary
{

class IFolder
{
public:
    virtual ~IFolder() = default;
    virtual int64_t id() const = 0;
    /**
     * @brief mrl Returns the full mrl for this folder.
     * Caller is responsible for checking isPresent() beforehand, as we
     * can't compute an for a folder that is/was present on a removable storage
     * or network share that has been unplugged
     * If for some reasons we can't compute the MRL, an empty string wil
     * @return The folder's mrl
     */
    virtual const std::string& mrl() const = 0;
    virtual const std::string& name() const = 0;
    virtual bool isPresent() const = 0;
    /**
     * @brief isBanned Will return true if the folder was explicitely banned
     * from being discovered.
     */
    virtual bool isBanned() const = 0;
    /**
     * @brief media Returns the media contained by this folder.
     * @param type The media type, or IMedia::Type::Unknown for all types
     * @param params A query parameter instance, or nullptr for the default
     * @return A Query object to be used to fetch the results
     *
     * This function will only return the media contained in the folder, not
     * the media contained in subfolders.
     * A media is considered to be in a directory when the main file representing
     * it is part of the directory.
     * For instance, in this file hierarchy:
     * .
     * ├── a
     * │   ├── c
     * │   │   └── NakedMoleRat.asf
     * │   └── seaotter_themovie.srt
     * └── b
     *     └── seaotter_themovie.mkv
     * Media of 'a' would be empty (since the only file is a subtitle file and
     *                              not the actual media, and NakedMoleRat.asf
     *                              is in a subfolder)
     * Media of 'c' would contain NakedMoleRat.asf
     * Media of 'b' would contain seaotter_themovie.mkv
     */
    virtual Query<IMedia> media( IMedia::Type type,
                                  const QueryParameters* params ) const = 0;
    /**
     * @brief subfolders Returns the subfolders contained folder
     * @return A query object to be used to fetch the results
     *
     * all of the folder subfolders, regardless of the folder content.
     * For instance, in this hierarchy:
     * ├── a
     * │   └── w
     * │       └── x
     * a->subfolders() would return w; w->subfolders would return x, even though
     * x is empty.
     * This is done for optimization purposes, as keeping track of the entire
     * folder hierarchy would be quite heavy.
     * As an alternative, it is possible to use IMediaLibrary::folders to return
     * a flattened list of all folders that contain media.
     */
    virtual Query<IFolder> subfolders( const QueryParameters* params ) const = 0;
};

}
