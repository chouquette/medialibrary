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

#include "medialibrary/IFile.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Media;

class File : public IFile, public DatabaseHelpers<File>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t File::*const PrimaryKey;
    };
    enum class Indexes : uint8_t
    {
        MediaId,
        FolderId,
        PlaylistId,
        InsertionDate,
    };

    File( MediaLibraryPtr ml, sqlite::Row& row );
    File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type,
          const fs::IFile& file, int64_t folderId, bool isRemovable,
          time_t insertionDate );
    File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId,
          int64_t subscriptionId, Type type, const std::string& mrl,
          int64_t fileSize, time_t insertionDate, CacheType cacheType );
    virtual int64_t id() const override;
    virtual const std::string& mrl() const override;
    /**
     * @brief rawMrl returns the raw mrl, ie. the mrl from the mountpoint.
     * This is the same as MRL for files on non removable devices.
     * This is meant to be used when fiddling with the value stored in database
     * during a migration, but shouldn't be used otherwise, as it would be unusable
     */
    const std::string& rawMrl() const;
    void setMrl( std::string mrl );
    static bool setMrl(MediaLibraryPtr ml, const std::string &mrl, int64_t fileId );
    virtual Type type() const override;
    virtual time_t lastModificationDate() const override;
    virtual uint64_t size() const override;
    virtual bool isExternal() const override;
    bool updateFsInfo( time_t newLastModificationDate, uint64_t newSize );
    virtual bool isRemovable() const override;
    virtual bool isNetwork() const override;
    virtual bool isMain() const override;
    virtual time_t insertionDate() const override;
    virtual CacheType cacheType() const override;

    std::shared_ptr<Media> media() const;
    int64_t mediaId() const;
    /**
     * @brief setMediaId Associates this file with a different media
     *
     * If this was the previous media's main file, it will be deleted.
     */
    bool setMediaId( int64_t mediaId );
    bool setPlaylistId( int64_t playlistId );
    bool destroy();
    int64_t folderId() const;

    bool update( const fs::IFile& fileFs, int64_t folderId, bool isRemovable );
    bool convertToExternal();

    FilePtr cache( const std::string& mrl, CacheType cacheType, uint64_t fileSize );
    std::string cachedFileName() const;


    static void createTable( sqlite::Connection* dbConnection );
    static void createIndexes( sqlite::Connection* dbConnection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<File> createFromMedia( MediaLibraryPtr ml, int64_t mediaId, Type type,
                                                  const fs::IFile& file, int64_t folderId,
                                                  bool isRemovable, time_t insertionDate );
    static std::shared_ptr<File> createFromExternalMedia( MediaLibraryPtr ml,
                                                          int64_t mediaId, Type type,
                                                          const std::string& mrl,
                                                          int64_t fileSize,
                                                          time_t insertionDate );

    static std::shared_ptr<File> createFromPlaylist( MediaLibraryPtr ml, int64_t playlistId, const fs::IFile& file,
                                                     int64_t folderId, bool isRemovable, time_t insertionDate );
    static std::shared_ptr<File> createFromSubscription( MediaLibraryPtr ml, std::string mrl, int64_t subscriptionId );
    std::shared_ptr<File> createForCache( MediaLibraryPtr ml, int64_t mediaId,
                                          const std::string& mrl, int64_t fileSize,
                                          time_t insertionDate, CacheType cacheType );

    static bool exists( MediaLibraryPtr ml, const std::string& mrl );
    /**
     * @brief fromPath  Attempts to fetch a file using its mrl
     * This will only work if the file was stored on a non removable device
     * @param path      The wanted file mrl
     * @return          A pointer to the wanted file, or nullptr if it wasn't found
     */
    static std::shared_ptr<File> fromMrl( MediaLibraryPtr ml, const std::string& mrl );
    /**
     * @brief fromFileName  Attempts to fetch a file based on its filename and folder id
     * @param ml
     * @param fileName
     * @param folderId
     * @return
     */
    static std::shared_ptr<File> fromFileName( MediaLibraryPtr ml, const std::string& fileName, int64_t folderId );

    /**
     * @brief fromMrl Attempts to find an external stream (ie. it was added with MediaLibrary::addMedia,
     * and not discovered through any IDiscoverer)
     * This implies the folder_id is null
     * @return
     */
    static std::shared_ptr<File> fromExternalMrl( MediaLibraryPtr ml, const std::string& mrl );

    /**
     * @brief fromParentFolder Returns a vector of the known file in a given folder
     * @param ml The medialibrary instance pointer
     * @param parentFolderId The parent folder ID
     */
    static std::vector<std::shared_ptr<File>> fromParentFolder( MediaLibraryPtr ml,
                                                                int64_t parentFolderId );

    static std::vector<std::shared_ptr<File>> cachedFiles( MediaLibraryPtr ml );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    int64_t m_mediaId;
    int64_t m_playlistId;
    // Contains the path relative to the containing folder for files contained in a removable folder
    // or the full file MRL for non removable ones
    std::string m_mrl;
    const Type m_type;
    time_t m_lastModificationDate;
    uint64_t m_size;
    int64_t m_folderId;
    bool m_isRemovable;
    bool m_isExternal;
    bool m_isNetwork;
    int64_t m_subscriptionId;
    time_t m_insertionDate;
    CacheType m_cacheType;

    // Contains the full path as a MRL
    mutable std::string m_fullPath;
    mutable std::weak_ptr<Media> m_media;

    friend File::Table;
};

}
