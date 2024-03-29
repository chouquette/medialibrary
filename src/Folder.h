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

#include "medialibrary/IFolder.h"
#include "medialibrary/IMedia.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class File;
class Device;

// This doesn't publicly expose the DatabaseHelper inheritance in order to force
// the user to go through Folder's overloads, as they take care of the device mountpoint
// fetching & path composition
class Folder : public IFolder, public DatabaseHelpers<Folder>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Folder::*const PrimaryKey;
    };

    struct FtsTable
    {
        static const std::string Name;
    };

    /* Deprecated since model 30 */
    struct ExcludedFolderTable
    {
        static const std::string Name;
    };

    enum class Triggers : uint8_t
    {
        InsertFts,
        DeleteFts,
        UpdateNbMediaOnIndex,
        UpdateNbMediaOnUpdate,
        UpdateNbMediaOnDelete,
        UpdateIsPublic,
    };

    enum class Indexes : uint8_t
    {
        DeviceId,
        ParentId,
    };

    enum class BannedType
    {
        Yes,    //< Only select banned folders
        No,     //< Only select unbanned folders
        Any,    //< Well... any of the above.
    };

    enum class RemovalBehavior
    {
        /*
         * The folder was removed from disk: remove it from the db and delete
         * the media it contained
         */
        RemovedFromDisk,
        /*
         * The root folder was explicitly removed. Flag it as such in database
         * but keep its media on disk
         */
        RootRemoved,
    };

    Folder( MediaLibraryPtr ml, sqlite::Row& row );
    Folder(MediaLibraryPtr ml, std::string path, std::string name, int64_t parent,
            int64_t deviceId , bool isRemovable );

    static void createTable( sqlite::Connection* connection );
    static void createTriggers( sqlite::Connection* connection );
    static void createIndexes( sqlite::Connection* connection );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string triggerName( Triggers trigger, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static std::shared_ptr<Folder> create( MediaLibraryPtr ml, const std::string& mrl,
                                           int64_t parentId, const Device& device,
                                           fs::IDevice& deviceFs );
    static bool ban( MediaLibraryPtr ml, const std::string& mrl );
    static std::vector<std::shared_ptr<Folder>> fetchRootFolders( MediaLibraryPtr ml );

    static std::shared_ptr<Folder> fromMrl( MediaLibraryPtr ml,
                                            const std::string& mrl );
    static std::shared_ptr<Folder> bannedFolder( MediaLibraryPtr ml,
                                                 const std::string& mrl );
    static Query<IFolder> withMedia( MediaLibraryPtr ml, IMedia::Type type,
                                     const QueryParameters* params );
    static Query<IFolder> searchWithMedia( MediaLibraryPtr ml,
                                           const std::string& pattern,
                                           IMedia::Type type,
                                           const QueryParameters* params );
    static Query<IFolder> roots( MediaLibraryPtr ml, bool banned,
                                 int64_t deviceId, const QueryParameters* params );
    /**
     * @brief deleteFolder Mark a folder in database as removed
     * @param ml The media library instance
     * @param folder The folder to remove
     * @param behavior An enum member to drive the removal behavior
     * @return true in case of success, false otherwise.
     *
     * This will mark the folder as removed. If the folder was a root folder,
     * it will simply be removed from the database. If it was a sub folder of an
     * root folder, the folder will be marked as banned in order to not be
     * discovered again.
     * If the behavior passed is RemovedFromDisk, all the media that belonged to
     * that folder will be removed from the database. This also means that the
     * media will be removed from any media group or playlist they belonged to.
     * Otherwise, when behavior is RootRemoved, the media will be converted
     * to external media, and will be kept in database.
     */
    static bool remove( MediaLibraryPtr ml, std::shared_ptr<Folder> folder, RemovalBehavior behavior );

    virtual int64_t id() const override;
    virtual const std::string& mrl() const override;
    virtual const std::string& name() const override;
    const std::string& rawMrl() const;
    void setMrl( std::string mrl );
    std::vector<std::shared_ptr<File>> files();
    std::vector<std::shared_ptr<Folder>> folders();
    int64_t deviceId() const;
    virtual bool isRemovable() const override;
    virtual bool isPresent() const override;
    virtual bool isBanned() const override;
    virtual bool isPublic() const override;
    virtual bool setPublic( bool isPublic ) override;
    bool isRootFolder() const;
    virtual Query<IMedia> media( IMedia::Type type,
                                 const QueryParameters* params ) const override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       IMedia::Type type,
                                       const QueryParameters* params = nullptr ) const override;
    virtual Query<IFolder> subfolders( const QueryParameters* params ) const override;

    virtual Query<IPlaylist> playlists(const QueryParameters* params) const override;

    virtual uint32_t nbVideo() const override;
    virtual uint32_t nbAudio() const override;
    virtual uint32_t nbMedia() const override;
    virtual int64_t duration() const override;
    virtual bool isFavorite() const override;
    virtual bool setFavorite( bool favorite ) override;

    static std::shared_ptr<Folder> fromMrl( MediaLibraryPtr ml,
                                            const std::string& mrl,
                                            BannedType bannedType );

private:
    static std::string sortRequest( const QueryParameters* params );
    static std::string filterByMediaType( IMedia::Type type );

    std::shared_ptr<Device> device() const;
    bool ban();

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    // This contains the path relative to the device mountpoint (ie. excluding it)
    // or the full path (including mrl scheme) for folders on non removable devices
    std::string m_path;
    mutable std::string m_name;
    const int64_t m_parent;
    const bool m_isBanned;
    const int64_t m_deviceId;
    // Can't be const anymore, but should be if we ever get to remove the
    // removable->non removable device fixup (introduced after vlc-android 3.1.0 rc3)
    bool m_isRemovable;
    uint32_t m_nbAudio;
    uint32_t m_nbVideo;
    int64_t m_duration;
    bool m_isPublic;
    bool m_isFavorite;
    bool m_publicOnlyListing;

    mutable std::shared_ptr<Device> m_device;
    // This contains the full path, including device mountpoint (and mrl scheme,
    // as its part of the mountpoint
    mutable std::string m_fullPath;

    friend struct Folder::Table;
};

}
