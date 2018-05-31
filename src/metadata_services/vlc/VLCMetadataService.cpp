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
#include "metadata_services/vlc/Common.hpp"
#include "utils/Filename.h"

namespace medialibrary
{

VLCMetadataService::VLCMetadataService()
    : m_ml( nullptr )
    , m_instance( VLCInstance::get() )
{
}

bool VLCMetadataService::initialize( MediaLibrary* ml )
{
    m_ml = ml;
    return true;
}

parser::Task::Status VLCMetadataService::run( parser::Task& task )
{
    auto mrl = task.mrl;
    LOG_INFO( "Parsing ", mrl );

    // Having a valid media means we're re-executing this parser after the thumbnailer,
    // which isn't expected, as we always mark this task as completed.
    assert( task.vlcMedia.isValid() == false );
    task.vlcMedia = VLC::Media( m_instance, mrl, VLC::Media::FromType::FromLocation );

    VLC::Media::ParsedStatus status;
    bool done = false;

    auto event = task.vlcMedia.eventManager().onParsedChanged( [this, &status, &done](VLC::Media::ParsedStatus s ) {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        status = s;
        done = true;
        m_cond.notify_all();
    });
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );

        if ( task.vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local | VLC::Media::ParseFlags::Network |
                                             VLC::Media::ParseFlags::FetchLocal, 5000 ) == false )
            return parser::Task::Status::Fatal;
        m_cond.wait( lock, [&status, &done]() {
            return done == true;
        });
    }
    event->unregister();
    if ( status == VLC::Media::ParsedStatus::Failed || status == VLC::Media::ParsedStatus::Timeout )
        return parser::Task::Status::Fatal;
    auto tracks = task.vlcMedia.tracks();
    auto artworkMrl = task.vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( ( tracks.size() == 0 && task.vlcMedia.subitems()->count() == 0 ) ||
         utils::file::schemeIs( "attachment://", artworkMrl ) == true )
    {
        if ( tracks.size() == 0 && task.vlcMedia.subitems()->count() == 0 )
            LOG_WARN( "Failed to fetch any tracks for ", mrl, ". Falling back to playback" );
        VLC::MediaPlayer mp( task.vlcMedia );
        auto res = MetadataCommon::startPlayback( task.vlcMedia, mp );
        if ( res == false )
            return parser::Task::Status::Fatal;
    }
    task.item().setMeta( parser::Task::Item::Metadata::Title,
                  task.vlcMedia.meta( libvlc_meta_Title ) );
    task.item().setMeta( parser::Task::Item::Metadata::ArtworkUrl,
                  task.vlcMedia.meta( libvlc_meta_ArtworkURL ) );
    task.item().setMeta( parser::Task::Item::Metadata::ShowName,
                  task.vlcMedia.meta( libvlc_meta_ShowName ) );
    task.item().setMeta( parser::Task::Item::Metadata::Episode,
                  task.vlcMedia.meta( libvlc_meta_Episode ) );
    task.item().setMeta( parser::Task::Item::Metadata::Album,
                  task.vlcMedia.meta( libvlc_meta_Album ) );
    task.item().setMeta( parser::Task::Item::Metadata::Genre,
                  task.vlcMedia.meta( libvlc_meta_Genre ) );
    task.item().setMeta( parser::Task::Item::Metadata::Date,
                  task.vlcMedia.meta( libvlc_meta_Date ) );
    task.item().setMeta( parser::Task::Item::Metadata::AlbumArtist,
                  task.vlcMedia.meta( libvlc_meta_AlbumArtist ) );
    task.item().setMeta( parser::Task::Item::Metadata::Artist,
                  task.vlcMedia.meta( libvlc_meta_Artist ) );
    task.item().setMeta( parser::Task::Item::Metadata::TrackNumber,
                  task.vlcMedia.meta( libvlc_meta_TrackNumber ) );
    task.item().setMeta( parser::Task::Item::Metadata::DiscNumber,
                  task.vlcMedia.meta( libvlc_meta_DiscNumber ) );
    task.item().setMeta( parser::Task::Item::Metadata::DiscTotal,
                  task.vlcMedia.meta( libvlc_meta_DiscTotal ) );
    // Don't save the file parsing step yet, since all data are just in memory. Just mark
    // the extraction as done.
    task.markStepCompleted( parser::Task::ParserStep::MetadataExtraction );
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

void VLCMetadataService::onFlushing()
{
}

void VLCMetadataService::onRestarted()
{
}

}
