#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include <vlc/vlc.h>
#include <vlcpp/vlc.hpp>
#include <mutex>

#include "IMetadataService.h"

class VLCMetadataService : public IMetadataService
{
    public:
        VLCMetadataService(const VLC::Instance& vlc);
        virtual ~VLCMetadataService();

        virtual bool initialize( IMetadataServiceCb *callback, IMediaLibrary* ml );
        virtual unsigned int priority() const;
        virtual bool run( FilePtr file, void *data );

    private:
        struct Context : public ::VLC::IMediaEventCb
        {
            Context(VLCMetadataService* self, FilePtr file, void* data );
            ~Context();

            VLCMetadataService* self;
            FilePtr file;
            void* data;
            VLC::Media media;

            private:
                virtual void parsedChanged( bool status ) override;
        };

    private:
        static void eventCallback( const libvlc_event_t* e, void* data );
        void parse( Context* ctx );
        ServiceStatus handleMediaMeta( Context* ctx );
        void parseAudioFile( Context* ctx );
        void parseVideoFile( Context* ctx );
        void cleanup();

        VLC::Instance m_instance;
        IMetadataServiceCb* m_cb;
        IMediaLibrary* m_ml;
        // We can't cleanup from the callback since it's called from a VLC thread.
        // If the refcounter was to reach 0 from there, it would destroy resources
        // that are still held.
        std::vector<Context*> m_cleanup;
        std::mutex m_lock;
};

#endif // VLCMETADATASERVICE_H
