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

namespace parser
{

class ITaskCb
{
public:
    virtual ~ITaskCb() = default;
    virtual bool updateFileId( int64_t fileId ) = 0;
};

class Task : public DatabaseHelpers<Task>, private ITaskCb
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t parser::Task::*const PrimaryKey;
    };
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
        Item( ITaskCb* taskCb, std::string mrl, IFile::Type fileType,
              unsigned int subitemIndex, bool isRefresh );
        Item( ITaskCb* taskCb, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
              std::shared_ptr<Folder> folder, std::shared_ptr<fs::IDirectory> folderFs,
              IFile::Type fileType,
              std::shared_ptr<Playlist> parentPlaylist, unsigned int parentPlaylistIndex,
              bool isRefresh );
        Item( ITaskCb* taskCb, std::shared_ptr<File> file, std::shared_ptr<fs::IFile> fileFs );


        virtual std::string meta( Metadata type ) const override;
        virtual void setMeta( Metadata type, std::string value ) override;

        virtual const std::string& mrl() const override;
        void setMrl( std::string mrl );

        virtual IFile::Type fileType() const override;

        virtual size_t nbSubItems() const override;
        virtual const IItem& subItem( unsigned int index ) const override;
        virtual IItem& createSubItem( std::string mrl, unsigned int playlistIndex ) override;

        virtual int64_t duration() const override;
        virtual void setDuration( int64_t duration ) override;

        virtual const std::vector<Track>& tracks() const override;
        virtual void addTrack( Track&& t ) override;

        virtual MediaPtr media() override;
        virtual void setMedia( MediaPtr media ) override;

        virtual FilePtr file() override;
        virtual bool setFile( FilePtr file ) override;

        virtual FolderPtr parentFolder() override;

        virtual std::shared_ptr<fs::IFile> fileFs() override;

        virtual std::shared_ptr<fs::IDirectory> parentFolderFs() override;

        virtual PlaylistPtr parentPlaylist() override;

        virtual unsigned int parentPlaylistIndex() const override;

        virtual bool isRefresh() const override;

    private:
        ITaskCb* m_taskCb;

        std::string m_mrl;
        IFile::Type m_fileType;
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
        bool m_isRefresh;
    };

    static_assert( std::is_move_assignable<Item>::value, "Item must be move assignable" );


    Task( MediaLibraryPtr ml, sqlite::Row& row );
    /**
     * @brief Task Construct a task for a newly detected file
     * @param ml A pointer to the medialibrary instance
     * @param fileFs The file, as seen on the filesystem
     * @param parentFolder The parent folder, in DB
     * @param parentFolderFs The parent folder, as seen on the filesystem
     * @param fileType The newly detected file type
     * @param parentPlaylist A parent playlist, if any
     * @param parentPlaylistIndex The index in the playlist
     */
    Task( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
          std::shared_ptr<Folder> parentFolder,
          std::shared_ptr<fs::IDirectory> parentFolderFs,
          IFile::Type fileType,
          std::shared_ptr<Playlist> parentPlaylist,
          unsigned int parentPlaylistIndex );
    /**
     * @brief Task Constructor for refresh tasks
     * @param ml A medialibrary instance pointer
     * @param file The known file, to be refreshed
     * @param fileFs The updated file, on the filesystem
     */
    Task( MediaLibraryPtr ml, std::shared_ptr<File> file,
          std::shared_ptr<fs::IFile> fileFs );

    /*
     * We need to decouple the current parser state and the saved one.
     * For instance, metadata extraction won't save anything in DB, so while
     * we might want to know that it's been processed and metadata have been
     * extracted, in case we were to restart the parsing, we would need to
     * extract the same information again
     */
    void markStepCompleted( parser::Step stepCompleted );
    bool saveParserStep();
    // Reset the retry count to 0 but doesn't advance the step.
    // This is intended to be used when a step requires its completion not to be
    // saved in database, but to avoid having a retry_count being incremented again
    // when starting next step.
    bool decrementRetryCount();
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
    static std::shared_ptr<Task> create( MediaLibraryPtr ml, std::string mrl, std::shared_ptr<fs::IFile> fileFs,
                                         std::shared_ptr<Folder> parentFolder,
                                         std::shared_ptr<fs::IDirectory> parentFolderFs,
                                         IFile::Type fileType,
                                         std::pair<std::shared_ptr<Playlist>,
                                         unsigned int> parentPlaylist );
    static std::shared_ptr<Task> createRefreshTask( MediaLibraryPtr ml,
                                                    std::shared_ptr<File> file,
                                                    std::shared_ptr<fs::IFile> fsFile );
    /**
     * @brief removePlaylistContentTasks Removes existing task associated with
     *                                   the given playlist
     *
     * Only completed tasks will be removed.
     */
    static void removePlaylistContentTasks( MediaLibraryPtr ml, int64_t playlistId );
    static void recoverUnscannedFiles( MediaLibraryPtr ml );

private:
    MediaLibraryPtr m_ml;
    int64_t     m_id;
    Step        m_step;
    int         m_retryCount;
    int64_t     m_fileId;
    int64_t     m_parentFolderId;
    int64_t     m_parentPlaylistId;
    Item        m_item;

    friend Task::Table;
};

}

}
