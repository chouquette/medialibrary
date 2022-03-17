/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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
    ///
    /// \brief id Returns the playlist id
    ///
    virtual int64_t id() const = 0;
    ///
    /// \brief name Returns the playlist name
    ///
    virtual const std::string& name() const = 0;
    ///
    /// \brief setName Updates the playlist name
    ///
    virtual bool setName( const std::string& name ) = 0;
    ///
    /// \brief creationDate Returns the playlist creation date.
    ///
    /// For playlist that were analyzed based on a playlist file (as opposed to
    /// created by the application) this will be the date when the playlist was
    /// first discovered, not the playlist *file* creation/last modification date
    ///
    virtual unsigned int creationDate() const = 0;
    ///
    /// \brief artworkMrl An artwork for this playlist, if any.
    /// \return An artwork, or an empty string if none is available.
    ///
    virtual const std::string& artworkMrl() const = 0;
    ///
    /// \brief nbMedia Return the number of media in this playlist
    ///
    /// This number doesn't reflect media presence. For the count of present
    /// media, use nbPresentMedia
    ///
    virtual uint32_t nbMedia() const = 0;
    virtual uint32_t nbVideo() const = 0;
    virtual uint32_t nbAudio() const = 0;
    virtual uint32_t nbUnknown() const = 0;
    ///
    /// \brief nbPresentMedia Returns the number of present media in this playlist
    ///
    virtual uint32_t nbPresentMedia() const = 0;
    virtual uint32_t nbPresentVideo() const = 0;
    virtual uint32_t nbPresentAudio() const = 0;
    virtual uint32_t nbPresentUnknown() const = 0;
    ///
    /// \brief duration Returns the duration of the playlist
    ///
    /// This is equal to the sum of the duration for all media belonging to
    /// that playlist.
    /// Some media duration may not be known by the media library, in which case
    /// \sa{nbDurationUnknown} will return a non-zero value.
    ///
    virtual int64_t duration() const = 0;
    ///
    /// \brief nbDurationUnknown Returns the number of media for which the duration
    /// is unknown
    ///
    ///
    virtual uint32_t nbDurationUnknown() const = 0;
    ///
    /// \brief media Returns the media contained in this playlist
    /// \param params A QueryParameters object or nullptr for the default params
    /// \return A query object representing the media in this playlist
    ///
    /// The media will always be sorted by their ascending position in the
    /// playlist, meaning QueryParameters::sort & QueryParameters::desc will be
    /// ignored.
    /// QueryParameters::includeMissing will be used however, so the called can
    /// get the missing media for the playlist
    ///
    virtual Query<IMedia> media( const QueryParameters* params ) const = 0;
    ///
    /// \brief searchMedia Search some media in a playlist
    /// \param pattern The search pattern. Minimal length is 3 characters
    /// \param params Some query parameters. \see{IMediaLibrary::searchMedia}
    /// \return A query object, or nullptr in case of error or if the pattern is too short
    ///
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params = nullptr ) const = 0;
    ///
    /// \brief append Appends a media to a playlist
    /// The media will be the last element of a subsequent call to media()
    /// \param media The media to add
    /// \return true on success, false on failure.
    ///
    virtual bool append( const IMedia& media ) = 0;
    ///
    /// \brief add Add a media to the playlist at the given position.
    /// \param media The media to add
    /// \param position The position of this new media, in the [0;size-1] range
    /// \return true on success, false on failure
    ///
    /// If the position is greater than the playlist size, it will be interpreted
    /// as a regular append operation, and the item position will be set to
    /// <playlist size>
    /// For instance, on the playlist [<B,0>, <A,1>, <C,2>], if add(D, 999)
    /// gets called, the resulting playlist will be [<A,0>, <C,1>, <B,2>, <D,3>]
    ///
    virtual bool add( const IMedia& media, uint32_t position ) = 0;

    /// Convenience wrappers
    virtual bool append( int64_t mediaId ) = 0;
    virtual bool add( int64_t mediaId, uint32_t position ) = 0;

    ///
    /// \brief move Change the position of a media
    /// \param from The position of the item being moved
    /// \param to The moved item target position
    ///
    /// \return true on success, false on failure
    ///
    /// In case there is already an item at the given position, it will be placed before
    /// the media being moved. This will cascade to any media placed afterward.
    /// For instance, a playlist with <media,position> like
    /// [<A,0>, <B,1>, <C,2>] on which move(0, 1) is called will result in the
    /// playlist being changed to
    /// [<B,0>, <A,1>, <C,2>]
    /// If the target position is out of range (ie greater than the playlist size)
    /// the target position will be interpreted as the playlist size (prior to insertion).
    /// For instance, on the playlist [<B,0>, <A,1>, <C,2>], if move(0, 999)
    /// gets called, the resulting playlist will be [<A,0>, <C,1>, <B,2>]
    ///
    virtual bool move( uint32_t from, uint32_t to ) = 0;
    ///
    /// \brief remove Removes an item from the playlist
    /// \param position The position of the item to remove.
    /// \return true on success, false on failure
    ///
    virtual bool remove( uint32_t position ) = 0;
    ///
    /// \brief isReadOnly Return true if the playlist is backed by an actual file
    ///                   and should therefor not modified directly.
    /// \return true if the playlist should be considered read-only, false otherwise
    ///
    /// If the application doesn't respect this, the medialibrary will, for
    /// now, accept the changes, but if the playlist file changes, any user
    /// provided changes will be discarded without warning.
    ///
    virtual bool isReadOnly() const = 0;
    ///
    /// \brief mrl Return the file backing this playlist.
    ///
    /// This must be called only when isReadOnly() returns true, as a modifiable
    /// playlist has no file associated with it.
    /// In case of errors, an empty string will be returned.
    ///
    virtual std::string mrl() const = 0;
};

}
