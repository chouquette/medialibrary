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
#include <unordered_map>

#include "database/DatabaseHelpers.h"
#include "medialibrary/parser/IItem.h"
#include "medialibrary/parser/Parser.h"

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
class Playlist;

namespace parser
{
class Task;
}

namespace policy
{
struct TaskTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t parser::Task::*const PrimaryKey;
};
}

namespace parser
{

class ITaskCb
{
public:
    virtual ~ITaskCb() = default;
    virtual bool updateFileId( int64_t fileId ) = 0;
};

class Task : public DatabaseHelpers<Task, policy::TaskTable, cachepolicy::Uncached<Task>>, private ITaskCb
{
public:
    class Item : public IItem
    {
    public:
        struct MetadataHash
        {
            size_t operator()(IItem::Metadata m) const
            {
                return static_cast<size_t>( m );
            }
        };

        Item() = default;
        /**
         * @brief Item Construct a parser item with a given mrl and subitem index
         * @param mrl The item's mrl
         * @param subitemPosition A potential subitem index, if any, or 0 if none.
         *
         * The position is used to keep subitems ordering for playlists
         */
        Item( ITaskCb* taskCb, std::string mrl, unsigned int subitemIndex );
        Item( ITaskCb* taskCb, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> folder, std::shared_ptr<fs::IDirectory> folderFs,
              std::shared_ptr<Playlist> parentPlaylist, unsigned int parentPlaylistIndex );


        virtual std::string meta( Metadata type ) const override;
        virtual void setMeta( Metadata type, std::string value ) override;

        virtual const std::string& mrl() const override;
        void setMrl( std::string mrl );

        virtual size_t nbSubItems() const override;
        virtual const IItem& subItem( unsigned int index ) const override;
        virtual IItem& createSubItem( std::string mrl, unsigned int playlistIndex ) override;

        virtual int64_t duration() const override;
        virtual void setDuration( int64_t duration ) override;

        virtual const std::vector<Track>& tracks() const override;
        virtual void addTrack( Track t ) override;

        virtual MediaPtr media() override;
        virtual void setMedia( MediaPtr media ) override;

        virtual FilePtr file() override;
        virtual bool setFile( FilePtr file ) override;

        virtual FolderPtr parentFolder() override;

        virtual std::shared_ptr<fs::IFile> fileFs() override;

        virtual std::shared_ptr<fs::IDirectory> parentFolderFs() override;

        virtual PlaylistPtr parentPlaylist() override;

        virtual unsigned int parentPlaylistIndex() const override;

    private:
        ITaskCb* m_taskCb;

        std::string m_mrl;
        std::unordered_map<Metadata, std::string, MetadataHash> m_metadata;
        std::vector<Item> m_subItems;
        std::vector<Track> m_tracks;
        int64_t m_duration;
        MediaPtr m_media;
        FilePtr m_file;
        std::shared_ptr<fs::IFile> m_fileFs;
        FolderPtr m_parentFolder;
        std::shared_ptr<fs::IDirectory> m_parentFolderFs;
        PlaylistPtr m_parentPlaylist;
        unsigned int m_parentPlaylistIndex;
    };

    static_assert( std::is_move_assignable<Item>::value, "Item must be move assignable" );


    /*
     * Constructs a task to be resumed.
     * The Media is provided as a parameter to avoid this to implicitely query
     * the database for the media associated to the provided file
     */
    Task( MediaLibraryPtr ml, sqlite::Row& row );
    Task( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
          std::shared_ptr<Folder> parentFolder,
          std::shared_ptr<fs::IDirectory> parentFolderFs,
          std::shared_ptr<Playlist> parentPlaylist,
          unsigned int parentPlaylistIndex );

    /*
     * We need to decouple the current parser state and the saved one.
     * For instance, metadata extraction won't save anything in DB, so while
     * we might want to know that it's been processed and metadata have been
     * extracted, in case we were to restart the parsing, we would need to
     * extract the same information again
     */
    void markStepCompleted( parser::Step stepCompleted );
    bool saveParserStep();
    bool isCompleted() const;
    bool isStepCompleted( parser::Step step ) const;
    /**
     * @brief startParserStep Do some internal book keeping to avoid restarting a step too many time
     */
    void startParserStep();

    virtual bool updateFileId( int64_t fileId ) override;
    int64_t id() const;

    Item& item();

    // Restore attached entities such as media/files
    bool restoreLinkedEntities();
    void setMrl( std::string mrl );

    unsigned int                    currentService;

    static void createTable( sqlite::Connection* dbConnection );
    static void resetRetryCount( MediaLibraryPtr ml );
    static void resetParsing( MediaLibraryPtr ml );
    static std::vector<std::shared_ptr<Task>> fetchUncompleted( MediaLibraryPtr ml );
    static std::shared_ptr<Task> create( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
                                         std::shared_ptr<Folder> parentFolder,
                                         std::shared_ptr<fs::IDirectory> parentFolderFs,
                                         std::pair<std::shared_ptr<Playlist>, unsigned int> parentPlaylist );
    static void recoverUnscannedFiles( MediaLibraryPtr ml );

private:
    MediaLibraryPtr m_ml;
    int64_t     m_id;
    Step  m_step;
    int         m_retryCount;
    int64_t     m_fileId;
    int64_t     m_parentFolderId;
    int64_t     m_parentPlaylistId;
    Item        m_item;

    friend policy::TaskTable;
};

}

}
