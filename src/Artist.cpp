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

#include "Artist.h"
#include "Album.h"
#include "AlbumTrack.h"
#include "Media.h"

#include "database/SqliteTools.h"

const std::string policy::ArtistTable::Name = "Artist";
const std::string policy::ArtistTable::PrimaryKeyColumn = "id_artist";
unsigned int Artist::*const policy::ArtistTable::PrimaryKey = &Artist::m_id;


Artist::Artist( DBConnection dbConnection, sqlite::Row& row )
    : m_dbConnection( dbConnection )
{
    row >> m_id
        >> m_name
        >> m_shortBio
        >> m_artworkMrl
        >> m_nbAlbums
        >> m_mbId
        >> m_isPresent;
}

Artist::Artist( const std::string& name )
    : m_id( 0 )
    , m_name( name )
    , m_nbAlbums( 0 )
    , m_isPresent( true )
{
}

unsigned int Artist::id() const
{
    return m_id;
}

const std::string &Artist::name() const
{
    return m_name;
}

const std::string &Artist::shortBio() const
{
    return m_shortBio;
}

bool Artist::setShortBio(const std::string &shortBio)
{
    static const std::string req = "UPDATE " + policy::ArtistTable::Name
            + " SET shortbio = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, shortBio, m_id ) == false )
        return false;
    m_shortBio = shortBio;
    return true;
}

std::vector<AlbumPtr> Artist::albums() const
{
    if ( m_id == 0 )
        return {};
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name + " alb "
            "WHERE artist_id = ? ORDER BY release_year, title";
    return Album::fetchAll<IAlbum>( m_dbConnection, req, m_id );
}

std::vector<MediaPtr> Artist::media() const
{
    static const std::string req = "SELECT med.* FROM " + policy::MediaTable::Name + " med "
            "INNER JOIN MediaArtistRelation mar ON mar.media_id = med.id_media "
            "WHERE mar.artist_id = ? AND med.is_present = 1";
    return Media::fetchAll<IMedia>( m_dbConnection, req, m_id );
}

bool Artist::addMedia( Media& media )
{
    static const std::string req = "INSERT INTO MediaArtistRelation VALUES(?, ?)";
    // If track's ID is 0, the request will fail due to table constraints
    sqlite::ForeignKey artistForeignKey( m_id );
    return sqlite::Tools::insert( m_dbConnection, req, media.id(), artistForeignKey ) != 0;
}

const std::string& Artist::artworkMrl() const
{
    return m_artworkMrl;
}

bool Artist::setArtworkMrl( const std::string& artworkMrl )
{
    if ( m_artworkMrl == artworkMrl )
        return true;
    static const std::string req = "UPDATE " + policy::ArtistTable::Name +
            " SET artwork_mrl = ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, artworkMrl, m_id ) == false )
        return false;
    m_artworkMrl = artworkMrl;
    return true;
}

bool Artist::updateNbAlbum( int increment )
{
    assert( increment != 0 );
    assert( increment > 0 || ( increment < 0 && m_nbAlbums >= 1 ) );

    static const std::string req = "UPDATE " + policy::ArtistTable::Name +
            " SET nb_albums = nb_albums + ? WHERE id_artist = ?";
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, increment, m_id ) == false )
        return false;
    m_nbAlbums += increment;
    return true;
}

std::shared_ptr<Album> Artist::unknownAlbum()
{
    static const std::string req = "SELECT * FROM " + policy::AlbumTable::Name +
                        " WHERE artist_id = ? AND title IS NULL";
    auto album = Album::fetch( m_dbConnection, req, m_id );
    if ( album == nullptr )
    {
        album = Album::createUnknownAlbum( m_dbConnection, this );
        if ( album == nullptr )
            return nullptr;
        if ( updateNbAlbum( 1 ) == false )
        {
            Album::destroy( m_dbConnection, album->id() );
            return nullptr;
        }
    }
    return album;
}

const std::string& Artist::musicBrainzId() const
{
    return m_mbId;
}

bool Artist::setMusicBrainzId( const std::string& mbId )
{
    static const std::string req = "UPDATE " + policy::ArtistTable::Name
            + " SET mb_id = ? WHERE id_artist = ?";
    if ( mbId == m_mbId )
        return true;
    if ( sqlite::Tools::executeUpdate( m_dbConnection, req, mbId, m_id ) == false )
        return false;
    m_mbId = mbId;
    return true;
}

