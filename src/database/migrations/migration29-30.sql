"CREATE TEMPORARY TABLE " + Playlist::MediaRelationTable::Name + "_backup"
"("
    "media_id INTEGER,"
    "mrl STRING,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO " + Playlist::MediaRelationTable::Name + "_backup"
    " SELECT * FROM " + Playlist::MediaRelationTable::Name,

"DROP TABLE " + Playlist::MediaRelationTable::Name,
Playlist::schema( Playlist::MediaRelationTable::Name, 30 ),

"INSERT INTO " + Playlist::MediaRelationTable::Name +
    " SELECT * FROM " + Playlist::MediaRelationTable::Name + "_backup",

Media::trigger( Media::Triggers::IncrementNbPlaylist, 30 ),
Media::trigger( Media::Triggers::DecrementNbPlaylist, 30 ),
Playlist::trigger( Playlist::Triggers::UpdateOrderOnInsert, 30 ),
Playlist::trigger( Playlist::Triggers::UpdateOrderOnDelete, 30 ),
Playlist::index( Playlist::Indexes::PlaylistIdPosition, 30 ),

"DROP TABLE " + Genre::Table::Name,
"DROP TABLE " + Genre::FtsTable::Name,

"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::UpdateOnNewTrack, 29 ),
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::UpdateOnTrackDelete, 29 ),

Genre::schema( Genre::Table::Name, 30 ),
Genre::schema( Genre::FtsTable::Name, 30 ),

Genre::trigger( Genre::Triggers::InsertFts, 30 ),
Genre::trigger( Genre::Triggers::DeleteFts, 30 ),
Genre::trigger( Genre::Triggers::UpdateOnNewTrack, 30 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 30 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 30 ),
