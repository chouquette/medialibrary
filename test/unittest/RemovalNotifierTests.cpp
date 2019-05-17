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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "Tests.h"
#include "Media.h"
#include "File.h"
#include "mocks/NoopCallback.h"
#include "compat/Mutex.h"

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback() : m_nbMedia( 0 ) {}
private:
    virtual void onMediaDeleted( std::vector<int64_t> batch ) override
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        m_nbMedia = batch.size();
        m_cond.notify_all();
    }
public:
    std::unique_lock<compat::Mutex> prepareWait()
    {
        m_nbMedia = 0;
        return std::unique_lock<compat::Mutex>{ m_lock };
    }

    unsigned int waitForNotif( std::unique_lock<compat::Mutex> preparedLock, std::chrono::duration<int64_t> timeout )
    {
        m_cond.wait_for( preparedLock, timeout, [this]() { return m_nbMedia != 0; });
        return m_nbMedia;
    }

private:
    compat::Mutex m_lock;
    compat::ConditionVariable m_cond;
    uint32_t m_nbMedia;
};

class RemovalNotifierTests : public Tests
{
protected:
    std::unique_ptr<MockCallback> cbMock;
    virtual void SetUp() override
    {
        cbMock.reset( new MockCallback );
        mlCb = cbMock.get();
        Tests::SetUp();
    }

    virtual void InstantiateMediaLibrary() override
    {
        ml.reset( new MediaLibraryWithNotifier );
    }
};

TEST_F( RemovalNotifierTests, DeleteOne )
{
    auto m = std::static_pointer_cast<Media>( ml->addMedia( "media.avi" ) );
    auto lock = cbMock->prepareWait();
    m->removeFile( static_cast<File&>( *m->files()[0] ) );
    // This media doesn't have any associated files, and should be removed by a sqlite hook
    // The notification will arrive "late", as it will need to timeout first
    auto res = cbMock->waitForNotif( std::move( lock ), std::chrono::seconds{ 1 } );
    ASSERT_EQ( 1u, res );
}

#ifdef FIX_TEST_ON_SLOW_CI
TEST_F( RemovalNotifierTests, DeleteBatch )
{
    std::shared_ptr<Media> media[5];
    for ( auto i = 0u; i < 5; ++i )
    {
        media[i] = std::static_pointer_cast<Media>( ml->addMedia( std::string{ "media" } + std::to_string( i ) + ".avi" ) );
    }
    auto lock = cbMock->prepareWait();
    for ( auto i = 0u; i < 5; ++i )
        media[i]->removeFile( static_cast<File&>( *media[i]->files()[0] ) );
    auto res = cbMock->waitForNotif( std::move( lock ), std::chrono::seconds{ 1 } );
    ASSERT_EQ( 5u, res );
}
#endif
