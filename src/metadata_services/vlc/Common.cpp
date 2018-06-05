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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include "Common.hpp"
#include "Media.h"
#include "utils/Filename.h"
namespace medialibrary
{

bool medialibrary::MetadataCommon::startPlayback( VLC::Media& media, VLC::MediaPlayer& mp )
{
    // Use a copy of the event manager to automatically unregister all events as soon
    // as we leave this method.
    bool hasVideoTrack = false;
    bool failedToStart = false;
    bool hasAnyTrack = false;
    bool success = false;
    auto em = mp.eventManager();
    compat::Mutex mutex;
    compat::ConditionVariable cond;

    em.onESAdded([&mutex, &cond, &hasVideoTrack, &hasAnyTrack]( libvlc_track_type_t type, int ) {
        std::lock_guard<compat::Mutex> lock( mutex );
        if ( type == libvlc_track_video )
            hasVideoTrack = true;
        hasAnyTrack = true;
        cond.notify_all();
    });
    em.onEncounteredError([&mutex, &cond, &failedToStart]() {
        std::lock_guard<compat::Mutex> lock( mutex );
        failedToStart = true;
        cond.notify_all();
    });

    bool metaArtworkChanged = false;
    bool watchForArtworkChange = false;
    auto mem = media.eventManager();
    if ( utils::file::schemeIs( "attachment", media.meta( libvlc_meta_ArtworkURL ) ) == true )
    {
        watchForArtworkChange = true;
        mem.onMetaChanged([&mutex, &cond, &metaArtworkChanged, &media]( libvlc_meta_t meta ) {
            if ( meta != libvlc_meta_ArtworkURL
                 || metaArtworkChanged == true
                 || utils::file::schemeIs( "attachment", media.meta( libvlc_meta_ArtworkURL ) ) == true )
                return;
            std::lock_guard<compat::Mutex> lock( mutex );
            metaArtworkChanged = true;
            cond.notify_all();
        });
    }

    std::unique_lock<compat::Mutex> lock( mutex );
    mp.play();
    success = cond.wait_for( lock, std::chrono::seconds( 3 ), [&failedToStart, &hasAnyTrack]() {
        return failedToStart == true || hasAnyTrack == true;
    });

    // In case the playback failed, we probably won't fetch anything interesting anyway.
    if ( failedToStart == true || success == false )
        return false;

    // If we have any kind of track, but not a video track, we don't have to wait long, tracks are usually
    // being discovered together.
    if ( hasVideoTrack == false )
    {
        if ( watchForArtworkChange == true )
        {
            cond.wait_for( lock, std::chrono::milliseconds( 500 ), [&metaArtworkChanged]() {
                return metaArtworkChanged == true;
            });
        }
        else
        {
            cond.wait_for( lock, std::chrono::seconds( 1 ), [&hasVideoTrack]() {
                return hasVideoTrack == true;
            });
        }
    }

    return true;
}


}
