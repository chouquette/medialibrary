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
    enum class ParserStep : uint8_t
    {
        None = 0,
        MetadataExtraction = 1,
        MetadataAnalysis = 2,
        Thumbnailer = 4,

        Completed = 1 | 2 | 4,
    };

    File( MediaLibraryPtr ml, sqlite::Row& row );
    File( MediaLibraryPtr ml, int64_t mediaId, Type type, const fs::IFile& file, int64_t folderId, bool isRemovable );
    virtual int64_t id() const override;
    virtual const std::string& mrl() const override;
    virtual Type type() const override;
    virtual unsigned int lastModificationDate() const override;
    virtual unsigned int size() const override;
    /*
     * We need to decouple the current parser state and the saved one.
     * For instance, metadata extraction won't save anything in DB, so while
     * we might want to know that it's been processed and metadata have been
     * extracted, in case we were to restart the parsing, we would need to
     * extract the same information again
     */
    void markStepCompleted( ParserStep step );
    bool saveParserStep();
    ParserStep parserStep() const;
    std::shared_ptr<Media> media() const;
    bool destroy();

    static bool createTable( DBConnection dbConnection );
    static std::shared_ptr<File> create( MediaLibraryPtr ml, int64_t mediaId, Type type,
                                         const fs::IFile& file, int64_t folderId, bool isRemovable );
    /**
     * @brief fromPath  Attempts to fetch a file using its full path
     * This will only work if the file was stored on a non removable device
     * @param path      The full path to the wanted file
     * @return          A pointer to the wanted file, or nullptr if it wasn't found
     */
    static std::shared_ptr<File> fromPath( MediaLibraryPtr ml, const std::string& path );
    /**
     * @brief fromFileName  Attemps to fetch a file based on its filename and folder id
     * @param ml
     * @param fileName
     * @param folderId
     * @return
     */
    static std::shared_ptr<File> fromFileName( MediaLibraryPtr ml, const std::string& fileName, int64_t folderId );

private:
    MediaLibraryPtr m_ml;

    int64_t m_id;
    int64_t m_mediaId;
    std::string m_mrl;
    Type m_type;
    unsigned int m_lastModificationDate;
    unsigned int m_size;
    ParserStep m_parserSteps;
    int64_t m_folderId;
    bool m_isPresent;
    bool m_isRemovable;

    mutable Cache<std::string> m_fullPath;
    mutable Cache<std::weak_ptr<Media>> m_media;

    friend policy::FileTable;
};

}
