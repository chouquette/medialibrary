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

"DROP TABLE " + Playlist::MediaRelationTable::Name + "_backup",

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

"CREATE TEMPORARY TABLE " + MediaGroup::Table::Name + "_backup"
"("
    "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "nb_video UNSIGNED INTEGER DEFAULT 0,"
    "nb_audio UNSIGNED INTEGER DEFAULT 0,"
    "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
    "duration INTEGER DEFAULT 0,"
    "creation_date INTEGER NOT NULL,"
    "last_modification_date INTEGER NOT NULL,"
    "user_interacted BOOLEAN,"
    "forced_singleton BOOLEAN"
")",

"INSERT INTO " + MediaGroup::Table::Name + "_backup "
    "SELECT id_group, name, nb_video, nb_audio, nb_unknown, duration, creation_date, "
    "last_modification_date, user_interacted, forced_singleton "
    "FROM " + MediaGroup::Table::Name,

"DROP TABLE " + MediaGroup::Table::Name,

MediaGroup::schema( MediaGroup::Table::Name, 30 ),

"INSERT INTO " + MediaGroup::Table::Name +
    " SELECT id_group, name, "
    " (SELECT COUNT() FROM " + Media::Table::Name + " WHERE group_id = id_group"
        " AND type = " + utils::enum_to_string( IMedia::Type::Video ) + "),"
    " (SELECT COUNT() FROM " + Media::Table::Name + " WHERE group_id = id_group"
        " AND type = " + utils::enum_to_string( IMedia::Type::Audio ) + "),"
    " (SELECT COUNT() FROM " + Media::Table::Name + " WHERE group_id = id_group"
        " AND type = " + utils::enum_to_string( IMedia::Type::Unknown ) + "),"
    " nb_video, nb_audio, nb_unknown, duration, creation_date,"
    " last_modification_date, user_interacted, forced_singleton "
    " FROM " + MediaGroup::Table::Name + "_backup",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 30 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 30 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 30 ),
MediaGroup::index( MediaGroup::Indexes::ForcedSingleton, 30 ),
MediaGroup::index( MediaGroup::Indexes::Duration, 30 ),
MediaGroup::index( MediaGroup::Indexes::CreationDate, 30 ),
MediaGroup::index( MediaGroup::Indexes::LastModificationDate, 30 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::UpdateTotalNbMedia, 29 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::UpdateNbMediaPerType, 29 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 30 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 29 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 30 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 29 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 30 ),

"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    "id_playlist PRIMARY KEY,"
    "name TEXT,"
    "file_id UNSIGNED INT,"
    "creation_date UNSIGNED INT,"
    "artwork_mrl TEXT"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup "
    " SELECT * FROM " + Playlist::Table::Name,

"DROP TABLE " + Playlist::Table::Name,
Playlist::schema( Playlist::Table::Name, 30 ),

"INSERT INTO " + Playlist::Table::Name + " SELECT *,"
    " (SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
        " WHERE playlist_id = id_playlist),"
    " (SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name + " mrt"
        " INNER JOIN " + Media::Table::Name + " m ON m.id_media = mrt.media_id "
        " WHERE m.is_present != 0 AND playlist_id = id_playlist)"
    " FROM " + Playlist::Table::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",

Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 30 ),
Playlist::trigger( Playlist::Triggers::InsertFts, 30 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 30 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 30 ),
Playlist::trigger( Playlist::Triggers::UpdateNbPresentMediaOnPresenceChange, 30 ),
parser::Task::trigger( parser::Task::Triggers::DeletePlaylistLinkingTask, 30 ),

Playlist::index( Playlist::Indexes::FileId, 30 ),

"DROP TABLE ExcludedEntryFolder",
