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

#include "Types.h"
#include "IQuery.h"

namespace medialibrary
{

struct QueryParameters;

class IPlaylist
{
public:
    virtual ~IPlaylist() = default;
    virtual int64_t id() const = 0;
    virtual const std::string& name() const = 0;
    virtual bool setName( const std::string& name ) = 0;
    virtual unsigned int creationDate() const = 0;
    virtual const std::string& artworkMrl() const = 0;
    virtual Query<IMedia> media() const = 0;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const = 0;
    ///
    /// \brief append Appends a media to a playlist
    /// The media will be the last element of a subsequent call to media()
    /// This is equivalent to calling add( media, 0 )
    /// \param media The media to add
    /// \return true on success, false on failure.
    ///
    virtual bool append( const IMedia& media ) = 0;
    ///
    /// \brief add Add a media to the playlist at the given position.
    /// Valid positions start at 1. 0 means appending.
    /// \param media The media to add
    /// \param position The position of this new media
    /// \return true on success, false on failure
    ///
    virtual bool add( const IMedia& media, unsigned int position ) = 0;

    /// Convenience wrappers
    virtual bool append( int64_t mediaId ) = 0;
    virtual bool add( const int64_t mediaId, unsigned int position ) = 0;

    ///
    /// \brief move Change the position of a media
    /// \param mediaId The media to move reorder
    /// \param position The new position within the playlist.
    /// 0 is an invalid value when moving.
    /// In case there is already a media at the given position, it will be placed after
    /// the media being moved. This will cascade to any media placed afterward.
    /// For instance, a playlist with <media,position> like
    /// [<1,1>, <2,2>, <3,3>] on which move(1, 2) is called will result in the playlist
    /// being changed to
    /// [<1,2>, <2,3>, <3,4>]
    /// \return true on success, false on failure
    ///
    virtual bool move( int64_t mediaId, unsigned int position ) = 0;
    ///
    /// \brief remove Removes a media from the playlist
    /// \param mediaId The media to remove.
    /// \return true on success, false on failure
    ///
    virtual bool remove( int64_t mediaId ) = 0;
};

}
