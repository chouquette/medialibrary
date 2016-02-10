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

#include "MediaLibrary.h"
#include "IAlbum.h"
#include "IArtist.h"
#include "IMedia.h"
#include "IAlbumTrack.h"
#include "IAudioTrack.h"
#include "IVideoTrack.h"
#include "IGenre.h"

class MockCallback : public IMediaLibraryCb
{
public:
    MockCallback();
    bool waitForParsingComplete();

private:
    virtual void onMediaAdded(MediaPtr) override {}
    virtual void onMediaUpdated(MediaPtr) override {}
    virtual void onMediaDeleted( std::vector<int64_t> ) override {}
    virtual void onDiscoveryStarted(const std::string&) override {}
    virtual void onDiscoveryCompleted(const std::string&) override;
    virtual void onReloadStarted( const std::string& ) override {}
    virtual void onReloadCompleted( const std::string& ) override {}
    virtual void onArtistAdded( ArtistPtr ) override {}
    virtual void onAlbumAdded( AlbumPtr ) override {}
    virtual void onTrackAdded( MediaPtr, AlbumTrackPtr ) override {}

    virtual void onParsingStatsUpdated(uint32_t percent) override;

    std::condition_variable m_parsingCompleteVar;
    std::mutex m_parsingMutex;
    bool m_done;
    bool m_discoveryCompleted;
};

class Tests : public ::testing::TestWithParam<const char*>
{
protected:
    std::unique_ptr<MockCallback> m_cb;
    std::unique_ptr<MediaLibrary> m_ml;

    virtual void SetUp() override;

    void checkVideoTracks( const rapidjson::Value& expectedTracks, const std::vector<VideoTrackPtr>& tracks );
    void checkAudioTracks(const rapidjson::Value& expectedTracks, const std::vector<AudioTrackPtr>& tracks );
    void checkMedias( const rapidjson::Value& expectedMedias );
    void checkAlbums(const rapidjson::Value& expectedAlbums, std::vector<AlbumPtr> albums);
    void checkArtists( const rapidjson::Value& expectedArtists, std::vector<ArtistPtr> artists );
    void checkAlbumTracks(const IAlbum* album, const std::vector<MediaPtr>& tracks, const rapidjson::Value& expectedTracks , bool& found) const;
};

#endif // TESTER_H
