#ifndef TESTER_H
#define TESTER_H

#include <condition_variable>
#include "gtest/gtest.h"
#include <mutex>
#include <rapidjson/document.h>

#include "MediaLibrary.h"
#include "IAlbum.h"
#include "IArtist.h"
#include "IMedia.h"
#include "IAlbumTrack.h"
#include "IAudioTrack.h"
#include "IVideoTrack.h"

class MockCallback : public IMediaLibraryCb
{
public:
    MockCallback();
    bool waitForParsingComplete();

private:
    void onFileAdded(MediaPtr) {}
    void onFileUpdated(MediaPtr) {}
    void onDiscoveryStarted(const std::string&) {}
    void onDiscoveryCompleted(const std::string&) {}
    void onReloadStarted() {}
    void onReloadCompleted() {}
    void onParsingStatsUpdated(uint32_t nbParsed, uint32_t nbToParse);

    std::condition_variable m_parsingCompleteVar;
    std::mutex m_parsingMutex;
    bool m_done;
};

class Tests : public ::testing::TestWithParam<const char*>
{
protected:
    std::unique_ptr<MediaLibrary> m_ml;
    std::unique_ptr<MockCallback> m_cb;

    virtual void SetUp() override
    {
        unlink("test.db");
        m_cb.reset( new MockCallback );
        m_ml.reset( new MediaLibrary );
        m_ml->setVerbosity( LogLevel::Error );
        m_ml->initialize( "test.db", "/tmp", m_cb.get() );
    }

    void checkVideoTracks( const rapidjson::Value& expectedTracks, const std::vector<VideoTrackPtr>& tracks );
    void checkAudioTracks(const rapidjson::Value& expectedTracks, const std::vector<AudioTrackPtr>& tracks );
    void checkMedias( const rapidjson::Value& expectedMedias );
    void checkAlbums( const rapidjson::Value& expectedAlbums);
    void checkAlbumTracks( const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks );
};

#endif // TESTER_H
