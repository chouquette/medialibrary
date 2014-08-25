#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include <vlc/vlc.h>
#include <mutex>

#include "IMetadataService.h"

class VLCMetadataService : public IMetadataService
{
    public:
        VLCMetadataService(libvlc_instance_t* vlc);
        virtual ~VLCMetadataService();

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        struct Context
        {
            Context();
            ~Context();

            VLCMetadataService* self;
            FilePtr file;
            void* data;
            libvlc_media_t* media;
            libvlc_event_manager_t* m_em;
            libvlc_media_player_t* mp;
            libvlc_event_manager_t* mp_em;
        };

    private:
        static void eventCallback( const libvlc_event_t* e, void* data );
        void parse( Context* ctx );
        ServiceStatus handleMediaMeta( Context* ctx );
        void parseAudioFile( Context* ctx );
        void parseVideoFile( Context* ctx );
        void cleanup();

        libvlc_instance_t* m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
        // We can't cleanup from the callback since it's called from a VLC thread.
        // If the refcounter was to reach 0 from there, it would destroy resources
        // that are still held.
        std::vector<Context*> m_cleanup;
        std::mutex m_lock;
};

#endif // VLCMETADATASERVICE_H
