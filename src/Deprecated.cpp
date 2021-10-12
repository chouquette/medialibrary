/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2021 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "Deprecated.h"
#include "Common.h"

#include "Media.h"
#include "Artist.h"
#include "Genre.h"

#include <cassert>

namespace medialibrary
{
const std::string AlbumTrack::Table::Name = "AlbumTrack";

std::string AlbumTrack::schema( const std::string& tableName, uint32_t )
{
    UNUSED_IN_RELEASE( tableName );

    assert( tableName == Table::Name );
    return "CREATE TABLE " + AlbumTrack::Table::Name +
    "("
         "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
         "media_id INTEGER UNIQUE,"
         "duration INTEGER NOT NULL,"
         "artist_id UNSIGNED INTEGER,"
         "genre_id INTEGER,"
         "track_number UNSIGNED INTEGER,"
         "album_id UNSIGNED INTEGER NOT NULL,"
         "disc_number UNSIGNED INTEGER,"
         "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "(id_media)"
             " ON DELETE CASCADE,"
         "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name + "(id_artist)"
             " ON DELETE CASCADE,"
         "FOREIGN KEY(genre_id) REFERENCES " + Genre::Table::Name + "(id_genre),"
         "FOREIGN KEY(album_id) REFERENCES Album(id_album) "
             " ON DELETE CASCADE"
    ")";
}

std::string AlbumTrack::index( AlbumTrack::Indexes index, uint32_t dbModel )
{
    switch ( index )
    {
        case Indexes::AlbumGenreArtist:
            return "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name +
                        "(album_id, genre_id, artist_id)";
        case Indexes::MediaArtistGenreAlbum:
            return  "CREATE INDEX " + indexName( index, dbModel ) +
                    " ON " + Table::Name +
                        "(media_id, artist_id, genre_id, album_id)";

    }
    return "<invalid request>";
}

std::string AlbumTrack::indexName(AlbumTrack::Indexes index, uint32_t )
{
    switch ( index )
    {
        case Indexes::AlbumGenreArtist:
            return "album_track_album_genre_artist_ids";
        case Indexes::MediaArtistGenreAlbum:
            return "album_media_artist_genre_album_idx";
        default:
            assert( !"Invalid index provided" );
    };
    return "<invalid request>";
}
}
