/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "medialibrary/IMediaLibrary.h"
#include "test/common/NoopCallback.h"
#include "test/common/util.h"
#include "compat/Mutex.h"
#include "compat/ConditionVariable.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "medialibrary/filesystem/Errors.h"

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <unistd.h>
#include <cassert>

class TestCb : public mock::NoopCallback
{
public:
    explicit TestCb( bool generateThumbnails )
        : m_isDiscoveryCompleted( false )
        , m_isParsingCompleted( false )
        , m_isIdle( false )
        , m_error( false )
        , m_generateThumbnails( generateThumbnails )
        , m_nbThumbnails( 0 )
    {
    }

    bool waitForCompletion()
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_cond.wait( lock, [this](){
            return (m_isDiscoveryCompleted == true &&
                    m_isParsingCompleted == true &&
                    m_isIdle == true) || m_error;
        });
        return m_error == false;
    }

    void waitForThumbnails()
    {
        std::unique_lock<compat::Mutex> lock{ m_mutex };
        m_thumbnailsCond.wait( lock, [this]() {
            return m_nbThumbnails == 0;
        });
    }

private:
    virtual void onDiscoveryStarted() override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isDiscoveryCompleted = false;
            m_isParsingCompleted = false;
        }
        m_cond.notify_all();
    }

    virtual void onDiscoveryCompleted() override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isDiscoveryCompleted = true;
        }
        m_cond.notify_all();
    }

    virtual void onDiscoveryFailed( const std::string& ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_error = true;
        }
        m_cond.notify_all();
    }

    virtual void onParsingStatsUpdated( uint32_t done, uint32_t scheduled ) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isParsingCompleted = (done == scheduled);
        }
        m_cond.notify_all();
    }
    virtual void onBackgroundTasksIdleChanged(bool isIdle) override
    {
        {
            std::lock_guard<compat::Mutex> lock( m_mutex );
            m_isIdle = isIdle;
        }
        m_cond.notify_all();
    }

    virtual void onMediaAdded( std::vector<MediaPtr> media ) override
    {
        if ( m_generateThumbnails == false )
            return;
        for ( const auto& m : media )
        {
            if ( m->type() != IMedia::Type::Video )
                continue;
            {
                std::unique_lock<compat::Mutex> lock( m_mutex );
                m_nbThumbnails++;
            }
            auto res = m->requestThumbnail( ThumbnailSizeType::Thumbnail, 320, 0, 0.3f );
            assert( res == true );
        }
    }

    virtual void onMediaThumbnailReady( MediaPtr media, ThumbnailSizeType, bool success ) override
    {
        if ( success == false )
        {
            std::cerr << "Failed to generate thumbnail for media "
                      << media->id() << std::endl;
            return;
        }
        std::unique_lock<compat::Mutex> lock( m_mutex );
        assert( m_nbThumbnails != 0 );
        --m_nbThumbnails;
        if ( m_nbThumbnails == 0 && m_isParsingCompleted == true
             && m_isDiscoveryCompleted == true )
        {
            m_thumbnailsCond.notify_all();
        }
    }

private:
    compat::ConditionVariable m_cond;
    compat::Mutex m_mutex;
    bool m_isDiscoveryCompleted;
    bool m_isParsingCompleted;
    bool m_isIdle;
    bool m_error;
    bool m_generateThumbnails;
    size_t m_nbThumbnails;
    compat::ConditionVariable m_thumbnailsCond;
};

static void usage(const char* const* argv)
{
    std::cerr << "usage: " << argv[0] << "[-q] [-n X] [-t] <entrypoint|database>\n"
                 "-q: Use Error log level. Default is Debug\n"
                 "-n X: Run X discover of the provided entrypoint\n"
                 "-t: Generate thumbnails for discovered videos\n"
                 "-m: Migrate the provided database in-place.\n"
                 "-r: When used in combination with -m, it will reload "
                     "the database after it's been migrated\n\n"
                 "When using -m the required argument is an existing database to migrate."
              << std::endl;
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        usage(argv);
        exit(1);
    }

    auto mlDir = getTempPath( "discoverer_test" );
    auto dbPath = mlDir + "/test.db";
    auto nbRuns = 1;
    auto quiet = false;
    auto migrate = false;
    auto reload = false;
    auto thumbnails = false;

    int opt;
    while ( ( opt = getopt(argc, argv, "qn:mrt") ) != -1 )
    {
        switch ( opt )
        {
            case 'q':
                quiet = true;
                break;
            case 'n':
                nbRuns = atoi(optarg);
                break;
            case 'm':
                migrate = true;
                break;
            case 'r':
                reload = true;
                break;
            case 't':
                thumbnails = true;
                break;
            default:
                usage(argv);
                exit(1);
        }
    }
    if ( reload == true && migrate == false )
    {
        std::cerr << "-r is only valid when -m is also provided" << std::endl;
        usage( argv );
        exit( 1 );
    }

    if ( optind >= argc )
    {
        std::cerr << "Missing entry point" << std::endl;
        usage(argv);
        exit(2);
    }

    auto entrypoint = argv[optind];
    std::string target;
    if ( migrate == false )
    {
        try
        {
            utils::url::scheme( entrypoint );
            target = entrypoint;
        }
        catch ( const medialibrary::fs::errors::UnhandledScheme& )
        {
            target = utils::file::toMrl( entrypoint );
        }

        unlink( dbPath.c_str() );
    }
    else
        dbPath = entrypoint;

    auto testCb = std::make_unique<TestCb>(thumbnails);
    std::unique_ptr<medialibrary::IMediaLibrary> ml{
        NewMediaLibrary( dbPath.c_str(), mlDir.c_str(), false, nullptr )
    };

    ml->setVerbosity( quiet == true ? medialibrary::LogLevel::Error :
                                      medialibrary::LogLevel::Debug );
    auto initRes = ml->initialize( testCb.get() );
    assert( initRes == InitializeResult::Success );
    auto res = ml->setDiscoverNetworkEnabled( true );
    assert( res );
    if ( migrate == true )
    {
        if ( reload == true )
        {
            ml->reload();
            res = testCb->waitForCompletion();
            if ( res == false )
                return 1;
        }
        return 0;
    }

    for ( auto i = 0; i < nbRuns; ++i )
    {
        ml->discover( target );
        res = testCb->waitForCompletion();
        if ( res == false )
            return 1;
        if ( i < nbRuns - 1 )
            ml->forceRescan();
    }
    if ( thumbnails == true )
    {
        testCb->waitForThumbnails();
    }
    return 0;
}
