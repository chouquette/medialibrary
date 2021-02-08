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

#include "database/DatabaseHelpers.h"

namespace medialibrary
{

namespace parser
{
class IEmbeddedThumbnail;
}

class Thumbnail : public DatabaseHelpers<Thumbnail>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Thumbnail::*const PrimaryKey;
    };
    struct LinkingTable
    {
        static const std::string Name;
    };
    enum class Triggers : uint8_t
    {
        AutoDeleteAlbum,
        AutoDeleteArtist,
        AutoDeleteMedia,
        IncrementRefcount,
        DecrementRefcount,
        UpdateRefcount,
        DeleteUnused,

        // Deprecated since v18
        DeleteAfterLinkingDelete,
    };
    enum class Indexes : uint8_t
    {
        ThumbnailId,
    };

    enum class Origin : uint8_t
    {
        /// The thumbnail comes from a media that was tagged using Artist.
        /// This means the thumbnail may be from a compilation album
        Artist,
        /// The thumbnail comes from a media that was tagged using AlbumArtist.
        /// This means the artist is either a various artist, or the main artist
        /// of an album.
        /// AlbumArtist has a higher priority than Artist when selecting a thumbnail
        AlbumArtist,
        /// An artwork that was attached to the media, or a generated video
        /// thumbnail
        Media,
        /// A thumbnail provided by the application
        UserProvided,
        /// An image (jpg or png) that was located in the album folder
        CoverFile,
    };

    enum class EntityType
    {
        Media,
        Album,
        Artist,
        Genre,
    };

    /**
     * This is a callback prototype intended to chose between updating an
     * existing thumbnail, or inserting a new one and leaving the previous one
     * untouched.
     * If the old thumbnail should just be updated (meaning that all other
     * entities using that thumbnail will start using the new one, as the
     * thumbnail they link to will be updated), this must return true.
     * If the user wishes to keep the previous thumbnail for other entities
     * linking to it, they should return false. In which case, the new thumbnail
     * will be inserted if required, and a new linking record will be inserted
     * to link the new thumbnail with the updated entity
     */
    using ShouldUpdateCb = bool( const Thumbnail& currentThumbnail );

    Thumbnail( MediaLibraryPtr ml, sqlite::Row& row );
    /**
     * @brief Thumbnail Builds a temporary thumbnail in memory
     *
     * @param ml A pointer to the media library instance
     * @param mrl The absolute mrl to the thumbnail
     * @param origin The thumbnail's origin
     * @param sizeType The thumbnail's size type
     * @param isOwned true if the thumbnail is owned by the media library
     *
     * A thumbnail created with this constructor can be inserted in database
     * at a later time using \sa{Thumbnail::insert()}
     * This constructor is meant for successful thumbnails only. Failure records
     * need to use the constructor taking a ThumbnailStatus as 2nd parameter
     */
    Thumbnail( MediaLibraryPtr ml, std::string mrl, Origin origin,
               ThumbnailSizeType sizeType, bool isOwned );
    /**
     * @brief Thumbnail Builds a temporary failure record in memory
     * @param ml A pointer to the media library instance
     * @param status The status, which must *not* be ThumbnailStatus::Success
     * @param origin The thumbnail origin
     * @param sizeType The thumbnail size type
     */
    Thumbnail( MediaLibraryPtr ml, ThumbnailStatus status, Origin origin,
               ThumbnailSizeType sizeType );

    /**
     * @brief Thumbnail Builds a thumbnail that represents an embedded artwork
     * @param ml A pointer to the media library instance
     * @param e An instance of an embedded thumbnail
     * @param sizeType The thumbnail size type
     *
     * This assumes that the origin is Origin::Media
     */
    Thumbnail( MediaLibraryPtr ml, std::shared_ptr<parser::IEmbeddedThumbnail> e,
               ThumbnailSizeType sizeType );

    int64_t id() const;
    const std::string& mrl() const;

    /**
     * @brief insert Insert the thumbnail in database
     * @return The new entity primary key, or 0 in case of failure
     */
    int64_t insert();

    /**
     * @brief unlinkThumbnail Removes the link between an entity and a thumbnail
     * @param entityId The ID of the entity to unlink
     * @param type The type of entity of the linking record to remove
     */
    void unlinkThumbnail( int64_t entityId, EntityType entityType );

    Origin origin() const;
    /**
     * @brief isOwned Returns true if the medialibrary owns this thumbnail.
     *
     * A thumbnail is owned if it's been (re)located into the medialib's thumbnail
     * folder.
     */
    bool isOwned() const;
    bool isShared() const;

    ThumbnailSizeType sizeType() const;

    ThumbnailStatus status() const;
    /**
     * @brief setErrorStatus Updates the status in case of an error
     * @param status An error status
     */
    bool markFailed();

    /**
     * @brief nbAttempts Returns the number of attempted generation for this thumbnail
     * This is intended for unit testing purposes only.
     */
    uint32_t nbAttempts() const;

    /**
     * @brief hash returns this thumbnail sha1
     */
    const std::string& hash() const;
    /**
     * @brief fileSize Returns this thumbnail size on disk, in bytes
     */
    uint64_t fileSize() const;

    /**
     * @brief setHash Sets the thumbnail hash & file size
     * @param hash A hash of this thumbnail file
     * @param fileSize The file size
     */
    void setHash( std::string hash, uint64_t fileSize );

    /**
     * @brief relocate Moves the file associated with the thumbnail to the dedicated
     * media library folder.
     *
     * @note This function assumes that the thumbnail is valid. Failure records
     * must not be relocated.
     */
    void relocate();

    static std::shared_ptr<Thumbnail>
    updateOrReplace( MediaLibraryPtr ml, std::shared_ptr<Thumbnail> oldThumbnail,
                     std::shared_ptr<Thumbnail> newThumbnail, ShouldUpdateCb cb,
                     int64_t entityId, EntityType entityType );

    static void createTable( sqlite::Connection* dbConnection );
    static void createTriggers( sqlite::Connection* dbConnection );
    static void createIndexes( sqlite::Connection* dbConnection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string triggerName( Triggers trigger, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    // This also hides the database helper variant, as we can't just select from
    // the thumbnail table. We need to get data from both the thumbnail & linking
    // tables.
    static std::shared_ptr<Thumbnail> fetch( MediaLibraryPtr ml,
                                             EntityType type,
                                             int64_t entityId,
                                             ThumbnailSizeType sizeType );

    /**
     * @brief deleteFailureRecords Allow the thumbnail to retry any previously failed attempt
     *
     * This will delete all failure records
     */
    static bool deleteFailureRecords( MediaLibraryPtr ml );

    static std::string path( MediaLibraryPtr ml, int64_t thumbnailId );

    static const std::string EmptyMrl;

    static constexpr std::underlying_type_t<ThumbnailSizeType>
    SizeToInt( ThumbnailSizeType sizeType )
    {
        return static_cast<std::underlying_type_t<ThumbnailSizeType>>( sizeType );
    }

private:
    /**
     * @brief toRelativeMrl Convert the provided absolute mrl to a mrl relative to
     *                      the user provided thumbnail directory.
     */
    std::string toRelativeMrl( const std::string& absoluteMrl );


    /**
     * @brief updateLinkEntity Updates the record linking an entity and a thumbnail
     * @param entityId The target entity ID
     * @param type The type of entity for this linking record
     * @param sizeType The thumbnail size type
     * @return true in case of success, false otherwise
     *
     * @warning This must be run in a transaction, as the associated thumbnail
     * gets inserted.
     * This is expected to be called when a new thumbnail had to be inserted, and
     * the linking entity needs updating.
     */
    bool updateLinkRecord( int64_t entityId, EntityType type, Origin origin );
    /**
     * @brief insertLinkRecord Insert a new record to link an entity and a thumbnail
     * @param entityId The target entity id
     * @param type The type of entity to insert a linking record for
     * @param origin The thumbnail origin
     *
     * This is expected to be called when a new thumbnail gets inserted, or
     * when it can be shared with another entity.
     */
    bool insertLinkRecord( int64_t entityId, EntityType type, Origin origin );

    bool update( std::shared_ptr<Thumbnail> newThumbnail );
    bool update( std::string mrl, bool isOwned );

    /**
     * @brief updateAllLinkRecords Updates all link record using this thumbnail to
     *                             use the one identified by newThumbnailId
     * @param newThumbnailId The new thumbnail to link to
     * @return true in case of success, false otherwise
     *
     * If this returns true, the instance must be considered invalid since it's
     * pointing to the previous thumbnail, while it has now been removed from
     * database, as the shared counter immediatly reached 0 (since no entity is
     * linked with it anymore)
     */
    bool updateAllLinkRecords( int64_t newThumbnailId );

private:
    MediaLibraryPtr m_ml;
    int64_t m_id;
    std::string m_mrl;
    Origin m_origin;
    ThumbnailSizeType m_sizeType;
    ThumbnailStatus m_status;
    uint32_t m_nbAttempts;
    bool m_isOwned;
    uint32_t m_sharedCounter;
    uint64_t m_fileSize;
    std::string m_hash;
    std::shared_ptr<parser::IEmbeddedThumbnail> m_embeddedThumbnail;

    friend Thumbnail::Table;
};

}
