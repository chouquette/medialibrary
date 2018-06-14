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

#include "medialibrary/IPlaylist.h"

#include "database/SqliteTools.h"
#include "database/DatabaseHelpers.h"
#include "utils/Cache.h"

namespace medialibrary
{

class Playlist;
class File;

namespace policy
{
struct PlaylistTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t Playlist::*const PrimaryKey;
};
}

class Playlist : public IPlaylist, public DatabaseHelpers<Playlist, policy::PlaylistTable>
{
public:
    Playlist( MediaLibraryPtr ml, sqlite::Row& row );
    Playlist( MediaLibraryPtr ml, const std::string& name );

    static std::shared_ptr<Playlist> create( MediaLibraryPtr ml, const std::string& name );

    virtual int64_t id() const override;
    virtual const std::string& name() const override;
    virtual bool setName( const std::string& name ) override;
    virtual unsigned int creationDate() const override;
    virtual const std::string& artworkMrl() const override;
    virtual Query<IMedia> media() const override;
    virtual bool append( int64_t mediaId ) override;
    virtual bool add( int64_t mediaId, unsigned int position ) override;
    virtual bool move( int64_t mediaId, unsigned int position ) override;
    virtual bool remove( int64_t mediaId ) override;
    std::shared_ptr<File> addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                   bool isFolderFsRemovable );

    static void createTable( sqlite::Connection* dbConn );
    static void createTriggers( sqlite::Connection* dbConn );
    static Query<IPlaylist> search( MediaLibraryPtr ml, const std::string& name,
                                    const QueryParameters* params );
    static Query<IPlaylist> listAll( MediaLibraryPtr ml, const QueryParameters* params );

    /**
     * @brief deleteAllExternal Delete all external playlists, ie. all playlist
     *                          that were parsed from playlist files.
     * Playlist manually added by the user are untouched
     */
    static void clearExternalPlaylistContent( MediaLibraryPtr ml );

private:
    static std::string sortRequest( const QueryParameters* params );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    std::string m_name;
    int64_t m_fileId;
    unsigned int m_creationDate;
    std::string m_artworkMrl;

    friend policy::PlaylistTable;
};

}
