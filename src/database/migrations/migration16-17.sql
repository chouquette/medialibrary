/* Reinsert media to update the thumbnail_id foreign key */
"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                            IMedia::SubType::Unknown ) ) + ","
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "real_last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail_id INTEGER,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT COLLATE NOCASE,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "device_id INTEGER,"
    "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "folder_id UNSIGNED INTEGER"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

#include "database/tables/Media_v17.sql"

"INSERT INTO " + Media::Table::Name + " SELECT * FROM " + Media::Table::Name + "_backup",
"DROP TABLE " + Media::Table::Name + "_backup",

// This trigger was incorrectly recreated during the 13 -> 14 migration, so
// ensure it's the correct one now.
// Artists triggers are all recreated as part of the 16 -> 17 migration
"DROP TRIGGER has_track_remaining",
