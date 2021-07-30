Thumbnail::schema( Thumbnail::CleanupTable::Name, 32 ),
Thumbnail::trigger( Thumbnail::Triggers::InsertCleanup, 32 ),

/*
 * During migraton 29 to 30 we forgot to delete the backup table. If the user
 * ends up migration from 29 to 32 directly, the table will still exist and the
 * migration will fail
 */
"DROP TABLE IF EXISTS " + Playlist::MediaRelationTable::Name + "_backup",

"CREATE TEMPORARY TABLE " + Playlist::MediaRelationTable::Name + "_backup"
"("
    "media_id INTEGER,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO " + Playlist::MediaRelationTable::Name + "_backup"
    " SELECT media_id, playlist_id, position"
        " FROM " + Playlist::MediaRelationTable::Name,

"DROP TABLE " + Playlist::MediaRelationTable::Name,
Playlist::schema( Playlist::MediaRelationTable::Name, 32 ),

"INSERT INTO " + Playlist::MediaRelationTable::Name +
    " SELECT * FROM " + Playlist::MediaRelationTable::Name + "_backup",

"DROP TABLE " + Playlist::MediaRelationTable::Name + "_backup",

Playlist::trigger( Playlist::Triggers::UpdateOrderOnInsert, 32 ),
Playlist::trigger( Playlist::Triggers::UpdateOrderOnDelete, 32 ),

Playlist::index( Playlist::Indexes::PlaylistIdPosition, 32 ),

Media::trigger( Media::Triggers::IncrementNbPlaylist, 32 ),
Media::trigger( Media::Triggers::DecrementNbPlaylist, 32 ),
