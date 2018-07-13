/******************* Migrate Media table **************************************/
"CREATE TEMPORARY TABLE " + MediaTable::Name + "_backup("
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

"INSERT INTO " + MediaTable::Name + "_backup SELECT * FROM " + MediaTable::Name,

"INSERT INTO " + ThumbnailTable::Name + "(id_thumbnail, mrl, origin) "
    "SELECT id_media, thumbnail, " +
    std::to_string( static_cast<ThumbnailType>( Thumbnail::Origin::UserProvided ) ) +
    " FROM " + MediaTable::Name + " WHERE thumbnail IS NOT NULL AND thumbnail != ''",

"DROP TABLE " + MediaTable::Name,

#include "database/tables/Media_v14.sql"

"INSERT INTO " + MediaTable::Name + "("
    "id_media, type, subtype, duration, play_count, last_played_date, insertion_date,"
    "release_date, thumbnail_id, thumbnail_generated, title, filename, is_favorite,"
    "is_present) "
"SELECT id_media, type, ifnull(subtype, " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                    IMedia::SubType::Unknown ) )
    + "), duration, play_count, last_played_date,"
    "insertion_date, release_date, "
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE id_media END,"
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE 1 END,"
    "title, filename, is_favorite, is_present FROM " + MediaTable::Name + "_backup",

"DROP TABLE " + MediaTable::Name + "_backup",

/************ Playlist external media were stored as Unknown ******************/

"UPDATE " + policy::MediaTable::Name + " SET type = " +
std::to_string( static_cast<typename std::underlying_type<IMedia::Type>::type>(
            IMedia::Type::External ) ) + " "
"WHERE id_media IN (SELECT media_id FROM PlaylistMediaRelation) AND "
"type = " + std::to_string( static_cast<typename std::underlying_type<IMedia::Type>::type>(
IMedia::Type::Unknown ) ),

/******************* Migrate metadata table ***********************************/
"CREATE TEMPORARY TABLE " + MetadataTable::Name + "_backup"
"("
    "id_media INTEGER,"
    "type INTEGER,"
    "value TEXT"
")",

"INSERT INTO " + MetadataTable::Name + "_backup SELECT * FROM MediaMetadata",

"DROP TABLE MediaMetadata",

// Recreate the new table
#include "database/tables/Metadata_v14.sql"

"INSERT INTO " + MetadataTable::Name + " "
"SELECT "
    "id_media, " + std::to_string(
        static_cast<typename std::underlying_type<IMetadata::EntityType>::type>(
            IMetadata::EntityType::Media ) ) +
    ", type, value "
"FROM " + MetadataTable::Name + "_backup",

"DROP TABLE " + MetadataTable::Name + "_backup",

/******************* Migrate the playlist table *******************************/
"CREATE TEMPORARY TABLE " + PlaylistTable::Name + "_backup"
"("
    + PlaylistTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
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

"INSERT INTO " + PlaylistTable::Name + "_backup SELECT * FROM " + PlaylistTable::Name,
"INSERT INTO PlaylistMediaRelation_backup SELECT * FROM PlaylistMediaRelation",

"DROP TABLE " + PlaylistTable::Name,
"DROP TABLE PlaylistMediaRelation",

#include "database/tables/Playlist_v14.sql"

"INSERT INTO " + PlaylistTable::Name + " SELECT * FROM " + PlaylistTable::Name + "_backup",
"INSERT INTO PlaylistMediaRelation SELECT media_id, NULL, playlist_id, position "
    "FROM PlaylistMediaRelation_backup",

"DROP TABLE " + PlaylistTable::Name + "_backup",
"DROP TABLE PlaylistMediaRelation_backup",

/******************* Migrate Device table *************************************/

"CREATE TEMPORARY TABLE " + policy::DeviceTable::Name + "_backup"
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT UNIQUE ON CONFLICT FAIL,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN"
")",

"INSERT INTO " + DeviceTable::Name + "_backup SELECT * FROM " + DeviceTable::Name,

"DROP TABLE " + DeviceTable::Name,

#include "database/tables/Device_v14.sql"

"INSERT INTO " + DeviceTable::Name + " SELECT id_device, uuid, scheme, is_removable, is_present,"
    "strftime('%s', 'now') FROM " + DeviceTable::Name,

"DROP TABLE " + DeviceTable::Name + "_backup",

/******************* Delete other tables **************************************/

"DROP TABLE " + AlbumTable::Name,
"DELETE FROM " + AlbumTable::Name + "Fts",
"DROP TABLE " + ArtistTable::Name,
"DELETE FROM " + ArtistTable::Name + "Fts",
"DELETE FROM MediaArtistRelation",
"DROP TABLE " + MovieTable::Name,
"DROP TABLE " + ShowTable::Name,
// No need to delete the ShowFts table since... it didn't exist
"DROP TABLE " + VideoTrackTable::Name,
// Flush the audio track table to recreate all tracks
"DELETE FROM " + AudioTrackTable::Name,

// History table & its triggers were removed for good:
"DROP TABLE History",