bool Artist::createTable( DBConnection dbConnection )
{
    static const std::string req = "CREATE TABLE IF NOT EXISTS " +
            policy::ArtistTable::Name +
            "("
                "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
                "shortbio TEXT,"
                "artwork_mrl TEXT,"
                "nb_albums UNSIGNED INT DEFAULT 0,"
                "mb_id TEXT,"
                "is_present BOOLEAN NOT NULL DEFAULT 1"
            ")";
    static const std::string reqRel = "CREATE TABLE IF NOT EXISTS MediaArtistRelation("
                "media_id INTEGER NOT NULL,"
                "artist_id INTEGER,"
                "PRIMARY KEY (media_id, artist_id),"
                "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name +
                "(id_media) ON DELETE CASCADE,"
                "FOREIGN KEY(artist_id) REFERENCES " + policy::ArtistTable::Name + "("
                    + policy::ArtistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
            ")";
    static const std::string reqFts = "CREATE VIRTUAL TABLE IF NOT EXISTS " +
                policy::ArtistTable::Name + "Fts USING FTS3("
                "name"
            ")";
    return sqlite::Tools::executeRequest( dbConnection, req ) &&
            sqlite::Tools::executeRequest( dbConnection, reqRel ) &&
            sqlite::Tools::executeRequest( dbConnection, reqFts );
}

bool Artist::createTriggers(DBConnection dbConnection)
{
    static const std::string triggerReq = "CREATE TRIGGER IF NOT EXISTS has_album_present AFTER UPDATE OF "
            "is_present ON " + policy::AlbumTable::Name +
            " BEGIN "
            " UPDATE " + policy::ArtistTable::Name + " SET is_present="
                "(SELECT COUNT(id_album) FROM " + policy::AlbumTable::Name + " WHERE artist_id=new.artist_id AND is_present=1) "
                "WHERE id_artist=new.artist_id;"
            " END";
    static const std::string ftsInsertTrigger = "CREATE TRIGGER IF NOT EXISTS insert_artist_fts"
            " AFTER INSERT ON " + policy::ArtistTable::Name +
            " BEGIN"
            " INSERT INTO " + policy::ArtistTable::Name + "Fts(rowid,name) VALUES(new.id_artist, new.name);"
            " END";
    static const std::string ftsDeleteTrigger = "CREATE TRIGGER IF NOT EXISTS delete_artist_fts"
            " BEFORE DELETE ON " + policy::ArtistTable::Name +
            " BEGIN"
            " DELETE FROM " + policy::ArtistTable::Name + "Fts WHERE rowid=old.id_artist;"
            " END";
    return sqlite::Tools::executeRequest( dbConnection, triggerReq ) &&
            sqlite::Tools::executeRequest( dbConnection, ftsInsertTrigger ) &&
            sqlite::Tools::executeRequest( dbConnection, ftsDeleteTrigger );
}

bool Artist::createDefaultArtists( DBConnection dbConnection )
{
    // Don't rely on Artist::create, since we want to insert or do nothing here.
    // This will skip the cache for those new entities, but they will be inserted soon enough anyway.
    static const std::string req = "INSERT OR IGNORE INTO " + policy::ArtistTable::Name +
            "(id_artist) VALUES(?),(?)";
    sqlite::Tools::insert( dbConnection, req, medialibrary::UnknownArtistID,
                                          medialibrary::VariousArtistID );
    // Always return true. The insertion might succeed, but we consider it a failure when 0 row
    // gets inserted, while we are explicitely specifying "OR IGNORE" here.
    return true;
}

std::shared_ptr<Artist> Artist::create( DBConnection dbConnection, const std::string &name )
{
    auto artist = std::make_shared<Artist>( name );
    static const std::string req = "INSERT INTO " + policy::ArtistTable::Name +
            "(id_artist, name) VALUES(NULL, ?)";
    if ( insert( dbConnection, artist, req, name ) == false )
        return nullptr;
    artist->m_dbConnection = dbConnection;
    return artist;
}

std::vector<ArtistPtr> Artist::search( DBConnection dbConnection, const std::string& name )
{
    static const std::string req = "SELECT * FROM " + policy::ArtistTable::Name + " WHERE id_artist IN "
            "(SELECT rowid FROM " + policy::ArtistTable::Name + "Fts WHERE name MATCH ?)"
            "AND is_present != 0";
    return fetchAll<IArtist>( dbConnection, req, name + "*" );
}

