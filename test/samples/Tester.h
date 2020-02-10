/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2019 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
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

#include "medialibrary/IAlbum.h"
#include "medialibrary/IArtist.h"
#include "medialibrary/IMedia.h"
#include "medialibrary/IAlbumTrack.h"
#include "medialibrary/IAudioTrack.h"
#include "medialibrary/IVideoTrack.h"
#include "medialibrary/IGenre.h"
#include "medialibrary/IPlaylist.h"
#include "medialibrary/ISubtitleTrack.h"
#include "medialibrary/IFolder.h"
#include "Folder.h"
#include "mocks/NoopCallback.h"
#include "medialibrary/IDeviceLister.h"
#include "MediaLibrary.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback();
    virtual bool waitForParsingComplete( std::unique_lock<compat::Mutex>& lock );
    virtual bool waitForDiscoveryComplete( std::unique_lock<compat::Mutex>& ) { return true; }
    virtual void reinit() {}
    void prepareWaitForThumbnail( MediaPtr media );
    bool waitForThumbnail();
    std::unique_lock<compat::Mutex> lock();
protected:
    virtual void onDiscoveryCompleted( const std::string&, bool ) override;
    virtual void onParsingStatsUpdated(uint32_t percent) override;
    virtual void onMediaThumbnailReady( MediaPtr media, ThumbnailSizeType sizeType,
                                        bool success ) override;

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

class ForceRemovableStorareDeviceLister : public IDeviceLister
{
public:
    ForceRemovableStorareDeviceLister();
    virtual std::vector<std::tuple<std::string, std::string, bool>> devices() const override;
    virtual bool start( IDeviceListerCb* ) override;
    virtual void stop() override;

private:
    DeviceListerPtr m_lister;
};

class MockResumeCallback : public MockCallback
{
public:
    MockResumeCallback();
    virtual bool waitForDiscoveryComplete( std::unique_lock<compat::Mutex>& lock ) override;
    virtual bool waitForParsingComplete( std::unique_lock<compat::Mutex>& lock ) override;
    virtual void onDiscoveryCompleted( const std::string& entryPoint, bool ) override;
    virtual void reinit() override;

private:
    compat::ConditionVariable m_discoveryCompletedVar;
};

struct TestParams
{
    const char* testCase;
    bool useRemovableFiles;
};

class Tests : public ::testing::TestWithParam<std::tuple<std::string, bool>>
{
protected:
    std::unique_ptr<MockCallback> m_cb;
    std::unique_ptr<IMediaLibrary> m_ml;

    virtual void SetUp() override;

    virtual void InitializeCallback();
    virtual void InitializeMediaLibrary();

    void runChecks( const rapidjson::Document& doc );

    void checkVideoTracks( const rapidjson::Value& expectedTracks, const std::vector<VideoTrackPtr>& tracks );
    void checkAudioTracks(const rapidjson::Value& expectedTracks, const std::vector<AudioTrackPtr>& tracks );
    void checkSubtitleTracks( const rapidjson::Value& expectedTracks, const std::vector<SubtitleTrackPtr>& tracks );
    void checkMedias( const rapidjson::Value& expectedMedias );
    void checkPlaylists( const rapidjson::Value& expectedPlaylists, std::vector<PlaylistPtr> playlists );
    void checkAlbums(const rapidjson::Value& expectedAlbums, std::vector<AlbumPtr> albums);
    void checkArtists( const rapidjson::Value& expectedArtists, std::vector<ArtistPtr> artists );
    void checkAlbumTracks(const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks , bool& found) const;
    void checkShows( const rapidjson::Value& expectedShows, std::vector<ShowPtr> shows );
    void checkShowEpisodes( const rapidjson::Value& expectedEpisodes, std::vector<MediaPtr> episodes );
    void checkMediaGroups( const rapidjson::Value& expectedMediaGroups,
                           std::vector<MediaGroupPtr> mediaGroups );
};

class ReducedTests : public Tests
{
};

class ResumeTests : public Tests
{
public:
    class MediaLibraryResumeTest : public MediaLibrary
    {
    public:
        void forceParserStart();
    protected:
        virtual void startParser() override;
    };

    virtual void InitializeMediaLibrary() override;
    virtual void InitializeCallback() override;
};

class RefreshTests : public Tests
{
public:
    void forceRefresh();
    virtual void InitializeCallback() override;
};

#endif // TESTER_H
