#ifndef IFILE_H
#define IFILE_H

#include <vector>

#include "ITrackInformation.h"

class IAlbumTrack;
class ILabel;
class IShowEpisode;
class ITrackInformation;

class IFile
{
    public:
        virtual ~IFile() {}

        virtual IAlbumTrack* albumTrack() = 0;
        virtual unsigned int duration() = 0;
        virtual IShowEpisode* showEpisode() = 0;
        virtual int playCount() = 0;
        virtual const std::string& mrl() = 0;
        
        virtual std::vector<ILabel*> labels() = 0;
};

#endif // IFILE_H
