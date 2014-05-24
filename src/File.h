#ifndef FILE_H
#define FILE_H


#include <sqlite3.h>

#include "IFile.h"
#include "Cache.h"

class Album;
class ShowEpisode;
class AlbumTrack;

class File;

namespace policy
{
struct FileTable
{
    static const std::string Name;
    static const std::string CacheColumn;
    static unsigned int File::*const PrimaryKey;
};
struct FileCache
{
    typedef std::string KeyType;
    static const std::string& key(const std::shared_ptr<File> self);
    static std::string key( sqlite3_stmt* stmt );
};
}

class File : public IFile, public Cache<File, IFile, policy::FileTable, policy::FileCache>
{
    private:
        typedef Cache<File, IFile, policy::FileTable, policy::FileCache> _Cache;
    public:

        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        File(sqlite3* dbConnection , sqlite3_stmt* stmt);
        File( const std::string& mrl );

        static FilePtr create(sqlite3* dbConnection, const std::string& mrl );
        static bool createTable(sqlite3* connection);

        virtual unsigned int id() const;
        virtual AlbumTrackPtr albumTrack();
        virtual bool setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual unsigned int duration() const;
        virtual std::shared_ptr<IShowEpisode> showEpisode();
        virtual bool addLabel( LabelPtr label );
        virtual bool removeLabel( LabelPtr label );
        virtual std::vector<std::shared_ptr<ILabel> > labels();
        virtual int playCount() const;
        virtual const std::string& mrl() const;

    private:
        sqlite3* m_dbConnection;

        // DB fields:
        unsigned int m_id;
        Type m_type;
        unsigned int m_duration;
        unsigned int m_albumTrackId;
        unsigned int m_playCount;
        unsigned int m_showEpisodeId;
        std::string m_mrl;

        // Auto fetched related properties
        Album* m_album;
        AlbumTrackPtr m_albumTrack;
        std::shared_ptr<ShowEpisode> m_showEpisode;

        friend class Cache<File, IFile, policy::FileTable, policy::FileCache>;
        friend struct policy::FileTable;
};

#endif // FILE_H
