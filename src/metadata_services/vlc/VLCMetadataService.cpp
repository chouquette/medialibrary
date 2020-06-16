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

#include "VLCMetadataService.h"
#include "utils/VLCInstance.h"
#include "metadata_services/vlc/Common.hpp"
#include "utils/Filename.h"
#include "logging/Logger.h"
#include "utils/Url.h"

namespace medialibrary
{
namespace parser
{

VLCMetadataService::VLCMetadataService()
    : m_instance( VLCInstance::get() )
{
}

bool VLCMetadataService::initialize( IMediaLibrary* )
{
    return true;
}

Status VLCMetadataService::run( IItem& item )
{
    auto mrl = item.mrl();

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
        {
            // We need m_currentMedia to be updated from a locked context
            // but we also need parseWithOption to be called with the lock
            // unlocked, to avoid a potential lock inversion with VLC's internal
            // mutexes.
            std::unique_lock<compat::Mutex> lock( m_mutex );
            m_currentMedia = vlcMedia;
        }

        if ( vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local |
                                        VLC::Media::ParseFlags::Network, 5000 ) == false )
        {
            std::unique_lock<compat::Mutex> lock( m_mutex );
            m_currentMedia = VLC::Media{};
            return Status::Fatal;
        }
        std::unique_lock<compat::Mutex> lock( m_mutex );
        m_cond.wait( lock, [&done]() {
            return done == true;
        });
        m_currentMedia = VLC::Media{};
    }
    event->unregister();
    if ( status == VLC::Media::ParsedStatus::Failed || status == VLC::Media::ParsedStatus::Timeout )
        return Status::Fatal;
    auto artworkMrl = vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( item.fileType() == IFile::Type::Playlist &&
         vlcMedia.subitems()->count() == 0 )
    {
        LOG_DEBUG( "Discarding playlist file with no subitem: ", mrl );
        return Status::Fatal;
    }
    if ( utils::file::schemeIs( "attachment://", artworkMrl ) == true )
    {
        LOG_WARN( "Artwork for ", mrl, " is an attachment. Falling back to playback" );
        VLC::MediaPlayer mp( vlcMedia );
        auto res = MetadataCommon::startPlayback( vlcMedia, mp, m_mutex, m_cond );
        if ( res == false )
            return Status::Fatal;
    }
    mediaToItem( vlcMedia, item );
    return Status::Success;
}

const char* VLCMetadataService::name() const
{
    return "VLC";
}

void VLCMetadataService::onFlushing()
{
}

void VLCMetadataService::onRestarted()
{
}

Step VLCMetadataService::targetedStep() const
{
    return Step::MetadataExtraction;
}

void VLCMetadataService::stop()
{
    std::lock_guard<compat::Mutex> lock{ m_mutex };
    if ( m_currentMedia.isValid() )
        m_currentMedia.parseStop();
}

void VLCMetadataService::mediaToItem( VLC::Media& media, IItem& item )
{
    item.setMeta( IItem::Metadata::Title,
                  media.meta( libvlc_meta_Title ) );
    item.setMeta( IItem::Metadata::ArtworkUrl,
                  media.meta( libvlc_meta_ArtworkURL ) );
    item.setMeta( IItem::Metadata::ShowName,
                  media.meta( libvlc_meta_ShowName ) );
    item.setMeta( IItem::Metadata::Episode,
                  media.meta( libvlc_meta_Episode ) );
    item.setMeta( IItem::Metadata::Album,
                  media.meta( libvlc_meta_Album ) );
    item.setMeta( IItem::Metadata::Genre,
                  media.meta( libvlc_meta_Genre ) );
    item.setMeta( IItem::Metadata::Date,
                  media.meta( libvlc_meta_Date ) );
    item.setMeta( IItem::Metadata::AlbumArtist,
                  media.meta( libvlc_meta_AlbumArtist ) );
    item.setMeta( IItem::Metadata::Artist,
                  media.meta( libvlc_meta_Artist ) );
    item.setMeta( IItem::Metadata::TrackNumber,
                  media.meta( libvlc_meta_TrackNumber ) );
    item.setMeta( IItem::Metadata::DiscNumber,
                  media.meta( libvlc_meta_DiscNumber ) );
    item.setMeta( IItem::Metadata::DiscTotal,
                  media.meta( libvlc_meta_DiscTotal ) );
    item.setDuration( media.duration() );

#if LIBVLC_VERSION_INT < LIBVLC_VERSION(4, 0, 0, 0)
    auto tracks = media.tracks();
#else
    std::vector<VLC::MediaTrack> tracks;
    auto trackList = media.tracks( VLC::MediaTrack::Type::Audio );
    std::move( begin( trackList ), end( trackList ), std::back_inserter( tracks ) );
    trackList = media.tracks( VLC::MediaTrack::Type::Video );
    std::move( begin( trackList ), end( trackList ), std::back_inserter( tracks ) );
    trackList = media.tracks( VLC::MediaTrack::Type::Subtitle );
    std::move( begin( trackList ), end( trackList ), std::back_inserter( tracks ) );
#endif
    for ( const auto& track : tracks )
    {
        IItem::Track t;

        if ( track.type() == VLC::MediaTrack::Type::Audio )
        {
            t.type = IItem::Track::Type::Audio;
            t.a.nbChannels = track.channels();
            t.a.rate = track.rate();
        }
        else if ( track.type() == VLC::MediaTrack::Type::Video )
        {
            t.type = IItem::Track::Type::Video;
            t.v.fpsNum = track.fpsNum();
            t.v.fpsDen = track.fpsDen();
            t.v.width = track.width();
            t.v.height = track.height();
            t.v.sarNum = track.sarNum();
            t.v.sarDen = track.sarDen();
        }
        else if ( track.type() == VLC::MediaTrack::Type::Subtitle )
        {
            t.type = IItem::Track::Type::Subtitle;
            t.language = track.language();
            t.description = track.description();
            const auto& enc = track.encoding();
            strncpy( t.s.encoding, enc.c_str(), sizeof(t.s.encoding) - 1 );
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
            auto mrl = vlcMedia->mrl();
            mrl = utils::url::encode( utils::url::decode( mrl ) );
            IItem& subItem = item.createSubItem( std::move( mrl ), i );
            mediaToItem( *vlcMedia, subItem );
        }
    }
}

}
}
