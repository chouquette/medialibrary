/******************* Migrate Media table **************************************/
"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER,"
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail TEXT,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"INSERT INTO " + Thumbnail::Table::Name + "(id_thumbnail, mrl, origin) "
    "SELECT id_media, thumbnail, " +
    std::to_string( static_cast<ThumbnailType>( Thumbnail::Origin::UserProvided ) ) +
    " FROM " + Media::Table::Name + " WHERE thumbnail IS NOT NULL AND thumbnail != ''",

"DROP TABLE " + Media::Table::Name,

#include "database/tables/Media_v14.sql"

"INSERT INTO " + Media::Table::Name + "("
    "id_media, type, subtype, duration, play_count, last_played_date, real_last_played_date, insertion_date,"
    "release_date, thumbnail_id, thumbnail_generated, title, filename, is_favorite,"
    "is_present) "
"SELECT id_media, type, ifnull(subtype, " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                    IMedia::SubType::Unknown ) )
    + "), duration, play_count, last_played_date,"
    "strftime('%s', 'now'),"
    "insertion_date, release_date, "
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE id_media END,"
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE 1 END,"
    "title, filename, is_favorite, is_present FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

/******************* Populate new media.nb_playlists **************************/

"UPDATE " + Media::Table::Name + " SET nb_playlists = "
"(SELECT COUNT(media_id) FROM PlaylistMediaRelation WHERE media_id = id_media )"
"WHERE id_media IN (SELECT media_id FROM PlaylistMediaRelation)",

/************ Playlist external media were stored as Unknown ******************/

"UPDATE " + Media::Table::Name + " SET type = " +
std::to_string( static_cast<typename std::underlying_type<IMedia::Type>::type>(
            IMedia::Type::External ) ) + " "
"WHERE nb_playlists > 0 AND "
"type = " + std::to_string( static_cast<typename std::underlying_type<IMedia::Type>::type>(
IMedia::Type::Unknown ) ),

/******************* Migrate metadata table ***********************************/
"CREATE TEMPORARY TABLE " + Metadata::Table::Name + "_backup"
"("
    "id_media INTEGER,"
    "type INTEGER,"
    "value TEXT"
")",

"INSERT INTO " + Metadata::Table::Name + "_backup SELECT * FROM MediaMetadata",

"DROP TABLE MediaMetadata",

// Recreate the new table
#include "database/tables/Metadata_v14.sql"

"INSERT INTO " + Metadata::Table::Name + " "
"SELECT "
    "id_media, " + std::to_string(
        static_cast<typename std::underlying_type<IMetadata::EntityType>::type>(
            IMetadata::EntityType::Media ) ) +
    ", type, value "
"FROM " + Metadata::Table::Name + "_backup",

"DROP TABLE " + Metadata::Table::Name + "_backup",

/******************* Migrate the playlist table *******************************/
"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    + Playlist::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT"
")",

"CREATE TEMPORARY TABLE PlaylistMediaRelation_backup"
"("
    "media_id INTEGER,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup SELECT * FROM " + Playlist::Table::Name,
"INSERT INTO PlaylistMediaRelation_backup SELECT * FROM PlaylistMediaRelation",

"DROP TABLE " + Playlist::Table::Name,
"DROP TABLE PlaylistMediaRelation",

#include "database/tables/Playlist_v14.sql"

"INSERT INTO " + Playlist::Table::Name + " SELECT * FROM " + Playlist::Table::Name + "_backup",
"INSERT INTO PlaylistMediaRelation SELECT media_id, NULL, playlist_id, position "
    "FROM PlaylistMediaRelation_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",
"DROP TABLE PlaylistMediaRelation_backup",

/******************* Migrate Device table *************************************/

"CREATE TEMPORARY TABLE " + Device::Table::Name + "_backup"
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT UNIQUE ON CONFLICT FAIL,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN"
")",

"INSERT INTO " + Device::Table::Name + "_backup SELECT * FROM " + Device::Table::Name,

"DROP TABLE " + Device::Table::Name,

#include "database/tables/Device_v14.sql"

"INSERT INTO " + Device::Table::Name + " SELECT id_device, uuid, scheme, is_removable, is_present,"
    "strftime('%s', 'now') FROM " + Device::Table::Name + "_backup",

"DROP TABLE " + Device::Table::Name + "_backup",

/******************* Delete removed triggers **********************************/

"DROP TRIGGER on_track_genre_changed",

/******************* Delete other tables **************************************/

"DROP TABLE " + Album::Table::Name,
"DELETE FROM " + Album::Table::Name + "Fts",
"DROP TABLE " + Artist::Table::Name,
"DELETE FROM " + Artist::Table::Name + "Fts",
"DELETE FROM MediaArtistRelation",
"DROP TABLE " + Movie::Table::Name,
"DROP TABLE " + Show::Table::Name,
// No need to delete the ShowFts table since... it didn't exist
"DROP TABLE " + VideoTrack::Table::Name,
// Flush the audio track table to recreate all tracks
"DELETE FROM " + AudioTrack::Table::Name,

// History table & its triggers were removed for good:
"DROP TABLE History",
