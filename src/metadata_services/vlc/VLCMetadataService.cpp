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
#ifdef __ANDROID__
# include <cstdlib>
#endif

#include "VLCMetadataService.h"
#include "Media.h"
#include "utils/VLCInstance.h"

VLCMetadataService::VLCMetadataService()
    : m_instance( VLCInstance::get() )
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

    auto vlcMedia = VLC::Media( m_instance, file->mrl(), VLC::Media::FromPath );

    std::unique_lock<std::mutex> lock( m_mutex );
    VLC::Media::ParseStatus status;

    auto event = vlcMedia.eventManager().onParsedStatus([this, &status](VLC::Media::ParseStatus s) {
        std::lock_guard<std::mutex> lock( m_mutex );
        status = s;
        m_cond.notify_all();
    });
    if ( vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local ) == false )
        return parser::Task::Status::Fatal;
    auto success = m_cond.wait_for( lock, std::chrono::seconds( 5 ), [&status]() {
        return status == VLC::Media::ParseStatus::Done ||
                status == VLC::Media::ParseStatus::Skipped ||
                status == VLC::Media::ParseStatus::Failed;
    });
    event->unregister();
    if ( success == false || status != VLC::Media::ParseStatus::Done )
        return parser::Task::Status::Fatal;
    auto tracks = vlcMedia.tracks();
    if ( tracks.size() == 0 )
    {
        LOG_ERROR( "Failed to fetch any tracks" );
        return parser::Task::Status::Fatal;
    }
    for ( const auto& track : tracks )
    {
        auto codec = track.codec();
        std::string fcc( (const char*)&codec, 4 );
        if ( track.type() == VLC::MediaTrack::Video )
        {
            auto fps = (float)track.fpsNum() / (float)track.fpsDen();
            task.videoTracks.emplace_back( fcc, fps, track.width(), track.height() );
        }
        else if ( track.type() == VLC::MediaTrack::Audio )
        {
            task.audioTracks.emplace_back( fcc, track.bitrate(), track.rate(), track.channels(),
                                            track.language(), track.description() );
        }
    }
    storeMeta( task, vlcMedia );
    auto duration = std::chrono::steady_clock::now() - chrono;
    LOG_DEBUG("VLC parsing done in ", std::chrono::duration_cast<std::chrono::microseconds>( duration ).count(), "µs" );
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

void VLCMetadataService::storeMeta( parser::Task& task, VLC::Media& vlcMedia )
{
#if LIBVLC_VERSION_INT >= LIBVLC_VERSION(3, 0, 0, 0)
    task.albumArtist = vlcMedia.meta( libvlc_meta_AlbumArtist );
    task.discNumber = toInt( vlcMedia, libvlc_meta_DiscNumber, "disc number" );
    task.discTotal = toInt( vlcMedia, libvlc_meta_DiscTotal, "disc total" );
#else
    task.discNumber = 0;
    task.discTotal = 0;
#endif
    task.artist = vlcMedia.meta( libvlc_meta_Artist );
    task.artworkMrl = vlcMedia.meta( libvlc_meta_ArtworkURL );
    task.title = vlcMedia.meta( libvlc_meta_Title );
    task.genre = vlcMedia.meta( libvlc_meta_Genre );
    task.releaseDate = vlcMedia.meta( libvlc_meta_Date );
    task.showName = vlcMedia.meta( libvlc_meta_ShowName );
    task.albumName = vlcMedia.meta( libvlc_meta_Album );
    task.duration = vlcMedia.duration();

    task.trackNumber = toInt( vlcMedia, libvlc_meta_TrackNumber, "track number" );
    task.episode = toInt( vlcMedia, libvlc_meta_Episode, "episode number" );
}

int VLCMetadataService::toInt( VLC::Media& vlcMedia, libvlc_meta_t meta, const char* name )
{
    auto str = vlcMedia.meta( meta );
    if ( str.empty() == false )
    {
#ifndef __ANDROID__
        try
        {
            return std::stoi( str );
        }
        catch( std::logic_error& ex)
        {
            LOG_WARN( "Invalid ", name, " provided (", str, "): ", ex.what() );
        }
#else
        return atoi( str.c_str() );
#endif
    }
    return 0;
}

