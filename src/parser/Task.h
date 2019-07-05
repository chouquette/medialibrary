/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include "medialibrary/parser/IItem.h"
#include "database/DatabaseHelpers.h"
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

class Task : public DatabaseHelpers<Task>, public IItem
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t parser::Task::*const PrimaryKey;
    };

    struct MetadataHash
    {
        size_t operator()(IItem::Metadata m) const
        {
            return static_cast<size_t>( m );
        }
    };

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

    /**
     * @brief Task Constructor for dummy tasks, to represent subitems
     */
    Task( std::string mrl, IFile::Type fileType, unsigned int playlistIndex );

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

    int64_t id() const;

    // Restore attached entities such as media/files
    bool restoreLinkedEntities();
    void setMrl( std::string mrl );

    unsigned int                    currentService = 0;

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

    /***************************************************************************
     * IItem interface implementation
     **************************************************************************/
    virtual std::string meta( Metadata type ) const override;
    virtual void setMeta( Metadata type, std::string value ) override;

    virtual const std::string& mrl() const override;
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
    MediaLibraryPtr m_ml = nullptr;
    int64_t     m_id = 0;
    Step        m_step = Step::None;
    int         m_retryCount = 0;
    std::string m_mrl;
    IFile::Type m_fileType = IFile::Type::Unknown;
    int64_t     m_fileId = 0;
    int64_t     m_parentFolderId = 0;
    int64_t     m_parentPlaylistId = 0;
    unsigned int m_parentPlaylistIndex = 0;
    bool m_isRefresh = false;

    std::unordered_map<Metadata, std::string, MetadataHash> m_metadata;
    std::vector<Task> m_subItems;
    std::vector<Track> m_tracks;
    int64_t m_duration = 0;
    MediaPtr m_media;
    FilePtr m_file;
    std::shared_ptr<fs::IFile> m_fileFs;
    FolderPtr m_parentFolder;
    std::shared_ptr<fs::IDirectory> m_parentFolderFs;
    PlaylistPtr m_parentPlaylist;

    friend Task::Table;
};

}

}
