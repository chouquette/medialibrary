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
#include <vlcpp/vlc.hpp>

#include "database/DatabaseHelpers.h"

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

class Task : public DatabaseHelpers<Task, policy::TaskTable, cachepolicy::Uncached<Task>>
{
public:
    enum class Status
    {
        /// Default value.
        /// Also, having success = 0 is not the best idea ever.
        Unknown,
        /// All good
        Success,
        /// We can't compute this file for now (for instance the file was on a network drive which
        /// isn't connected anymore)
        TemporaryUnavailable,
        /// Something failed and we won't continue
        Fatal
    };

    enum class ParserStep : uint8_t
    {
        None = 0,
        MetadataExtraction = 1,
        MetadataAnalysis = 2,

        Completed = 1 | 2,
    };

    class Item
    {
    public:
        Item() = default;
        Item( std::string mrl );
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
        };
        std::string meta( Metadata type ) const;
        void setMeta( Metadata type, std::string value );

        const std::string& mrl() const;

        const std::vector<Item>& subItems() const;
        void addSubItem( Item mrl );

        int64_t duration() const;
        void setDuration( int64_t duration );

    private:
        std::string m_mrl;
        std::unordered_map<Metadata, std::string> m_metadata;
        std::vector<Item> m_subItems;
        int64_t m_duration;
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
    void markStepCompleted( ParserStep stepCompleted );
    bool saveParserStep();
    bool isCompleted() const;
    bool isStepCompleted( ParserStep step ) const;
    /**
     * @brief startParserStep Do some internal book keeping to avoid restarting a step too many time
     */
    void startParserStep();

    bool updateFileId();
    int64_t id() const;

    Item& item();

    // Restore attached entities such as media/files
    bool restoreLinkedEntities();
    void setMrl( std::string mrl );

    std::shared_ptr<Media>          media;
    std::shared_ptr<File>           file;
    std::shared_ptr<fs::IFile>      fileFs;
    std::shared_ptr<Folder>         parentFolder;
    std::shared_ptr<fs::IDirectory> parentFolderFs;
    std::shared_ptr<Playlist>       parentPlaylist;
    unsigned int                    parentPlaylistIndex;
    VLC::Media                      vlcMedia;
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
    ParserStep  m_step;
    int         m_retryCount;
    int64_t     m_fileId;
    int64_t     m_parentFolderId;
    int64_t     m_parentPlaylistId;
    Item        m_item;

    friend policy::TaskTable;
};

}

}
