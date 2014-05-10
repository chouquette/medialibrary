#ifndef FILE_H
#define FILE_H

#include <sqlite3.h>

#include "IFile.h"

class File : public IFile
{
    public:
        enum Type
        {
            Video, // Any video file, not being a tv show episode
            Audio, // Any kind of audio file, not being an album track
            ShowEpisode,
            AlbumTrack,
        };

        File(sqlite3* dbConnection , sqlite3_stmt* stmt);
        File( sqlite3* dbConnection );

        bool insert();

        virtual IAlbumTrack* albumTrack();
        virtual const std::string& artworkUrl();
        virtual unsigned int duration();
        virtual IShowEpisode* showEpisode();
        virtual std::vector<ILabel*> labels();
        
        static bool CreateTable( sqlite3* connection );

    private:
        sqlite3* m_dbConnection;
    
        // DB fields:
        unsigned int m_id;
        Type m_type;
        unsigned int m_duration;
        
                
};

#endif // FILE_H
