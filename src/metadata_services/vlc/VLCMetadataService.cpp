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
    auto mrl = task.item().mrl();
    LOG_INFO( "Parsing ", mrl );

    // Having a valid media means we're re-executing this parser after the thumbnailer,
    // which isn't expected, as we always mark this task as completed.
    VLC::Media vlcMedia{ m_instance, mrl, VLC::Media::FromType::FromLocation };

    VLC::Media::ParsedStatus status;
    bool done = false;

    auto event = vlcMedia.eventManager().onParsedChanged( [this, &status, &done](VLC::Media::ParsedStatus s ) {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        status = s;
        done = true;
        m_cond.notify_all();
    });
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );

        if ( vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local | VLC::Media::ParseFlags::Network |
                                             VLC::Media::ParseFlags::FetchLocal, 5000 ) == false )
            return parser::Task::Status::Fatal;
        m_cond.wait( lock, [&status, &done]() {
            return done == true;
        });
    }
    event->unregister();
    if ( status == VLC::Media::ParsedStatus::Failed || status == VLC::Media::ParsedStatus::Timeout )
        return parser::Task::Status::Fatal;
    auto tracks = vlcMedia.tracks();
    auto artworkMrl = vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( ( tracks.size() == 0 && vlcMedia.subitems()->count() == 0 ) ||
         utils::file::schemeIs( "attachment://", artworkMrl ) == true )
    {
        if ( tracks.size() == 0 && vlcMedia.subitems()->count() == 0 )
            LOG_WARN( "Failed to fetch any tracks for ", mrl, ". Falling back to playback" );
        VLC::MediaPlayer mp( vlcMedia );
        auto res = MetadataCommon::startPlayback( vlcMedia, mp );
        if ( res == false )
            return parser::Task::Status::Fatal;
    }
    mediaToItem( vlcMedia, task.item() );
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

bool VLCMetadataService::isCompleted( const parser::Task& ) const
{
    // We always need to run this task if the metadata extraction isn't completed
    return false;
}

void VLCMetadataService::onFlushing()
{
}

void VLCMetadataService::onRestarted()
{
}

parser::Task::ParserStep VLCMetadataService::targetedStep() const
{
    return parser::Task::ParserStep::MetadataExtraction;
}

void VLCMetadataService::mediaToItem( VLC::Media& media, parser::Task::Item& item )
{
    item.setMeta( parser::Task::Item::Metadata::Title,
                  media.meta( libvlc_meta_Title ) );
    item.setMeta( parser::Task::Item::Metadata::ArtworkUrl,
                  media.meta( libvlc_meta_ArtworkURL ) );
    item.setMeta( parser::Task::Item::Metadata::ShowName,
                  media.meta( libvlc_meta_ShowName ) );
    item.setMeta( parser::Task::Item::Metadata::Episode,
                  media.meta( libvlc_meta_Episode ) );
    item.setMeta( parser::Task::Item::Metadata::Album,
                  media.meta( libvlc_meta_Album ) );
    item.setMeta( parser::Task::Item::Metadata::Genre,
                  media.meta( libvlc_meta_Genre ) );
    item.setMeta( parser::Task::Item::Metadata::Date,
                  media.meta( libvlc_meta_Date ) );
    item.setMeta( parser::Task::Item::Metadata::AlbumArtist,
                  media.meta( libvlc_meta_AlbumArtist ) );
    item.setMeta( parser::Task::Item::Metadata::Artist,
                  media.meta( libvlc_meta_Artist ) );
    item.setMeta( parser::Task::Item::Metadata::TrackNumber,
                  media.meta( libvlc_meta_TrackNumber ) );
    item.setMeta( parser::Task::Item::Metadata::DiscNumber,
                  media.meta( libvlc_meta_DiscNumber ) );
    item.setMeta( parser::Task::Item::Metadata::DiscTotal,
                  media.meta( libvlc_meta_DiscTotal ) );
    item.setDuration( media.duration() );

    auto tracks = media.tracks();
    for ( const auto& track : tracks )
    {
        parser::Task::Item::Track t;

        if ( track.type() == VLC::MediaTrack::Type::Audio )
        {
            t.type = parser::Task::Item::Track::Type::Audio;
            t.a.nbChannels = track.channels();
            t.a.rate = track.rate();
        }
        else if ( track.type() == VLC::MediaTrack::Type::Video )
        {
            t.type = parser::Task::Item::Track::Type::Video;
            t.v.fpsNum = track.fpsNum();
            t.v.fpsDen = track.fpsDen();
            t.v.width = track.width();
            t.v.height = track.height();
            t.v.sarNum = track.sarNum();
            t.v.sarDen = track.sarDen();
        }
        else
            continue;
        auto codec = track.codec();
        std::string fcc( reinterpret_cast<const char*>( &codec ), 4 );
        t.codec = std::move( fcc );

        t.bitrate = track.bitrate();
        t.language = track.language();
        t.description = track.description();

        item.addTrack( std::move( t ) );
    }

    auto subItems = media.subitems();
    if ( subItems != nullptr )
    {
        for ( auto i = 0; i < subItems->count(); ++i )
        {
            auto vlcMedia = subItems->itemAtIndex( i );
            assert( vlcMedia != nullptr );
            // Always add 1 to the playlist/subitem index, as 0 is an invalid index
            // in this context
            parser::Task::Item& subItem = item.createSubItem( vlcMedia->mrl(), i + 1u );
            mediaToItem( *vlcMedia, subItem );
        }
    }
}

}
