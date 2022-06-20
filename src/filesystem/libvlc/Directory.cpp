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

Directory::Directory( std::string mrl, fs::IFileSystemFactory& fsFactory )
    : CommonDirectory( fsFactory )
    , m_mrl( utils::file::toFolderPath(
                 utils::url::encode(
                     utils::url::decode( std::move( mrl ) ) ) ) )
{
}

const std::string& Directory::mrl() const
{
    return m_mrl;
}

void Directory::read() const
{
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
    VLC::Media media( m_mrl, VLC::Media::FromLocation );
    assert( media.parsedStatus( VLCInstance::get() ) != VLC::Media::ParsedStatus::Done );
#else
    VLC::Media media( VLCInstance::get(), m_mrl, VLC::Media::FromLocation );
    assert( media.parsedStatus() != VLC::Media::ParsedStatus::Done );
#endif

    compat::Mutex mutex;
    compat::ConditionVariable cond;
    VLC::Media::ParsedStatus res = VLC::Media::ParsedStatus::Skipped;

    media.addOption( ":show-hiddenfiles=true" );
    media.addOption( ":ignore-filetypes=''" );
    media.addOption( ":sub-autodetect-fuzzy=2" );

    auto eventHandler = media.eventManager().onParsedChanged(
        [&mutex, &cond, &res]( VLC::Media::ParsedStatus status) {
            std::lock_guard<compat::Mutex> lock( mutex );
            res = status;
            cond.notify_all();
        });

    bool success;
    {
        std::unique_lock<compat::Mutex> lock( mutex );
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
        media.parseRequest( VLCInstance::get(), VLC::Media::ParseFlags::Network |
                            VLC::Media::ParseFlags::Local, -1 );
#else
        media.parseWithOptions( VLC::Media::ParseFlags::Network |
                                VLC::Media::ParseFlags::Local, -1 );
#endif
        success = cond.wait_for( lock, std::chrono::seconds{ 5 }, [&res]() {
            return res != VLC::Media::ParsedStatus::Skipped;
        });
    }
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
        auto fileName = utils::file::fileName( m->mrl() );
        if ( fileName[0] == '.' )
        {
            /*
             * We need to expose the .nomedia file to the discoverer, but we don't
             * want to bother with hidden files & folders since they would be ignored
             * later on.
             * However, we consider something starting with '..' as a non-hidden
             * file or folder, see #218
             */
            if ( strcasecmp( fileName.c_str(), ".nomedia" ) != 0 &&
                 fileName.compare( 0, 2, ".." ) != 0 )
                continue;
        }
        if ( m->type() == VLC::Media::Type::Directory )
            m_dirs.push_back( std::make_shared<Directory>( m->mrl(), m_fsFactory ) );
        else
        {
            int64_t fileSize = 0;
            int64_t fileMtime = 0;
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(4, 0, 0, 0)
            auto fileSizeTpl = m->fileStat( VLC::Media::FileStat::Size );
            auto fileMtimeTpl = m->fileStat( VLC::Media::FileStat::Mtime );
            fileSize = std::get<1>( fileSizeTpl );
            fileMtime = std::get<1>( fileMtimeTpl );
#endif
            addFile( m->mrl(), IFile::LinkedFileType::None, {}, fileMtime, fileSize );
            for ( const auto& am : m->slaves() )
            {
                IFile::LinkedFileType linkedType;
                if ( am.type() == VLC::MediaSlave::Type::Audio )
                    linkedType = IFile::LinkedFileType::SoundTrack;
                else
                {
                    assert( am.type() == VLC::MediaSlave::Type::Subtitle );
                    linkedType = IFile::LinkedFileType::Subtitles;
                }
                addFile( am.uri(), linkedType, m->mrl(), 0, 0 );
            }
        }
    }
}

void Directory::addFile( std::string mrl, fs::IFile::LinkedFileType linkedType,
                         std::string linkedWith, time_t lastModificationDate,
                         int64_t fileSize ) const
{
    if ( m_fsFactory.isNetworkFileSystem() == false && lastModificationDate == 0 &&
         fileSize == 0 )
    {
        auto path = utils::url::toLocalPath( mrl );

#ifdef _WIN32
        /* We can't use _wstat here, see #323 */
        WIN32_FILE_ATTRIBUTE_DATA attributes;
        if ( GetFileAttributesExW( charset::ToWide( path.c_str() ).get(),
                                   GetFileExInfoStandard, &attributes ) == 0 )
        {
            LOG_ERROR( "Failed to get ", path, " attributes" );
            throw errors::System{ GetLastError(), "Failed to get stats" };
        }
        LARGE_INTEGER li;
        li.u.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
        li.u.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
        lastModificationDate = li.QuadPart / 10000000ULL - 11644473600ULL;
        if ( ( attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
            fileSize = 0;
        else
        {
            li.u.LowPart = attributes.nFileSizeLow;
            li.u.HighPart = attributes.nFileSizeHigh;
            fileSize = li.QuadPart;
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
        lastModificationDate = s.st_mtime;
        fileSize = s.st_size;
#endif
    }
    if ( linkedType == IFile::LinkedFileType::None )
        m_files.push_back( std::make_shared<File>( std::move( mrl ),
                           m_fsFactory, lastModificationDate, fileSize ) );
    else
        m_files.push_back( std::make_shared<File>( std::move( mrl ),
                           m_fsFactory, lastModificationDate, fileSize,
                           linkedType, std::move( linkedWith ) ) );
}

}
}
}
