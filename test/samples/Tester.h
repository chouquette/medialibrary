/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef TESTER_H
#define TESTER_H

#include <condition_variable>
#include "gtest/gtest.h"
#include <mutex>
#include <rapidjson/document.h>

#include "common/MediaLibraryTester.h"
#include "medialibrary/IAlbum.h"
#include "medialibrary/IArtist.h"
#include "medialibrary/IMedia.h"
#include "medialibrary/IAlbumTrack.h"
#include "medialibrary/IAudioTrack.h"
#include "medialibrary/IVideoTrack.h"
#include "medialibrary/IGenre.h"
#include "medialibrary/IPlaylist.h"
#include "mocks/NoopCallback.h"

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback();
    virtual bool waitForParsingComplete();
    virtual bool waitForDiscoveryComplete() { return true; }
    virtual void reinit() {}
    void prepareWaitForThumbnail( MediaPtr media );
    bool waitForThumbnail();
protected:
    virtual void onDiscoveryCompleted( const std::string&, bool ) override;
    virtual void onParsingStatsUpdated(uint32_t percent) override;
    virtual void onMediaThumbnailReady( MediaPtr media, bool success );

    compat::ConditionVariable m_parsingCompleteVar;
    compat::Mutex m_parsingMutex;
    compat::Mutex m_thumbnailMutex;
    compat::ConditionVariable m_thumbnailCond;
    MediaPtr m_thumbnailTarget;
    bool m_thumbnailDone;
    bool m_thumbnailSuccess;
    bool m_done;
    bool m_discoveryCompleted;
};

class MockResumeCallback : public MockCallback
{
public:
    MockResumeCallback();
    virtual bool waitForDiscoveryComplete() override;
    virtual bool waitForParsingComplete() override;
    virtual void onDiscoveryCompleted( const std::string& entryPoint, bool ) override;
    virtual void reinit() override;

private:
    compat::ConditionVariable m_discoveryCompletedVar;
    compat::Mutex m_discoveryMutex;
};

class Tests : public ::testing::TestWithParam<const char*>
{
protected:
    std::unique_ptr<MockCallback> m_cb;
    std::unique_ptr<MediaLibrary> m_ml;

    virtual void SetUp() override;

    virtual void InitializeCallback();
    virtual void InitializeMediaLibrary();

    void runChecks( const rapidjson::Document& doc );

    void checkVideoTracks( const rapidjson::Value& expectedTracks, const std::vector<VideoTrackPtr>& tracks );
    void checkAudioTracks(const rapidjson::Value& expectedTracks, const std::vector<AudioTrackPtr>& tracks );
    void checkMedias( const rapidjson::Value& expectedMedias );
    void checkPlaylists( const rapidjson::Value& expectedPlaylists, std::vector<PlaylistPtr> playlists );
    void checkAlbums(const rapidjson::Value& expectedAlbums, std::vector<AlbumPtr> albums);
    void checkArtists( const rapidjson::Value& expectedArtists, std::vector<ArtistPtr> artists );
    void checkAlbumTracks(const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks , bool& found) const;
};

class ResumeTests : public Tests
{
public:
    class MediaLibraryResumeTest : public MediaLibrary
    {
    public:
        void forceParserStart();
    protected:
        virtual bool startParser() override;
    };

    virtual void InitializeMediaLibrary() override;
    virtual void InitializeCallback();
};

#endif // TESTER_H
