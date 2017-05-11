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

#include <chrono>

#include "VLCMetadataService.h"
#include "Media.h"
#include "utils/VLCInstance.h"

namespace medialibrary
{

VLCMetadataService::VLCMetadataService()
    : m_instance( VLCInstance::get() )
{
}

parser::Task::Status VLCMetadataService::run( parser::Task& task )
{
    auto file = task.file;

    LOG_INFO( "Parsing ", file->mrl() );

    // Having a valid media means we're re-executing this parser after the thumbnailer,
    // which isn't expected, as we always mark this task as completed.
    assert( task.vlcMedia.isValid() == false );
    task.vlcMedia = VLC::Media( m_instance, file->mrl(), VLC::Media::FromType::FromLocation );

    std::unique_lock<compat::Mutex> lock( m_mutex );
    VLC::Media::ParsedStatus status;
    bool done = false;

    auto event = task.vlcMedia.eventManager().onParsedChanged( [this, &status, &done](VLC::Media::ParsedStatus s ) {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        status = s;
        done = true;
        m_cond.notify_all();
    });
    if ( task.vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local | VLC::Media::ParseFlags::Network |
                                         VLC::Media::ParseFlags::FetchLocal, 5000 ) == false )
        return parser::Task::Status::Fatal;
    m_cond.wait( lock, [&status, &done]() {
        return done == true;
    });
    event->unregister();
    if ( status == VLC::Media::ParsedStatus::Failed || status == VLC::Media::ParsedStatus::Timeout )
        return parser::Task::Status::Fatal;
    auto tracks = task.vlcMedia.tracks();
    if ( tracks.size() == 0 )
        LOG_WARN( "Failed to fetch any tracks for ", file->mrl() );
    // Don't save the file parsing step yet, since all data are just in memory. Just mark
    // the extraction as done.
    task.file->markStepCompleted( File::ParserStep::MetadataExtraction );
    return parser::Task::Status::Success;
}

const char* VLCMetadataService::name() const
{
    return "VLC";
}

uint8_t VLCMetadataService::nbThreads() const
{
    return 1;
}

bool VLCMetadataService::isCompleted( const parser::Task& task ) const
{
    // We always need to run this task if the metadata extraction isn't completed
    return task.vlcMedia.isValid() == true;
}

}
