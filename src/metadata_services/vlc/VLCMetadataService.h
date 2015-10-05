#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include <condition_variable>
#include <vlcpp/vlc.hpp>
#include <mutex>

#include "IMetadataService.h"

class VLCMetadataService : public IMetadataService
{
    struct Context
    {
        Context(FilePtr file_)
            : file( file_ )
        {
        }

        FilePtr file;
        VLC::Media media;
    };

    public:
        VLCMetadataService(const VLC::Instance& vlc);

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        ServiceStatus handleMediaMeta(FilePtr file , VLC::Media &media) const;
        bool parseAudioFile(FilePtr file , VLC::Media &media) const;
        bool parseVideoFile(FilePtr file , VLC::Media &media) const;

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
        std::vector<Context*> m_keepAlive;
        std::mutex m_mutex;
        std::condition_variable m_cond;
};

#endif // VLCMETADATASERVICE_H
