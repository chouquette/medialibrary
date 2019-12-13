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

#include "medialibrary/IPlaylist.h"

#include "database/DatabaseHelpers.h"

#include <map>

namespace medialibrary
{

class File;

class Playlist : public IPlaylist, public DatabaseHelpers<Playlist>, public std::enable_shared_from_this<Playlist>
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t Playlist::*const PrimaryKey;
    };
    struct FtsTable
    {
        static const std::string Name;
    };
    struct MediaRelationTable
    {
        static const std::string Name;
    };
    enum class Triggers : uint8_t
    {
        UpdateOrderOnInsert,
        UpdateOrderOnDelete,
        InsertFts,
        UpdateFts,
        DeleteFts,

        // Deprecated since model 16
        Append,
        UpdateOrderOnPositionUpdate
    };
    enum class Indexes : uint8_t
    {
        FileId,
        PlaylistIdPosition
    };

    // Contains the backup date as the index, and a vector of playlist files as values
    using Backups = std::map<time_t, std::vector<std::string>>;

    Playlist( MediaLibraryPtr ml, sqlite::Row& row );
    Playlist( MediaLibraryPtr ml, const std::string& name );

    static std::shared_ptr<Playlist> create( MediaLibraryPtr ml, const std::string& name );

    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual bool setName( const std::string& name ) override;
    virtual unsigned int creationDate() const override;
    virtual const std::string& artworkMrl() const override;
    virtual Query<IMedia> media() const override;
    virtual Query<IMedia> searchMedia( const std::string& pattern,
                                       const QueryParameters* params ) const override;
    virtual bool append( const IMedia& media ) override;
    virtual bool add( const IMedia& media, uint32_t position ) override;
    virtual bool append( int64_t mediaId ) override;
    virtual bool add( int64_t mediaId, uint32_t position ) override;
    virtual bool move( uint32_t from, uint32_t position ) override;
    virtual bool remove( uint32_t position ) override;
    virtual bool isReadOnly() const override;
    virtual std::string mrl() const override;
    std::shared_ptr<File> addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                   bool isFolderFsRemovable );
    bool contains( int64_t mediaId, uint32_t position );

    static void createTable( sqlite::Connection* dbConn );
    static void createTriggers( sqlite::Connection* dbConn , uint32_t dbModel);
    static void createIndexes( sqlite::Connection* dbConn, uint32_t dbModel );
    static std::string schema( const std::string& tableName, uint32_t dbModel );
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static Query<IPlaylist> search( MediaLibraryPtr ml, const std::string& name,
                                    const QueryParameters* params );
    static Query<IPlaylist> listAll( MediaLibraryPtr ml, const QueryParameters* params );

    /**
     * @brief deleteAllExternal Delete all external playlists, ie. all playlist
     *                          that were parsed from playlist files.
     * Playlist manually added by the user are untouched
     */
    static bool clearExternalPlaylistContent( MediaLibraryPtr ml );

    /**
     * @brief clearContent Removes all media from this playlist.
     */
    bool clearContent();

    static Backups loadBackups( MediaLibraryPtr ml );
    static std::tuple<bool, time_t, std::vector<std::string>>
        backupPlaylists( MediaLibraryPtr ml, uint32_t dbModel);

    static std::shared_ptr<Playlist> fromFile( MediaLibraryPtr ml, int64_t fileId );

private:
    static std::string sortRequest( const QueryParameters* params );
    static bool writeBackup( const std::string& name,
                             const std::vector<std::string>& mrls,
                             const std::string& destFile );
    void curateNullMediaID() const;

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    std::string m_name;
    int64_t m_fileId;
    const unsigned int m_creationDate;
    std::string m_artworkMrl;

    friend Playlist::Table;
};

}
