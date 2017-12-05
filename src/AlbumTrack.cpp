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

#include "AlbumTrack.h"
#include "Album.h"
#include "Artist.h"
#include "Media.h"
#include "Genre.h"
#include "database/SqliteTools.h"
#include "logging/Logger.h"

namespace medialibrary
{

const std::string policy::AlbumTrackTable::Name = "AlbumTrack";
const std::string policy::AlbumTrackTable::PrimaryKeyColumn = "id_track";
int64_t AlbumTrack::* const policy::AlbumTrackTable::PrimaryKey = &AlbumTrack::m_id;

AlbumTrack::AlbumTrack( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
{
    int64_t dummyDuration;
    row >> m_id
        >> m_mediaId
        >> dummyDuration
        >> m_artistId
        >> m_genreId
        >> m_trackNumber
        >> m_albumId
        >> m_discNumber
        >> m_isPresent;
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
    , m_isPresent( true )
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
    auto lock = m_artist.lock();
    if ( m_artist.isCached() == false )
    {
        m_artist = Artist::fetch( m_ml, m_artistId );
    }
    return m_artist.get();
}

bool AlbumTrack::setArtist( std::shared_ptr<Artist> artist )
{
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name +
            " SET artist_id = ? WHERE id_track = ?";
    if ( artist->id() == m_artistId )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, artist->id(), m_id ) == false )
        return false;
    m_artistId = artist->id();
    m_artist = artist;
    return true;
}

bool AlbumTrack::createTable( sqlite::Connection* dbConnection )
{
    const std::string req = "CREATE TABLE IF NOT EXISTS " + policy::AlbumTrackTable::Name + "("
                "id_track INTEGER PRIMARY KEY AUTOINCREMENT,"
                "media_id INTEGER,"
                "duration INTEGER NOT NULL,"
                "artist_id UNSIGNED INTEGER,"
                "genre_id INTEGER,"
                "track_number UNSIGNED INTEGER,"
                "album_id UNSIGNED INTEGER NOT NULL,"
                "disc_number UNSIGNED INTEGER,"
                "is_present BOOLEAN NOT NULL DEFAULT 1,"
                "FOREIGN KEY (media_id) REFERENCES " + policy::MediaTable::Name + "(id_media)"
                    " ON DELETE CASCADE,"
                "FOREIGN KEY (artist_id) REFERENCES " + policy::ArtistTable::Name + "(id_artist)"
                    " ON DELETE CASCADE,"
                "FOREIGN KEY (genre_id) REFERENCES " + policy::GenreTable::Name + "(id_genre),"
                "FOREIGN KEY (album_id) REFERENCES Album(id_album) "
                    " ON DELETE CASCADE"
            ")";
    const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS is_track_present"
            "AFTER UPDATE OF is_present "
            "ON " + policy::MediaTable::Name + " "
            "BEGIN "
            "UPDATE " + policy::AlbumTrackTable::Name + " "
                "SET is_present = new.is_present WHERE media_id = new.id_media;"
            "END";
    const std::string indexReq = "CREATE INDEX IF NOT EXISTS "
            "album_media_artist_genre_album_idx ON " +
            policy::AlbumTrackTable::Name +
            "(media_id, artist_id, genre_id, album_id)";

    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, indexReq );
}

std::shared_ptr<AlbumTrack> AlbumTrack::create( MediaLibraryPtr ml, int64_t albumId,
                                                std::shared_ptr<Media> media, unsigned int trackNb,
                                                unsigned int discNumber, int64_t artistId, int64_t genreId,
                                                int64_t duration )
{
    auto self = std::make_shared<AlbumTrack>( ml, media->id(), artistId, genreId, trackNb, albumId, discNumber );
    static const std::string req = "INSERT INTO " + policy::AlbumTrackTable::Name
            + "(media_id, duration, artist_id, genre_id, track_number, album_id, disc_number) VALUES(?, ?, ?, ?, ?, ?, ?)";
    if ( insert( ml, self, req, media->id(), duration >= 0 ? duration : 0, sqlite::ForeignKey( artistId ),
                 sqlite::ForeignKey( genreId ), trackNb, albumId, discNumber ) == false )
        return nullptr;
    self->m_media = media;
    return self;
}

AlbumTrackPtr AlbumTrack::fromMedia( MediaLibraryPtr ml, int64_t mediaId )
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTrackTable::Name +
            " WHERE media_id = ?";
    return fetch( ml, req, mediaId );
}

std::vector<MediaPtr> AlbumTrack::fromGenre( MediaLibraryPtr ml, int64_t genreId, SortingCriteria sort, bool desc )
{
    std::string req = "SELECT m.* FROM " + policy::MediaTable::Name + " m"
            " INNER JOIN " + policy::AlbumTrackTable::Name + " t ON m.id_media = t.media_id"
            " WHERE t.genre_id = ? ORDER BY ";
    switch ( sort )
    {
    case SortingCriteria::Duration:
        req += "m.duration";
        break;
    case SortingCriteria::InsertionDate:
        req += "m.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        req += "m.release_date";
        break;
    case SortingCriteria::Alpha:
        req += "m.title";
        break;
    default:
        if ( desc == true )
            req += "t.artist_id DESC, t.album_id DESC, t.disc_number DESC, t.track_number DESC, m.filename";
        else
            req += "t.artist_id, t.album_id, t.disc_number, t.track_number, m.filename";
        break;
    }

    if ( desc == true )
        req += " DESC";
    return Media::fetchAll<IMedia>( ml, req, genreId );
}

GenrePtr AlbumTrack::genre()
{
    auto l = m_genre.lock();
    if ( m_genre.isCached() == false )
    {
        m_genre = Genre::fetch( m_ml, m_genreId );
    }
    return m_genre.get();
}

bool AlbumTrack::setGenre( std::shared_ptr<Genre> genre )
{
    // We need to fetch the old genre entity now, in case it gets deleted through
    // the nbTracks reaching 0 trigger.
    if ( m_genreId > 0 )
    {
        auto l = m_genre.lock();
        if ( m_genre.isCached() == false )
            m_genre = Genre::fetch( m_ml, m_genreId );
    }
    static const std::string req = "UPDATE " + policy::AlbumTrackTable::Name
            + " SET genre_id = ? WHERE id_track = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req,
                                       sqlite::ForeignKey( genre != nullptr ? genre->id() : 0 ),
                                       m_id ) == false )
        return false;
    {
        auto l = m_genre.lock();
        if ( m_genreId > 0 )
            m_genre.get()->updateCachedNbTracks( -1 );
        m_genre = genre;
    }
    if ( genre != nullptr )
    {
        genre->updateCachedNbTracks( 1 );
        m_genreId = genre->id();
    }
    else
        m_genreId = 0;
    return true;
}

unsigned int AlbumTrack::trackNumber()
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

    auto lock = m_album.lock();
    if ( m_album.isCached() == false )
        m_album = Album::fetch( m_ml, m_albumId );
    return m_album.get().lock();
}

std::shared_ptr<IMedia> AlbumTrack::media()
{
    auto lock = m_media.lock();
    if ( m_media.isCached() == false )
    {
        m_media = Media::fetch( m_ml, m_mediaId );
    }
    return m_media.get().lock();
}

}
