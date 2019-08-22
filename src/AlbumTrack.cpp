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

#include "AlbumTrack.h"
#include "Album.h"
#include "Artist.h"
#include "Media.h"
#include "Genre.h"
#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "logging/Logger.h"

namespace medialibrary
{

const std::string AlbumTrack::Table::Name = "AlbumTrack";
const std::string AlbumTrack::Table::PrimaryKeyColumn = "id_track";
int64_t AlbumTrack::* const AlbumTrack::Table::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    , m_id( row.load<decltype(m_id)>( 0 ) )
    , m_mediaId( row.load<decltype(m_mediaId)>( 1 ) )
    // Skip duration
    , m_artistId( row.load<decltype(m_artistId)>( 3 ) )
    , m_genreId( row.load<decltype(m_genreId)>( 4 ) )
    , m_trackNumber( row.load<decltype(m_trackNumber)>( 5 ) )
    , m_albumId( row.load<decltype(m_albumId)>( 6 ) )
    , m_discNumber( row.load<decltype(m_discNumber)>( 7 ) )
{
}

AlbumTrack::AlbumTrack( MediaLibraryPtr ml, int64_t mediaId, int64_t artistId, int64_t genreId,
                        unsigned int trackNumber, int64_t albumId, unsigned int discNumber )
    : m_ml( ml )
    , m_id( 0 )
    , m_mediaId( mediaId )
    , m_artistId( artistId )
    , m_genreId( genreId )
    , m_trackNumber( trackNumber )
    , m_albumId( albumId )
    , m_discNumber( discNumber )
{
}

int64_t AlbumTrack::id() const
{
    return m_id;
}

ArtistPtr AlbumTrack::artist() const
{
    if ( m_artistId == 0 )
        return nullptr;
    if ( m_artist == nullptr )
        m_artist = Artist::fetch( m_ml, m_artistId );
    return m_artist;
}

int64_t AlbumTrack::artistId() const
{
    return m_artistId;
}

void AlbumTrack::createTable( sqlite::Connection* dbConnection )
{
    sqlite::Tools::executeRequest( dbConnection, schema( Table::Name,
                                                         Settings::DbModelVersion ) );
}

void AlbumTrack::createTriggers(sqlite::Connection* dbConnection)
{
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS "
            "album_media_artist_genre_album_idx ON " +
            AlbumTrack::Table::Name +
            "(media_id, artist_id, genre_id, album_id)";
    const std::string indexAlbumIdReq = "CREATE INDEX IF NOT EXISTS album_track_album_genre_artist_ids "
            "ON " + AlbumTrack::Table::Name + "(album_id, genre_id, artist_id)";
    sqlite::Tools::executeRequest( dbConnection, indexReq );
    sqlite::Tools::executeRequest( dbConnection, indexAlbumIdReq );
}

std::string AlbumTrack::schema( const std::string& tableName, uint32_t )
{
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

bool AlbumTrack::checkDbModel(MediaLibraryPtr ml)
{
    return sqlite::Tools::checkSchema( ml->getConn(),
                                       schema( Table::Name, Settings::DbModelVersion ),
                                       Table::Name );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create( MediaLibraryPtr ml, int64_t albumId,
                                                std::shared_ptr<Media> media, unsigned int trackNb,
                                                unsigned int discNumber, int64_t artistId, int64_t genreId,
                                                int64_t duration )
{
    auto self = std::make_shared<AlbumTrack>( ml, media->id(), artistId, genreId, trackNb, albumId, discNumber );
    static const std::string req = "INSERT INTO " + AlbumTrack::Table::Name
            + "(media_id, duration, artist_id, genre_id, track_number, album_id, disc_number) VALUES(?, ?, ?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, media->id(), duration >= 0 ? duration : 0, sqlite::ForeignKey( artistId ),
                 sqlite::ForeignKey( genreId ), trackNb, albumId, discNumber ) == false )
        return nullptr;
    return self;
}

AlbumTrackPtr AlbumTrack::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + AlbumTrack::Table::Name +
            " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

Query<IMedia> AlbumTrack::fromGenre( MediaLibraryPtr ml, int64_t genreId,
                                     bool withThumbnail, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m"
            " INNER JOIN " + AlbumTrack::Table::Name + " t ON m.id_media = t.media_id"
            " WHERE t.genre_id = ?1 AND m.is_present = 1";
    if ( withThumbnail == true )
    {
        req += " AND EXISTS(SELECT entity_id FROM " + Thumbnail::LinkingTable::Name +
               " WHERE entity_id = m.id_media AND entity_type = ?2)";
    }
    std::string orderBy = "ORDER BY ";
    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
    case SortingCriteria::Duration:
        orderBy += "m.duration";
        break;
    case SortingCriteria::InsertionDate:
        orderBy += "m.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        orderBy += "m.release_date";
        break;
    case SortingCriteria::Alpha:
        orderBy += "m.title";
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default" );
        /* fall-through */
    case SortingCriteria::Default:
        if ( desc == true )
            orderBy += "t.artist_id DESC, t.album_id DESC, t.disc_number DESC, t.track_number DESC, m.filename";
        else
            orderBy += "t.artist_id, t.album_id, t.disc_number, t.track_number, m.filename";
        break;
    }

    if ( desc == true )
        orderBy += " DESC";
    if ( withThumbnail == true )
        return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                          std::move( orderBy ), genreId,
                                          Thumbnail::EntityType::Media );
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      std::move( orderBy ), genreId );
}

GenrePtr AlbumTrack::genre()
{
    if ( m_genre == nullptr )
        m_genre = Genre::fetch( m_ml, m_genreId );
    return m_genre;
}

int64_t AlbumTrack::genreId() const
{
    return m_genreId;
}

unsigned int AlbumTrack::trackNumber() const
{
    return m_trackNumber;
}

unsigned int AlbumTrack::discNumber() const
{
    return m_discNumber;
}

std::shared_ptr<IAlbum> AlbumTrack::album()
{
    // "Fail" early in case there's no album to fetch
    if ( m_albumId == 0 )
        return nullptr;

    auto album = m_album.lock();
    if ( album == nullptr )
    {
        album = Album::fetch( m_ml, m_albumId );
        m_album = album;
    }
    return album;
}

int64_t AlbumTrack::albumId() const
{
    return m_albumId;
}

}
