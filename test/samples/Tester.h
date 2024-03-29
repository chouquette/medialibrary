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
#include <mutex>
#include <rapidjson/document.h>

#include "common/Tests.h"
#include "common/util.h"
#include "logging/Logger.h"
#include "logging/IostreamLogger.h"
#include "medialibrary/IAlbum.h"
#include "medialibrary/IArtist.h"
#include "medialibrary/IMedia.h"
#include "medialibrary/IAudioTrack.h"
#include "medialibrary/IVideoTrack.h"
#include "medialibrary/IGenre.h"
#include "medialibrary/IPlaylist.h"
#include "medialibrary/ISubtitleTrack.h"
#include "medialibrary/IFolder.h"
#include "Folder.h"
#include "common/NoopCallback.h"
#include "medialibrary/IDeviceLister.h"
#include "MediaLibrary.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback();
    virtual void waitForParsingComplete();
    virtual void waitForDiscoveryComplete() {}
    virtual bool waitForRemovalComplete();
    virtual void reinit();
    void prepareWaitForThumbnail( MediaPtr media );
    bool waitForThumbnail();
    void prepareForPlaylistReload();
    void waitForPlaylistReload();
    void prepareForDiscovery( uint32_t nbRootsExpected );
    void prepareForRemoval( uint32_t nbRootsRemovalExpected );
    /*
     * May be invoked directly by tests which don't discover anything, such
     * as collection tests which only discover external MRLs
     */
    virtual void onDiscoveryCompleted() override;
protected:
    virtual void onDiscoveryStarted() override;
    virtual void onParsingStatsUpdated( uint32_t done, uint32_t scheduled ) override;
    virtual void onMediaThumbnailReady( MediaPtr media, ThumbnailSizeType sizeType,
                                        bool success ) override;
    virtual void onRootRemoved( const std::string& root, bool res ) override;
    virtual void onBackgroundTasksIdleChanged( bool idle ) override;

    compat::ConditionVariable m_parsingCompleteVar;
    compat::Mutex m_parsingMutex;
    compat::Mutex m_thumbnailMutex;
    compat::ConditionVariable m_thumbnailCond;
    MediaPtr m_thumbnailTarget;
    bool m_thumbnailDone;
    bool m_thumbnailSuccess;
    bool m_parserDone;
    bool m_discoveryCompleted;
    bool m_removalCompleted;
    uint32_t m_nbRootsRemovalExpected;
};

class MockResumeCallback : public MockCallback
{
public:
    virtual void waitForDiscoveryComplete() override;
    virtual void waitForParsingComplete() override;
    virtual void onDiscoveryCompleted() override;
    virtual void reinit() override;

private:
    compat::ConditionVariable m_discoveryCompletedVar;
};

struct Tests
{
    Tests()
    {
        Log::SetLogger( std::make_shared<IostreamLogger>() );
        Log::setLogLevel( LogLevel::Debug );
    }

    virtual ~Tests() = default;
    virtual void SetUp(const std::string& testSuite, const std::string& testName );
    virtual void TearDown();
    std::unique_ptr<MockCallback> m_cb;
    std::unique_ptr<IMediaLibrary> m_ml;

    rapidjson::Document doc;
    rapidjson::GenericValue<rapidjson::UTF8<>> input;
    rapidjson::GenericValue<rapidjson::UTF8<>> subscriptions;

    static const std::string Directory;

    virtual void InitializeCallback();
    virtual void InitializeMediaLibrary( const std::string& dbPath,
                                         const std::string& mlFolderDir );
    /* Helper to manipulate the subscription MRL to convert it to an absolute path */
    void addSubscription( IService::Type s, std::string mrl );
    void runChecks();

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
    void checkMediaFiles( const IMedia* media, const rapidjson::Value &expectedFiles );
    void checkSubscriptions( const rapidjson::Value& expectedSubscriptions,
                             std::vector<SubscriptionPtr> subscriptions );

protected:
    virtual void InitTestCase( const std::string& testName );

private:
    std::string m_testDir;
};

class MediaLibraryResumeTest : public MediaLibrary
{
public:
    using MediaLibrary::MediaLibrary;
    virtual ~MediaLibraryResumeTest();
    void forceParserStart();
    virtual void onDbConnectionReady( sqlite::Connection* dbConn ) override;
    virtual parser::Parser* getParser() const override;
protected:
    bool m_allowParser = false;
};

struct ResumeTests : public Tests
{
    virtual void InitializeMediaLibrary( const std::string& dbPath,
                                         const std::string& mlFolderDir ) override;
    virtual void InitializeCallback() override;
};

struct RefreshTests : public Tests
{
    void forceRefresh();
    virtual void InitializeCallback() override;
};

struct BackupRestorePlaylistTests : public Tests
{
    virtual void InitTestCase( const std::string& ) override {}
};

struct ReplaceExternalMediaByPlaylistTests : public Tests
{
    virtual void InitTestCase( const std::string& ) override;
    std::string playlistMrl;
};

#endif // TESTER_H
