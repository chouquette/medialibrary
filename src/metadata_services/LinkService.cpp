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
# include "config.h"
#endif

#include "LinkService.h"
#include "logging/Logger.h"
#include "MediaLibrary.h"
#include "Playlist.h"
#include "Media.h"

namespace medialibrary
{
namespace parser
{

Status LinkService::run( IItem& item )
{
    switch ( item.linkType() )
    {
        case IItem::LinkType::NoLink:
            LOG_ERROR( "Processing a task which is not a linking task from a "
                       "linking service" );
            return Status::Fatal;
        case IItem::LinkType::Media:
            return Status::Fatal; //Not implemented yet
        case IItem::LinkType::Playlist:
            return linkToPlaylist( item );
    }
    assert( false );
    return Status::Fatal;
}

const char* LinkService::name() const
{
    return "linking";
}

Step LinkService::targetedStep() const
{
    return Step::Linking;
}

bool LinkService::initialize( IMediaLibrary* ml )
{
    m_ml = static_cast<MediaLibrary*>( ml );
    return true;
}

void LinkService::onFlushing()
{
}

void LinkService::onRestarted()
{
}

void LinkService::stop()
{
}

Status LinkService::linkToPlaylist(IItem& item)
{
    auto media = m_ml->media( item.mrl() );
    // If the media isn't present yet, we assume it wasn't created yet. Let's
    // try to link it later
    if ( media == nullptr )
        return Status::Requeue;
    auto playlist = Playlist::fetch( m_ml, item.linkToId() );
    if ( playlist == nullptr )
        return Status::Fatal;
    if ( playlist->add( *media, item.linkExtra() ) == false )
        return Status::Fatal;
    // Explicitely mark the task as completed, as there is nothing more to run.
    // This shouldn't be needed, but requires a better handling of multiple pipeline.
    return Status::Completed;
}

}
}
