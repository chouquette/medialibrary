/* Migrate to contiguous playlist position index */

"CREATE TEMPORARY TABLE " + Playlist::MediaRelationTable::Name + "_backup"
"("
    "media_id INTEGER,"
    "mrl STRING,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO " + Playlist::MediaRelationTable::Name + "_backup"
    "(media_id, mrl, playlist_id, position) "
    "SELECT media_id, mrl, playlist_id, ROW_NUMBER() OVER ("
        "PARTITION BY playlist_id "
        "ORDER BY position"
    ") - 1 "
"FROM " + Playlist::MediaRelationTable::Name,

"DROP TABLE " + Playlist::MediaRelationTable::Name + "",

Playlist::schema( Playlist::MediaRelationTable::Name, 16 ),

"INSERT INTO " + Playlist::MediaRelationTable::Name +
" SELECT * FROM " + Playlist::MediaRelationTable::Name + "_backup",
"DROP TABLE " + Playlist::MediaRelationTable::Name + "_backup",

#include "database/tables/Playlist_triggers_v16.sql"
Playlist::trigger( Playlist::Triggers::UpdateOrderOnInsert, 16 ),
Playlist::trigger( Playlist::Triggers::UpdateOrderOnDelete, 16 ),
Playlist::trigger( Playlist::Triggers::InsertFts, 16 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 16 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 16 ),

"DROP INDEX IF EXISTS folder_device_id",
"DROP INDEX IF EXISTS folder_parent_id",
