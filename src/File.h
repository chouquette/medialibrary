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

#include "medialibrary/IFile.h"
#include "database/DatabaseHelpers.h"
#include "database/SqliteConnection.h"
#include "filesystem/IFile.h"
#include "utils/Cache.h"
#include "parser/Parser.h"

namespace medialibrary
{

class File;
class Media;

namespace policy
{
struct FileTable
{
    static const std::string Name;
    static const std::string PrimaryKeyColumn;
    static int64_t File::*const PrimaryKey;
};
}

class File : public IFile, public DatabaseHelpers<File, policy::FileTable>
{
public:

    File( MediaLibraryPtr ml, sqlite::Row& row );
    File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type, const fs::IFile& file, int64_t folderId, bool isRemovable );
    File( MediaLibraryPtr ml, int64_t mediaId, int64_t playlistId, Type type, const std::string& mrl );
    virtual int64_t id() const override;
    virtual const std::string& mrl() const override;
    virtual Type type() const override;
    virtual unsigned int lastModificationDate() const override;
    virtual unsigned int size() const override;
    virtual bool isExternal() const override;
    /*
     * We need to decouple the current parser state and the saved one.
     * For instance, metadata extraction won't save anything in DB, so while
     * we might want to know that it's been processed and metadata have been
     * extracted, in case we were to restart the parsing, we would need to
     * extract the same information again
     */
    void markStepCompleted( parser::Task::ParserStep step );
    void markStepUncompleted( parser::Task::ParserStep step );
    bool saveParserStep();
    parser::Task::ParserStep parserStep() const;
    /**
     * @brief startParserStep Do some internal book keeping to avoid restarting a step too many time
     */
    void startParserStep();
    std::shared_ptr<Media> media() const;
    bool destroy();
    int64_t folderId();

    static bool createTable( sqlite::Connection* dbConnection );
    static std::shared_ptr<File> createFromMedia( MediaLibraryPtr ml, int64_t mediaId, Type type,
                                                  const fs::IFile& file, int64_t folderId, bool isRemovable );
    static std::shared_ptr<File> createFromMedia( MediaLibraryPtr ml, int64_t mediaId, Type type,
                                                  const std::string& mrl );

    static std::shared_ptr<File> createFromPlaylist( MediaLibraryPtr ml, int64_t playlistId, const fs::IFile& file,
                                                     int64_t folderId, bool isRemovable );

    /**
     * @brief fromPath  Attempts to fetch a file using its mrl
     * This will only work if the file was stored on a non removable device
     * @param path      The wanted file mrl
     * @return          A pointer to the wanted file, or nullptr if it wasn't found
     */
    static std::shared_ptr<File> fromMrl( MediaLibraryPtr ml, const std::string& mrl );
    /**
     * @brief fromFileName  Attemps to fetch a file based on its filename and folder id
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

    static std::vector<std::shared_ptr<File>> fetchUnparsed( MediaLibraryPtr ml );
    static void resetRetryCount( MediaLibraryPtr ml );
    static void resetParsing( MediaLibraryPtr ml );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    int64_t m_mediaId;
    int64_t m_playlistId;
    // Contains the path relative to the containing folder for files contained in a removable folder
    // or the full file MRL for non removable ones
    std::string m_mrl;
    Type m_type;
    std::time_t m_lastModificationDate;
    unsigned int m_size;
    parser::Task::ParserStep m_parserSteps;
    int64_t m_folderId;
    bool m_isPresent;
    bool m_isRemovable;
    bool m_isExternal;

    // Contains the full path as a MRL
    mutable Cache<std::string> m_fullPath;
    mutable Cache<std::weak_ptr<Media>> m_media;

    friend policy::FileTable;
};

}
