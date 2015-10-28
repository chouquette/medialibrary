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

#include "Tests.h"
#include <chrono>
#include <condition_variable>

#include "IMediaLibrary.h"
#include "IMetadataService.h"
#include "IMedia.h"
#include "IAudioTrack.h"
#include "IAlbum.h"
#include "IAlbumTrack.h"
#include "IArtist.h"
#include "IVideoTrack.h"

class ServiceCb : public IMediaLibraryCb
{
    public:
        std::condition_variable waitCond;
        std::mutex mutex;

        virtual void onFileAdded( MediaPtr ) override
        {
        }

        virtual void onFileUpdated( MediaPtr ) override
        {
            waitCond.notify_all();
        }

        // IMediaLibraryCb interface
        virtual void onDiscoveryStarted( const std::string& ) override {}
        virtual void onDiscoveryCompleted( const std::string& ) override {}
        virtual void onReloadStarted() override {}
        virtual void onReloadCompleted() override {}        
        virtual void onParsingStatsUpdated( uint32_t, uint32_t ) {}
};

class VLCMetadataServices : public Tests
{
    protected:
        static std::unique_ptr<ServiceCb> cb;

    protected:
        static void SetUpTestCase()
        {
            cb.reset( new ServiceCb );
        }

        virtual void SetUp() override
        {
            Tests::Reload( nullptr, cb.get() );
        }
};

std::unique_ptr<ServiceCb> VLCMetadataServices::cb;

TEST_F( VLCMetadataServices, ParseAudio )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mr-zebra.mp3", nullptr );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return file->audioTracks().size() > 0;
    } );

    ASSERT_TRUE( res );
    Reload();
    file = std::static_pointer_cast<Media>( ml->file( "mr-zebra.mp3" ) );
    auto tracks = file->audioTracks();
    ASSERT_EQ( tracks.size(), 1u );
    auto track = tracks[0];
    ASSERT_EQ( track->codec(), "mpga" );
    ASSERT_EQ( track->bitrate(), 128000u );
    ASSERT_EQ( track->sampleRate(), 44100u );
    ASSERT_EQ( track->nbChannels(), 2u );
}

TEST_F( VLCMetadataServices, ParseAlbum )
{
    {
        std::unique_lock<std::mutex> lock( cb->mutex );
        auto file = ml->addFile( "mr-zebra.mp3", nullptr );
        bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
            return file->albumTrack() != nullptr;
        } );
        ASSERT_TRUE( res );
    }
    Reload();

    auto file = ml->file( "mr-zebra.mp3" );
    ASSERT_TRUE( std::static_pointer_cast<Media>( file )->isParsed() );
    auto track = file->albumTrack();
    ASSERT_NE( track, nullptr );
    ASSERT_EQ( file->title(), "Mr. Zebra" );
    ASSERT_EQ( track->genre(), "Rock" );
    auto artist = file->artist();
    ASSERT_EQ( artist, "Tori Amos" );

    auto album = track->album();
    ASSERT_NE( album, nullptr );
    ASSERT_EQ( album->title(), "Boys for Pele" );
//    ASSERT_NE( album->artworkUrl().length(), 0u );

    auto releaseYear = album->releaseYear();
    ASSERT_NE( releaseYear, 0 );

    auto album2 = ml->album( album->id() );
    ASSERT_EQ( album, album2 );
}

TEST_F( VLCMetadataServices, ParseVideo )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mrmssmith.mp4", nullptr );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [file]{
        return file->videoTracks().size() != 0;
    } );

    ASSERT_TRUE( res );
    Reload();

    file = std::static_pointer_cast<Media>( ml->file( "mrmssmith.mp4" ) );

    ASSERT_EQ( file->showEpisode(), nullptr );

    auto tracks = file->videoTracks();
    ASSERT_EQ( tracks.size(), 1u );
    ASSERT_EQ( tracks[0]->codec(), "h264" );
    ASSERT_EQ( tracks[0]->width(), 320u );
    ASSERT_EQ( tracks[0]->height(), 176u );
    ASSERT_EQ( tracks[0]->fps(), 25 );

    auto audioTracks = file->audioTracks();
    ASSERT_EQ( audioTracks.size(), 1u );
    ASSERT_EQ( audioTracks[0]->codec(), "mp4a" );
    ASSERT_EQ( audioTracks[0]->sampleRate(), 44100u );
    ASSERT_EQ( audioTracks[0]->nbChannels(), 2u );
}

