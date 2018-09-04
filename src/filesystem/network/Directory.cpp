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

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "Directory.h"
#include "File.h"
#include "utils/Filename.h"
#include "utils/VLCInstance.h"

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

#include <vlcpp/vlc.hpp>

namespace medialibrary
{
namespace fs
{

NetworkDirectory::NetworkDirectory( const std::string& mrl, fs::IFileSystemFactory& fsFactory )
    : CommonDirectory( fsFactory )
    , m_mrl( utils::file::toFolderPath( mrl ) )
{
}

const std::string& NetworkDirectory::mrl() const
{
    return m_mrl;
}

void NetworkDirectory::read() const
{
    VLC::Media media( VLCInstance::get(), m_mrl, VLC::Media::FromLocation );
    assert( media.parsedStatus() != VLC::Media::ParsedStatus::Done );

    compat::Mutex mutex;
    compat::ConditionVariable cond;
    VLC::Media::ParsedStatus res = VLC::Media::ParsedStatus::Skipped;

    media.eventManager().onParsedChanged([&mutex, &cond, &res]( VLC::Media::ParsedStatus status) {
        std::lock_guard<compat::Mutex> lock( mutex );
        res = status;
        cond.notify_all();
    });

    std::unique_lock<compat::Mutex> lock( mutex );
    media.parseWithOptions( VLC::Media::ParseFlags::Network |
                            VLC::Media::ParseFlags::Local, -1 );
    bool timeout = cond.wait_for( lock, std::chrono::seconds{ 5 }, [res]() {
        return res != VLC::Media::ParsedStatus::Skipped;
    });
    if ( timeout == true )
        throw std::system_error( ETIMEDOUT, std::generic_category(),
                                 "Failed to browse network directory: Network is too slow" );
    if ( res == VLC::Media::ParsedStatus::Failed )
        throw std::system_error( EIO, std::generic_category(),
                                 "Failed to browse network directory: Unknown error" );
    auto subItems = media.subitems();
    for ( auto i = 0; i < subItems->count(); ++i )
    {
        auto m = subItems->itemAtIndex( i );
        if ( m->type() == VLC::Media::Type::Directory )
            m_dirs.push_back( std::make_shared<fs::NetworkDirectory>( m->mrl(), m_fsFactory ) );
        else
            m_files.push_back( std::make_shared<fs::NetworkFile>( m->mrl() ) );
    }
}

}
}
