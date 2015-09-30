#ifndef FILE_H
#define FILE_H


#include <sqlite3.h>

#include "IFile.h"
#include "database/Cache.h"

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
        File(DBConnection dbConnection , sqlite3_stmt* stmt);
        File(const fs::IFile* file , unsigned int folderId);

        static FilePtr create(DBConnection dbConnection, const fs::IFile* file , unsigned int folderId);
        static bool createTable( DBConnection connection );

        virtual unsigned int id() const;
        virtual Type type() override;
        virtual bool setType( Type type ) override;
        virtual AlbumTrackPtr albumTrack();
        virtual bool setAlbumTrack( AlbumTrackPtr albumTrack );
        virtual unsigned int duration() const;
        virtual bool setDuration( unsigned int duration) override;
        virtual std::shared_ptr<IShowEpisode> showEpisode();
        virtual bool setShowEpisode( ShowEpisodePtr showEpisode );
        virtual bool addLabel( LabelPtr label );
        virtual bool removeLabel( LabelPtr label );
        virtual std::vector<std::shared_ptr<ILabel> > labels();
        virtual int playCount() const;
        virtual const std::string& mrl() const;
        virtual MoviePtr movie();
        virtual bool setMovie( MoviePtr movie );
        virtual bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps );
        virtual std::vector<VideoTrackPtr> videoTracks();
        virtual bool addAudioTrack(const std::string& codec, unsigned int bitrate , unsigned int sampleRate, unsigned int nbChannels);
        virtual std::vector<AudioTrackPtr> audioTracks();
        virtual const std::string& snapshot() override;
        virtual bool setSnapshot( const std::string& snapshot ) override;

        virtual bool isStandAlone() override;
        virtual unsigned int lastModificationDate() override;

        virtual bool isReady() const;
        bool setReady();

    private:
        DBConnection m_dbConnection;

        // DB fields:
        unsigned int m_id;
        Type m_type;
        unsigned int m_duration;
        unsigned int m_albumTrackId;
        unsigned int m_playCount;
        unsigned int m_showEpisodeId;
        std::string m_mrl;
        unsigned int m_movieId;
        unsigned int m_folderId;
        unsigned int m_lastModificationDate;
        std::string m_snapshot;
        bool m_isParsed;

        // Auto fetched related properties
        Album* m_album;
        AlbumTrackPtr m_albumTrack;
        ShowEpisodePtr m_showEpisode;
        MoviePtr m_movie;

        friend class Cache<File, IFile, policy::FileTable, policy::FileCache>;
        friend struct policy::FileTable;
};

#endif // FILE_H
