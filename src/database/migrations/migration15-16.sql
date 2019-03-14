/* Migrate to contiguous playlist position index */

"CREATE TEMPORARY TABLE PlaylistMediaRelation_backup"
"("
    "media_id INTEGER,"
    "mrl STRING,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO PlaylistMediaRelation_backup"
    "(media_id, mrl, playlist_id, position) "
    "SELECT media_id, mrl, playlist_id, ROW_NUMBER() OVER ("
        "PARTITION BY playlist_id "
        "ORDER BY position"
    ") - 1 "
"FROM PlaylistMediaRelation",

"DROP TABLE PlaylistMediaRelation",

#include "database/tables/Playlist_v16.sql"

"INSERT INTO PlaylistMediaRelation SELECT * FROM PlaylistMediaRelation_backup",
"DROP TABLE PlaylistMediaRelation_backup",

#include "database/tables/Playlist_triggers_v16.sql"

"DROP INDEX IF EXISTS folder_device_id",
"DROP INDEX IF EXISTS folder_parent_id",
