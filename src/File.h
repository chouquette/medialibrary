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
    public:


        enum Type
        {
            VideoType, // Any video file, not being a tv show episode
            AudioType, // Any kind of audio file, not being an album track
            ShowEpisodeType,
            AlbumTrackType,
            UnknownType,
        };

        // Those should be private, however the standard states that the expression
        // ::new (pv) T(std::forward(args)...)
        // shall be well-formed, and private constructor would prevent that.
        // There might be a way with a user-defined allocator, but we'll see that later...
        File(sqlite3* dbConnection , sqlite3_stmt* stmt);
        File( const std::string& mrl );

        bool insert(sqlite3* dbConnection);
        static bool createTable(sqlite3* connection);

        virtual unsigned int id() const;
        virtual std::shared_ptr<IAlbumTrack> albumTrack();
        virtual unsigned int duration();
        virtual std::shared_ptr<IShowEpisode> showEpisode();
        virtual bool addLabel( LabelPtr label );
        virtual bool removeLabel( LabelPtr label );
        virtual std::vector<std::shared_ptr<ILabel> > labels();
        virtual int playCount();
        virtual const std::string& mrl();

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
        std::shared_ptr<AlbumTrack> m_albumTrack;
        std::shared_ptr<ShowEpisode> m_showEpisode;

        friend class Cache<File, IFile, policy::FileTable, policy::FileCache>;
};

#endif // FILE_H
