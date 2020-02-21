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

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "Directory.h"
#include "File.h"
#include "utils/Filename.h"
#include "utils/Url.h"
#include "utils/VLCInstance.h"
#include "medialibrary/filesystem/Errors.h"
#include "logging/Logger.h"
#include "medialibrary/filesystem/IFileSystemFactory.h"

#include "compat/ConditionVariable.h"
#include "compat/Mutex.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
# include "utils/Charsets.h"
#endif

#include <vlcpp/vlc.hpp>

namespace medialibrary
{
namespace fs
{
namespace libvlc
{

Directory::Directory( const std::string& mrl, fs::IFileSystemFactory& fsFactory )
    : CommonDirectory( fsFactory )
    , m_mrl( utils::url::encode( utils::url::decode( mrl ) ) )
{
    // Add the final '/' in place if needed to avoid a useless copy in the init list
    utils::file::toFolderPath( m_mrl );
}

const std::string& Directory::mrl() const
{
    return m_mrl;
}

void Directory::read() const
{
    VLC::Media media( VLCInstance::get(), m_mrl, VLC::Media::FromLocation );
    assert( media.parsedStatus() != VLC::Media::ParsedStatus::Done );

    compat::Mutex mutex;
    compat::ConditionVariable cond;
    VLC::Media::ParsedStatus res = VLC::Media::ParsedStatus::Skipped;

    media.addOption( ":show-hiddenfiles=true" );
    media.addOption( ":ignore-filetypes=''" );

    auto eventHandler = media.eventManager().onParsedChanged(
        [&mutex, &cond, &res]( VLC::Media::ParsedStatus status) {
            std::lock_guard<compat::Mutex> lock( mutex );
            res = status;
            cond.notify_all();
        });

    std::unique_lock<compat::Mutex> lock( mutex );
    media.parseWithOptions( VLC::Media::ParseFlags::Network |
                            VLC::Media::ParseFlags::Local, -1 );
    bool success = cond.wait_for( lock, std::chrono::seconds{ 5 }, [&res]() {
        return res != VLC::Media::ParsedStatus::Skipped;
    });
    eventHandler->unregister();
    if ( success == false )
        throw errors::System{ ETIMEDOUT,
                              "Failed to browse network directory: Network is too slow" };
    if ( res == VLC::Media::ParsedStatus::Failed )
        throw errors::System{ EIO,
                              "Failed to browse network directory: Unknown error" };
    auto subItems = media.subitems();
    for ( auto i = 0; i < subItems->count(); ++i )
    {
        auto m = subItems->itemAtIndex( i );
        if ( m->type() == VLC::Media::Type::Directory )
            m_dirs.push_back( std::make_shared<Directory>( m->mrl(), m_fsFactory ) );
        else
        {
            addFile( m->mrl() );
        }
    }
}

void Directory::addFile( std::string mrl ) const
{
    uint32_t lastModificationDate = 0;
    int64_t fileSize = 0;

    if ( m_fsFactory.isNetworkFileSystem() == false )
    {
        auto path = utils::file::toLocalPath( mrl );

#ifdef _WIN32
        struct _stat64 s;
        if ( _wstat64( charset::ToWide( path.c_str() ).get(), &s ) != 0 )
        {
            LOG_ERROR( "Failed to get ", path, " stats" );
            throw errors::System{ errno, "Failed to get stats" };
        }
#else
        struct stat s;
        if ( lstat( path.c_str(), &s ) != 0 )
        {
            if ( errno == EACCES )
                return;
            // some Android devices will list folder content, but will yield
            // ENOENT when accessing those.
            // See https://trac.videolan.org/vlc/ticket/19909
            if ( errno == ENOENT )
            {
                LOG_WARN( "Ignoring unexpected ENOENT while listing folder content." );
                return;
            }
            LOG_ERROR( "Failed to get file ", mrl, " info" );
            throw errors::System{ errno, "Failed to get file info" };
        }
#endif
        lastModificationDate = s.st_mtime;
        fileSize = s.st_size;
    }
    m_files.push_back( std::make_shared<File>( std::move( mrl ),
                       m_fsFactory, lastModificationDate, fileSize ) );
}

}
}
}
