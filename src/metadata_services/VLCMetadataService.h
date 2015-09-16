#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include <vlcpp/vlc.hpp>

#include "IMetadataService.h"

class VLCMetadataService : public IMetadataService
{
    public:
        VLCMetadataService(const VLC::Instance& vlc);

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        ServiceStatus handleMediaMeta(FilePtr file , VLC::Media &media);
        bool parseAudioFile(FilePtr file , VLC::Media &media);
        bool parseVideoFile(FilePtr file , VLC::Media &media);

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
};

#endif // VLCMETADATASERVICE_H
