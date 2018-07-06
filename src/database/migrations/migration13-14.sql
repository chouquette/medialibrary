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

"CREATE TABLE " + MediaTable::Name + "("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                            IMedia::SubType::Unknown ) ) + ","
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail_id INTEGER,"
    "thumbnail_generated BOOLEAN NOT NULL DEFAULT 0,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "FOREIGN KEY(thumbnail_id) REFERENCES " + ThumbnailTable::Name
    + "(id_thumbnail)"
")",

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
