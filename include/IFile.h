#ifndef IFILE_H
#define IFILE_H

#include <vector>
#include <memory>

#include "IMediaLibrary.h"

class IAlbumTrack;
class IShowEpisode;
class ITrackInformation;

class IFile
{
    public:
        enum Type
        {
            VideoType, // Any video file, not being a tv show episode
            AudioType, // Any kind of audio file, not being an album track
            ShowEpisodeType,
            AlbumTrackType,
            UnknownType
        };
        virtual ~IFile() {}

        virtual unsigned int id() const = 0;
        virtual AlbumTrackPtr albumTrack() = 0;
        virtual bool setAlbumTrack(AlbumTrackPtr albumTrack ) = 0;
        virtual unsigned int duration() const = 0;
        virtual std::shared_ptr<IShowEpisode> showEpisode() = 0;
        virtual bool setShowEpisode( ShowEpisodePtr showEpisode ) = 0;
        virtual int playCount() const = 0;
        virtual const std::string& mrl() const = 0;
        virtual bool addLabel( LabelPtr label ) = 0;
        virtual bool removeLabel( LabelPtr label ) = 0;
        virtual MoviePtr movie() = 0;
        virtual bool setMovie( MoviePtr movie ) = 0;
        virtual std::vector<std::shared_ptr<ILabel> > labels() = 0;
        virtual bool addVideoTrack( const std::string& codec, unsigned int width,
                                    unsigned int height, float fps ) = 0;
        virtual bool videoTracks( std::vector<VideoTrackPtr>& tracks ) = 0;
        virtual bool addAudioTrack( const std::string& codec, unsigned int bitrate ) = 0;
        virtual bool audioTracks( std::vector<AudioTrackPtr>& tracks ) = 0;
};

#endif // IFILE_H
