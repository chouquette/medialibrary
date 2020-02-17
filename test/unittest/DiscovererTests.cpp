/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2020 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Tests.h"

#include "discoverer/DiscovererWorker.h"

class DiscovererWorkerTest : public DiscovererWorker
{
public:
    using DiscovererWorker::DiscovererWorker;
    virtual void notify() override
    {
    }

    std::vector<Task> tasks()
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        return std::vector<Task>{ begin( m_tasks ), end( m_tasks ) };
    }

    void simulateWorkerProcessing()
    {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        m_fakeRunningTask = m_tasks.front();
        m_tasks.pop_front();
        m_currentTask = &m_fakeRunningTask;
    }

    using DiscovererWorker::Task;

private:
    Task m_fakeRunningTask;
};

class DiscovererTests : public Tests
{
public:
    std::unique_ptr<DiscovererWorkerTest> discoverer;

    virtual void SetUp() override
    {
        InstantiateMediaLibrary();
        discoverer.reset( new DiscovererWorkerTest( ml.get(), nullptr ) );
    }
};


TEST_F( DiscovererTests, SimpleEnqueue )
{
    discoverer->discover( "file:///test/" );
    const auto& tasks = discoverer->tasks();
    ASSERT_EQ( 1u, tasks.size() );
}

TEST_F( DiscovererTests, FilterDoubleEnqueue )
{
    discoverer->discover( "file:///test/" );
    discoverer->discover( "file:///test/" );
    discoverer->discover( "file:///test/" );
    const auto& tasks = discoverer->tasks();
    ASSERT_EQ( 1u, tasks.size() );
}

TEST_F( DiscovererTests, DontFilterUnrelatedDoubleEnqueue )
{
    discoverer->discover( "file:///sea/" );
    discoverer->discover( "file:///otter/" );
    const auto& tasks = discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
}

TEST_F( DiscovererTests, ReduceDiscoverRemove )
{
    discoverer->discover( "file:///test/" );
    discoverer->remove( "file:///test/" );
    auto tasks = discoverer->tasks();
    ASSERT_EQ( 0u, tasks.size() );

    discoverer->remove( "file:///test/" );
    discoverer->discover( "file:///test/" );

    tasks = discoverer->tasks();
    ASSERT_EQ( 1u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Discover, tasks[0].type );
}

TEST_F( DiscovererTests, ReduceBanUnban )
{
   discoverer->ban( "file:///test/" );
   discoverer->unban( "file:///test/" );
   const auto& tasks = discoverer->tasks();
   ASSERT_EQ( 0u, tasks.size() );
}

TEST_F( DiscovererTests, InterruptDiscover )
{
    discoverer->discover( "file:///path/" );
    discoverer->simulateWorkerProcessing();
    discoverer->ban( "file:///path/" );
    auto tasks = discoverer->tasks();
    ASSERT_EQ( 1u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Ban, tasks[0].type );
}

TEST_F( DiscovererTests, InterruptReload )
{
    discoverer->reload();
    discoverer->simulateWorkerProcessing();
    discoverer->remove( "file:///path/to/otters/" );
    auto tasks = discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Remove, tasks[0].type );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Reload, tasks[1].type );
    ASSERT_EQ( "", tasks[1].entryPoint );

}
