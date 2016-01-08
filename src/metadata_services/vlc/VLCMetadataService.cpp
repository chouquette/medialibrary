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

#include <chrono>

#include "VLCMetadataService.h"
#include "Media.h"

VLCMetadataService::VLCMetadataService( const VLC::Instance& vlc )
    : m_instance( vlc )
{
}

parser::Task::Status VLCMetadataService::run( parser::Task& task )
{
    auto media = task.media;
    auto file = task.file;
    // FIXME: This is now becomming an invalid predicate
    if ( media->duration() != -1 )
    {
        LOG_INFO( file->mrl(), " was already parsed" );
        return parser::Task::Status::Success;
    }

    LOG_INFO( "Parsing ", file->mrl() );
    auto chrono = std::chrono::steady_clock::now();

    task.vlcMedia = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    bool done = false;

    auto event = task.vlcMedia.eventManager().onParsedChanged([this, &done](bool parsed) {
        if ( parsed == false )
            return;
        std::lock_guard<std::mutex> lock( m_mutex );
        done = true;
        m_cond.notify_all();
    });
    task.vlcMedia.parseAsync();
    auto success = m_cond.wait_for( lock, std::chrono::seconds( 5 ), [&done]() { return done == true; } );
    event->unregister();
    if ( success == false )
        return parser::Task::Status::Fatal;
    auto duration = std::chrono::steady_clock::now() - chrono;
    LOG_DEBUG("VLC parsing done in ", std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
    return parser::Task::Status::Success;
}

const char* VLCMetadataService::name() const
{
    return "VLC";
}

