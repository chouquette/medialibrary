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

#include <condition_variable>
#include <mutex>
#include "IFile.h"
#include "IMediaLibrary.h"

#include "metadata_services/emotion/Emotion.h"

class EmotionMetadataServiceCb : public IMetadataCb
{
public:
    std::condition_variable waitCond;
    std::mutex mutex;

    virtual void onMetadataUpdated( FilePtr )
    {
        std::unique_lock<std::mutex> lock( mutex );
        waitCond.notify_all();
    }
};

class EmotionMetadataService_Tests : public Tests
{
protected:
    static std::unique_ptr<EmotionMetadataServiceCb> cb;

public:
    static void SetUpTestCase()
    {
        cb.reset( new EmotionMetadataServiceCb );
    }

    virtual void SetUp() override
    {
        Tests::Reload( nullptr, cb.get() );
        auto emotionService = std::unique_ptr<EmotionMetadataService>( new EmotionMetadataService );
        ml->addMetadataService( std::move( emotionService ) );
    }
};

std::unique_ptr<EmotionMetadataServiceCb> EmotionMetadataService_Tests::cb;


TEST_F( EmotionMetadataService_Tests, ParseAudio )
{
    std::unique_lock<std::mutex> lock( cb->mutex );
    auto file = ml->addFile( "mr-zebra.mp3" );
    bool res = cb->waitCond.wait_for( lock, std::chrono::seconds( 5 ), [&]{
        return file->audioTracks().size() > 0;
    } );
    ASSERT_TRUE( res );
}
