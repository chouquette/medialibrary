#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

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
        ~VLCMetadataService();

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        ServiceStatus handleMediaMeta(FilePtr file , VLC::Media &media) const;
        bool parseAudioFile(FilePtr file , VLC::Media &media) const;
        bool parseVideoFile(FilePtr file , VLC::Media &media) const;

        void cleanup();

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
        std::mutex m_cleanupLock;
        std::vector<Context*> m_cleanup;
};

#endif // VLCMETADATASERVICE_H
