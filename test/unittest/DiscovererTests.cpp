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

#include "UnitTests.h"

#include "discoverer/DiscovererWorker.h"

class DiscovererWorkerTest : public DiscovererWorker
{
public:
    DiscovererWorkerTest() = default;
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

struct DiscovererTests : public Tests
{
    std::unique_ptr<DiscovererWorkerTest> discoverer;

    virtual void SetUp( const std::string& testSuite,
                        const std::string& testName ) override
    {
        InitTestFolder( testSuite, testName );
        /* We won't use the database in these tests but need a valid medialib instance */
        InstantiateMediaLibrary( "/no/such/file.db", "/or/directory" );
        discoverer.reset( new DiscovererWorkerTest() );
    }

    virtual void TearDown() override
    {
        ml.reset();
    }
};

static void SimpleEnqueue( DiscovererTests* T )
{
    T->discoverer->discover( "file:///test/" );
    const auto& tasks = T->discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::AddEntryPoint, tasks[0].type );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Reload, tasks[1].type );
}

static void FilterDoubleEnqueue( DiscovererTests* T )
{
    T->discoverer->discover( "file:///test/" );
    T->discoverer->discover( "file:///test/" );
    T->discoverer->discover( "file:///test/" );
    const auto& tasks = T->discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::AddEntryPoint, tasks[0].type );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Reload, tasks[1].type );
}

static void DontFilterUnrelatedDoubleEnqueue( DiscovererTests* T )
{
    T->discoverer->discover( "file:///sea/" );
    T->discoverer->discover( "file:///otter/" );
    const auto& tasks = T->discoverer->tasks();
    ASSERT_EQ( 4u, tasks.size() );
}

static void ReduceDiscoverRemove( DiscovererTests* T )
{
    T->discoverer->discover( "file:///test/" );
    T->discoverer->remove( "file:///test/" );
    auto tasks = T->discoverer->tasks();
    ASSERT_EQ( 0u, tasks.size() );

    T->discoverer->remove( "file:///test/" );
    T->discoverer->discover( "file:///test/" );

    tasks = T->discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::AddEntryPoint, tasks[0].type );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Reload, tasks[1].type );
}

static void ReduceBanUnban( DiscovererTests* T )
{
   T->discoverer->ban( "file:///test/" );
   T->discoverer->unban( "file:///test/" );
   const auto& tasks = T->discoverer->tasks();
   ASSERT_EQ( 0u, tasks.size() );
}

static void InterruptDiscover( DiscovererTests* T )
{
    T->discoverer->discover( "file:///path/" );
    T->discoverer->simulateWorkerProcessing();
    T->discoverer->ban( "file:///path/" );
    auto tasks = T->discoverer->tasks();
    ASSERT_EQ( 1u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Ban, tasks[0].type );
}

static void InterruptReload( DiscovererTests* T )
{
    T->discoverer->reload();
    T->discoverer->simulateWorkerProcessing();
    T->discoverer->remove( "file:///path/to/otters/" );
    auto tasks = T->discoverer->tasks();
    ASSERT_EQ( 2u, tasks.size() );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Remove, tasks[0].type );
    ASSERT_EQ( DiscovererWorkerTest::Task::Type::Reload, tasks[1].type );
    ASSERT_EQ( "", tasks[1].entryPoint );
}

int main( int ac, char** av )
{
    INIT_TESTS_C( DiscovererTests );

    ADD_TEST( SimpleEnqueue );
    ADD_TEST( FilterDoubleEnqueue );
    ADD_TEST( DontFilterUnrelatedDoubleEnqueue );
    ADD_TEST( ReduceDiscoverRemove );
    ADD_TEST( ReduceBanUnban );
    ADD_TEST( InterruptDiscover );
    ADD_TEST( InterruptReload );

    END_TESTS
}
