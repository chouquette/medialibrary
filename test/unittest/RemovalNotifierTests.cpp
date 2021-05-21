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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "UnitTests.h"

#include "Media.h"
#include "File.h"
#include "common/NoopCallback.h"
#include "utils/ModificationsNotifier.h"
#include "compat/Mutex.h"

class MockCallback : public mock::NoopCallback
{
public:
    MockCallback() : m_nbMedia( 0 ), m_nbTotalMedia( 0 ) {}
private:
    virtual void onMediaDeleted( std::set<int64_t> batch ) override
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        m_nbMedia = batch.size();
        m_nbTotalMedia += batch.size();
        m_cond.notify_all();
    }

public:
    void resetCount()
    {
        m_nbMedia = 0;
        m_nbTotalMedia = 0;
    }

    std::unique_lock<compat::Mutex> prepareWait()
    {
        std::unique_lock<compat::Mutex> lock{ m_lock };
        resetCount();
        return lock;
    }

    unsigned int waitForNotif( std::unique_lock<compat::Mutex>& preparedLock,
                               std::chrono::duration<int64_t> timeout, bool& hasTimedout )
    {
        hasTimedout = !m_cond.wait_for( preparedLock, timeout, [this]() {
            return m_nbMedia != 0;
        });

        return m_nbMedia;
    }

    uint32_t getNbTotalMediaDeleted()
    {
        std::lock_guard<compat::Mutex> lock( m_lock );
        return m_nbTotalMedia;
    }

private:
    compat::Mutex m_lock;
    compat::ConditionVariable m_cond;
    uint32_t m_nbMedia;
    uint32_t m_nbTotalMedia;
};

struct RemovalNotifierTests : public UnitTests<MockCallback>
{
    virtual void InstantiateMediaLibrary( const std::string& dbPath,
                                          const std::string& mlFolderDir ) override
    {
        ml.reset( new MediaLibraryWithNotifier ( dbPath, mlFolderDir ) );
    }
};

static void DeleteOne( RemovalNotifierTests* T )
{
    auto m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.avi", IMedia::Type::Video ) );
    auto lock = T->cbMock->prepareWait();
    bool hasTimedout;
    m->removeFile( static_cast<File&>( *m->files()[0] ) );
    // This media doesn't have any associated files, and should be removed by a sqlite hook
    // The notification will arrive "late", as it will need to timeout first
    auto res = T->cbMock->waitForNotif( lock,
        std::chrono::duration_cast<std::chrono::seconds>( std::chrono::milliseconds{ 2000 } ),
        hasTimedout );
    ASSERT_FALSE( hasTimedout );
    ASSERT_EQ( 1u, res );

    // Re-run a notification after the queues have been used before
    T->cbMock->resetCount();

    m = std::static_pointer_cast<Media>( T->ml->addMedia( "media.avi", IMedia::Type::Video ) );
    m->removeFile( static_cast<File&>( *m->files()[0] ) );

    // Wait for a notification for 500ms. It shouldn't arrive, and we should timeout
    res = T->cbMock->waitForNotif( lock,
        std::chrono::duration_cast<std::chrono::seconds>( std::chrono::milliseconds{ 500 } ),
        hasTimedout );
    ASSERT_TRUE( hasTimedout );
    ASSERT_EQ( 0u, res );

    // Wait again, now it should arrive.
    res = T->cbMock->waitForNotif( lock,
        std::chrono::duration_cast<std::chrono::seconds>( std::chrono::milliseconds{ 2000 } ),
        hasTimedout );
    ASSERT_FALSE( hasTimedout );
    ASSERT_EQ( 1u, res );
}

static void DeleteBatch( RemovalNotifierTests* T )
{
    for ( auto i = 0u; i < 10; ++i )
        T->ml->addMedia( std::string{ "media" } + std::to_string( i ) + ".mkv", IMedia::Type::Video );

    auto lock = T->cbMock->prepareWait();
    bool hasTimedout;

    for ( auto i = 0u; i < 10; ++i )
        T->ml->deleteMedia( i + 1 );

    // This media doesn't have any associated files, and should be removed by a sqlite hook
    // The notification will arrive "late", as it will need to timeout first
    uint32_t nbTotalNotified = 0;
    while ( nbTotalNotified != 10 )
    {
        auto nbNotified = T->cbMock->waitForNotif( lock,
            std::chrono::duration_cast<std::chrono::seconds>( std::chrono::milliseconds{ 2000 } ),
            hasTimedout );
        ASSERT_NE( 0u, nbNotified );
        ASSERT_FALSE( hasTimedout );
        ASSERT_TRUE( nbNotified != 1 || nbNotified + nbTotalNotified == 10 );
        nbTotalNotified += nbNotified;
        T->cbMock->resetCount();
    }
    ASSERT_EQ( 10u, nbTotalNotified );
}

static void Flush( RemovalNotifierTests* T )
{
    for ( auto i = 0u; i < 10; ++i )
        T->ml->addMedia( std::string{ "media" } + std::to_string( i ) + ".mkv", IMedia::Type::Video );

    for ( auto i = 0u; i < 10; ++i )
        T->ml->deleteMedia( i + 1 );

    // We can't lock here since flush is blocking until the callbacks have been called
    T->ml->getNotifier()->flush();
    auto nbMediaSignaled = T->cbMock->getNbTotalMediaDeleted();
    ASSERT_EQ( 10u, nbMediaSignaled );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( RemovalNotifierTests );

    ADD_TEST( DeleteOne );
    ADD_TEST( DeleteBatch );
    ADD_TEST( Flush );

    END_TESTS
}
