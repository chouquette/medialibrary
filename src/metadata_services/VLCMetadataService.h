#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include "IMetadataService.h"

#include <vlc/vlc.h>

class VLCMetadataService : public IMetadataService
{
    public:
        VLCMetadataService(libvlc_instance_t* vlc);
        virtual ~VLCMetadataService();

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file );

    private:
        struct Context
        {
            Context();
            ~Context();

            VLCMetadataService* self;
            FilePtr file;
            libvlc_media_t* media;
            libvlc_event_manager_t* m_em;
            libvlc_media_player_t* mp;
            libvlc_event_manager_t* mp_em;
        };

    private:
        static void eventCallback( const libvlc_event_t* e, void* data );
        void handleMediaMeta( Context* ctx );
        void parseAudioFile( Context* ctx );
        void parseVideoFile( Context* ctx );

        libvlc_instance_t* m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
};

#endif // VLCMETADATASERVICE_H
