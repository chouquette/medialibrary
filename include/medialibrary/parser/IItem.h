 /*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include <string>
#include <vector>
#include <memory>

#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/Types.h"
#include "medialibrary/IFile.h"

namespace medialibrary
{

namespace parser
{

class IEmbeddedThumbnail
{
public:
    virtual ~IEmbeddedThumbnail() = default;
    virtual bool save( const std::string& path ) = 0;
    virtual size_t size() const = 0;
    virtual std::string hash() const = 0;
    virtual std::string extension() const = 0;
};

class IItem
{
public:
    enum class LinkType : uint8_t
    {
        NoLink,
        Playlist,
        Media,
    };

    enum class Metadata : uint8_t
    {
        Title,
        ArtworkUrl,
        ShowName,
        Episode,
        Album,
        Genre,
        Date,
        AlbumArtist,
        Artist,
        TrackNumber,
        DiscNumber,
        DiscTotal,

        /* for convenience, please keep this last */
        NbValues,
    };

    struct Track
    {
        Track() = default;

        enum class Type : uint8_t
        {
            Video,
            Audio,
            Subtitle,
        };

        std::string codec;
        Type type;
        uint32_t bitrate;
        std::string language;
        std::string description;
        // Audio
        union
        {
            struct
            {
                uint32_t nbChannels;
                uint32_t rate;
            } a;
            struct
            {
                // Video
                uint32_t height;
                uint32_t width;
                uint32_t sarNum;
                uint32_t sarDen;
                uint32_t fpsNum;
                uint32_t fpsDen;
            } v;
            struct
            {
                char encoding[sizeof(v)];
            } s;
        } u;
    };

public:
    virtual ~IItem() = default;

    /**
     * @brief meta Returns a stored meta for this item.
     * @param type The type of meta
     * @return A copy of the meta. It can be safely moved to another std::string
     * if required.
     * A default constructed string if the meta isn't known for this item
     */
    virtual std::string meta( Metadata type ) const = 0;
    /**
     * @brief setMeta Store a meta for the item
     * @param type The metadata type
     * @param value The metadata value
     */
    virtual void setMeta( Metadata type, std::string value ) = 0;

    /**
     * @return The MRL representing this item.
     */
    virtual const std::string& mrl() const = 0;

    /**
     * @brief fileType Returns the analyzed file type
     */
    virtual IFile::Type fileType() const = 0;

    /**
     * @return The number of linked items for this item.
     */
    virtual size_t nbLinkedItems() const = 0;
    /**
     * @return The linked item at the given index.
     * The linked items are owned by their parent item. No bound checking is
     * performed.
     */
    virtual const IItem& linkedItem( unsigned int index ) const = 0;
    /**
     * @brief createLinkedItem Create a linked item in place.
     * @param mrl The item's MRL
     * @param itemType The linked item type
     * @param linkExtra Some additional information about the item being linked
     * @return A *reference* to the created item, so the item can be populated
     *         after its creation.
     */
    virtual IItem& createLinkedItem( std::string mrl, IFile::Type itemType,
                                     int64_t linkExtra ) = 0;

    /**
     * @brief duration Returns the item duration in milliseconds
     */
    virtual int64_t duration() const = 0;
    /**
     * @brief setDuration Sets the item duration
     * @param duration A duration in milliseconds
     */
    virtual void setDuration( int64_t duration ) = 0;

    /**
     * @brief tracks List all the item tracks
     *
     * This only contains the Audio & Video tracks
     */
    virtual const std::vector<Track>& tracks() const = 0;
    /**
     * @brief addTrack Add a track to this item.
     * @param t The track to add.
     */
    virtual void addTrack( Track&& t ) = 0;

    /**
     * @brief media Returns the media associated with this item, if any.
     * @return nullptr if the item isn't associated with a Media yet.
     */
    virtual MediaPtr media() = 0;
    /**
     * @brief setMedia Assigns a Media to this item
     */
    virtual void setMedia( MediaPtr media ) = 0;

    /**
     * @brief file Returns the File (as in, the database entity) associated with
     *             this item, if any. It returns nullptr otherwise
     */
    virtual FilePtr file() = 0;

    /**
     * @brief fileId Returns the ID of the file associated with this item, if any.
     *
     * If no file was created for this item yet, 0 is returned.
     */
    virtual int64_t fileId() const = 0;

    /**
     * @brief setFile Assigns a File to the item
     */
    virtual bool setFile( FilePtr file ) = 0;

    /**
     * @brief parentFolder Returns the Folder (as in the database entity)
     *                     containing this item.
     *
     * @return This can be nullptr if the item refers to an "external" media, ie.
     *         if it was added through its complete MRL, instead of being
     *         discovered through its parent folder.
     */
    virtual FolderPtr parentFolder() = 0;

    /**
     * @brief fileFs returns an fs::IFile representing the item
     *
     * This will return nullptr for external media.
     */
    virtual std::shared_ptr<fs::IFile> fileFs() const = 0;

    /**
     * @brief parentFolderFs Returns an fs::IDirectory representing the parent
     *                       folder
     *
     * This will return nullptr for external media
     */
    virtual std::shared_ptr<fs::IDirectory> parentFolderFs() = 0;

    virtual bool isRefresh() const = 0;
    virtual bool isRestore() const = 0;

    virtual LinkType linkType() const = 0;
    virtual int64_t linkToId() const = 0;
    virtual int64_t linkExtra() const = 0;
    virtual const std::string& linkToMrl() const = 0;
    virtual const std::vector<std::shared_ptr<IEmbeddedThumbnail>>&
        embeddedThumbnails() const = 0;
    virtual void addEmbeddedThumbnail( std::shared_ptr<IEmbeddedThumbnail> t ) = 0;
};

}
}
