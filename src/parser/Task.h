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

class File;
class Folder;

namespace parser
{

static constexpr auto MaxNbRetries = 1u;

struct LastTaskInfo
{
    int64_t parentFolderId;
    std::shared_ptr<fs::IDirectory> fsDir;
};

class Task : public DatabaseHelpers<Task>, public IItem
{
public:
    struct Table
    {
        static const std::string Name;
        static const std::string PrimaryKeyColumn;
        static int64_t parser::Task::*const PrimaryKey;
    };
    enum class Triggers : uint8_t
    {
        DeletePlaylistLinkingTask,
    };
    enum class Indexes : uint8_t
    {
        ParentFolderId,
    };

    struct MetadataHash
    {
        size_t operator()(IItem::Metadata m) const
        {
            return static_cast<size_t>( m );
        }
    };

    enum class Type : uint8_t
    {
        /// This task is about creating an entity (Media, Playlist, ...)
        Creation,
        /// This task is about linking entities together (Adding playlist items
        /// in a playlist for instance)
        Link,
        /// This task is about refreshing an existing item because it associated
        /// file was updated on disk
        Refresh,
        /// This task is meant to restore something. For now, this is only used
        /// to restore a playlist backup, in the event of a database corruption
        Restore,
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
          IFile::Type fileType );
    /**
     * @brief Task Constructor for refresh tasks
     * @param ml A medialibrary instance pointer
     * @param file The known file, to be refreshed
     * @param fileFs The updated file, on the filesystem
     */
    Task( MediaLibraryPtr ml, std::shared_ptr<File> file,
          std::shared_ptr<fs::IFile> fileFs, std::shared_ptr<Folder> parentFolder,
          std::shared_ptr<fs::IDirectory> parentFolderFs );

    /**
     * @brief Task Contructor for a link task
     * @param ml A medialibrary instance pointer
     * @param linkToId The entity to link to ID
     * @param linkToType The entity to link to type
     * @param linkExtra An extra parameter, contextual to the type of linking.
     * @param mrl The mrl of the new entity to link
     */
    Task( MediaLibraryPtr ml, std::string mrl, int64_t linkToId,
          LinkType linkToType, int64_t linkExtra);

    /**
     * @brief Task constructor for a link task with an unknown entity
     * @param ml A medialibrary instance pointer
     * @param mrl The entity being linked's mrl
     * @param fileType The entity being linked's type
     * @param linkToMrl The entity being linked *to*'s mrl
     * @param linkToType The entity being linked'd type
     * @param linkExtra An extra parameter, contextual to the type of linking.
     */
    Task( MediaLibraryPtr ml, std::string mrl, IFile::Type fileType,
          std::string linkToMrl, LinkType linkToType, int64_t linkExtra );

    /**
     * @brief Task Contructor for restore tasks
     * @param ml A medialibrary instance pointer
     * @param mrl The mrl of the entity to restore
     */
    Task( MediaLibraryPtr ml, std::string mrl, IFile::Type fileType );

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
    /**
     * @brief goToNextService Increments the internal current service id and return it.
     * @return
     */
    uint32_t goToNextService();
    void resetCurrentService();
    unsigned int retryCount() const;

    int64_t id() const;

    // Restore attached entities such as media/files
    bool restoreLinkedEntities( LastTaskInfo& lastTaskInfo );
    static bool setMrl( MediaLibraryPtr ml, int64_t taskId, const std::string& mrl );
    void setMrl( std::string mrl );

