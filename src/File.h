#ifndef FILE_H
#define FILE_H

#include <sqlite3.h>

#include "IFile.h"

class Album;
class ShowEpisode;
class AlbumTrack;

class File : public IFile
{
    public:
        enum Type
        {
            VideoType, // Any video file, not being a tv show episode
            AudioType, // Any kind of audio file, not being an album track
            ShowEpisodeType,
            AlbumTrackType,
        };

        File(sqlite3* dbConnection , sqlite3_stmt* stmt);

        bool insert(sqlite3* dbConnection);

        virtual IAlbumTrack* albumTrack();
        virtual unsigned int duration();
        virtual IShowEpisode* showEpisode();
        virtual std::vector<ILabel*> labels();
        virtual int playCount();

        static bool createTable( sqlite3* connection );

    private:
        sqlite3* m_dbConnection;
    
        // DB fields:
        unsigned int m_id;
        Type m_type;
        unsigned int m_duration;
        unsigned int m_albumTrackId;
        unsigned int m_playCount;
        unsigned int m_showEpisodeId;

        // Auto fetched related properties
        Album* m_album;
        AlbumTrack* m_albumTrack;
        ShowEpisode* m_showEpisode;
        std::vector<ILabel*>* m_labels;
};

#endif // FILE_H
