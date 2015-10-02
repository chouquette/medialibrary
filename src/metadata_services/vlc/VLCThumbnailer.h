#pragma once

#include <condition_variable>

#include <vlcpp/vlc.hpp>

#include "IMetadataService.h"

class VLCThumbnailer : public IMetadataService
{
public:
    VLCThumbnailer( const VLC::Instance& vlc );
    virtual bool initialize(IMetadataServiceCb *callback, IMediaLibrary *ml) override;
    virtual unsigned int priority() const override;
    virtual bool run(FilePtr file, void *data) override;

private:
    bool startPlayback(FilePtr file, VLC::MediaPlayer& mp , void *data);
    bool seekAhead(FilePtr file, VLC::MediaPlayer &mp, void *data);
    void setupVout(VLC::MediaPlayer &mp);
    bool takeSnapshot(FilePtr file, VLC::MediaPlayer &mp, void* data);
    bool compress(uint8_t* buff, FilePtr file, void* data );

private:
    // Force a base width, let height be computed depending on A/R
    static const uint32_t Width = 320;
    static const uint8_t Bpp = 4;

private:
    VLC::Instance m_instance;
    IMetadataServiceCb* m_cb;
    IMediaLibrary* m_ml;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    // Per snapshot variables
    std::unique_ptr<uint8_t[]> m_buff;
    bool m_snapshotRequired;
    uint32_t m_height;
};