    static void createTable( sqlite::Connection* dbConnection );
    static void createTriggers(sqlite::Connection* dbConnection );
    static void createIndex(sqlite::Connection* dbConnection);
    static std::string schema(const std::string& tableName, uint32_t dbModel);
    static std::string trigger( Triggers trigger, uint32_t dbModel );
    static std::string triggerName( Triggers trigger, uint32_t dbModel );
    static std::string index( Indexes index, uint32_t dbModel );
    static std::string indexName( Indexes index, uint32_t dbModel );
    static bool checkDbModel( MediaLibraryPtr ml );
    static bool resetRetryCount( MediaLibraryPtr ml );
    static bool resetParsing( MediaLibraryPtr ml );
    static std::vector<std::shared_ptr<Task>> fetchUncompleted( MediaLibraryPtr ml );
    static std::shared_ptr<Task> create( MediaLibraryPtr ml, std::shared_ptr<fs::IFile> fileFs,
                                         std::shared_ptr<Folder> parentFolder,
                                         std::shared_ptr<fs::IDirectory> parentFolderFs,
                                         IFile::Type fileType );
    static std::shared_ptr<Task> createRefreshTask( MediaLibraryPtr ml,
                                                    std::shared_ptr<File> file,
                                                    std::shared_ptr<fs::IFile> fsFile,
                                                    std::shared_ptr<Folder> parentFolder,
                                                    std::shared_ptr<fs::IDirectory> parentFolderFs );
    /**
     * @brief createMediaRefreshTask Convenienve method to create a media refresh task
     *
     * This is basically just a wrapper to createRefreshTask, but it will handle
     * the required boilerplate of fetching parent folders, fs files, ...
     */
    static std::shared_ptr<Task> createMediaRefreshTask( MediaLibraryPtr ml,
                                                         std::shared_ptr<Media> media );
    /**
     * @brief createLinkTask Create a link task with a known entity
     * @param ml A media library instance
     * @param mrl The mrl of the entity being linked
     * @param linkToType The type of the entity being linked *to*
     * @param linkToExtra Some extra linking parameter
     * @return A new task
     *
     * This will create the task in database and automatically push it to the
     * parser task queue.
     */
    static std::shared_ptr<Task> createLinkTask( MediaLibraryPtr ml, std::string mrl,
                                                 int64_t linkToId, LinkType linkToType,
                                                 int64_t linkToExtra );
    /**
     * @brief createLinkTask Create a link task with an unknown entity
     * @param ml A media library instance
     * @param mrl The mrl of the entity being linked
     * @param fileType The linked file type
     * @param linkToMrl The mrl of the entity being linked *to*
     * @param linkToType The type of the entity being linked *to*
     * @param linkToExtra Some extra linking parameter
     * @return a new task
     *
     * This will create the task in database and automatically push it to the
     * parser task queue.
     * This will *not*, however, create the task responsible for creating the
     * entity to be linked *to*.
     */
    static std::shared_ptr<Task> createLinkTask( MediaLibraryPtr ml, std::string mrl,
                                                 IFile::Type fileType,
                                                 std::string linkToMrl,
                                                 LinkType linkToType,
                                                 int64_t linkToExtra );

    static std::shared_ptr<Task> createRestoreTask( MediaLibraryPtr ml,
                                                    std::string mrl,
                                                    IFile::Type fileType );
    /**
     * @brief removePlaylistContentTasks Removes existing task associated with
     *                                   the given playlist
     *
     * Only completed tasks will be removed.
     */
    static bool removePlaylistContentTasks( MediaLibraryPtr ml, int64_t playlistId );
    /**
     * @brief removePlaylistContentTasks
     * @param ml
     * @param playlistId
     */
    static bool removePlaylistContentTasks( MediaLibraryPtr ml );
    static bool recoverUnscannedFiles( MediaLibraryPtr ml );

    /***************************************************************************
     * IItem interface implementation
     **************************************************************************/
    virtual std::string meta( Metadata type ) const override;
    virtual void setMeta( Metadata type, std::string value ) override;

    virtual const std::string& mrl() const override;
    virtual IFile::Type fileType() const override;

    virtual size_t nbLinkedItems() const override;
    virtual const IItem& linkedItem( unsigned int index ) const override;
    virtual IItem& createLinkedItem( std::string mrl, IFile::Type itemType,
                                     int64_t linkExtra ) override;

    virtual int64_t duration() const override;
    virtual void setDuration( int64_t duration ) override;

    virtual const std::vector<Track>& tracks() const override;
    virtual void addTrack( Track&& t ) override;

    virtual MediaPtr media() override;
    virtual void setMedia( MediaPtr media ) override;

    virtual FilePtr file() override;
    virtual bool setFile( FilePtr file ) override;

    virtual FolderPtr parentFolder() override;

    virtual std::shared_ptr<fs::IFile> fileFs() const override;

    virtual std::shared_ptr<fs::IDirectory> parentFolderFs() override;

    virtual bool isRefresh() const override;
    bool isLinkTask() const;
    virtual bool isRestore() const override;

    virtual LinkType linkType() const override;
    virtual int64_t linkToId() const override;
    virtual int64_t linkExtra() const override;
    virtual const std::string& linkToMrl() const override;

    bool needEntityRestoration() const;

private:
    MediaLibraryPtr m_ml = nullptr;
    int64_t     m_id = 0;
    Step        m_step = Step::None;
    unsigned int m_retryCount = 0;
    Type        m_type;
    std::string m_mrl;
    IFile::Type m_fileType = IFile::Type::Unknown;
    int64_t     m_fileId = 0;
    int64_t     m_parentFolderId = 0;
    int64_t     m_linkToId = 0;
    LinkType    m_linkToType = LinkType::NoLink;
    int64_t     m_linkExtra = 0;
    std::string m_linkToMrl;

    unsigned int m_currentService = 0;
    std::unordered_map<Metadata, std::string, MetadataHash> m_metadata;
    std::vector<Task> m_linkedItems;
    std::vector<Track> m_tracks;
    int64_t m_duration = 0;
    MediaPtr m_media;
    FilePtr m_file;
    std::shared_ptr<fs::IFile> m_fileFs;
    FolderPtr m_parentFolder;
    std::shared_ptr<fs::IDirectory> m_parentFolderFs;

    friend Task::Table;
};

}

}
