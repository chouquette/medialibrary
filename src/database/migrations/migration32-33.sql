"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
   "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
   "type INTEGER,"
   "subtype INTEGER,"
   "duration INTEGER,"
   "last_position REAL DEFAULT -1,"
   "last_time INTEGER DEFAULT -1,"
   "play_count UNSIGNED INTEGER,"
   "last_played_date UNSIGNED INTEGER,"
   "insertion_date UNSIGNED INTEGER,"
   "release_date UNSIGNED INTEGER,"
   "title TEXT COLLATE NOCASE,"
   "filename TEXT COLLATE NOCASE,"
   "is_favorite BOOLEAN,"
   "is_present BOOLEAN,"
   "device_id INTEGER,"
   "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
   "folder_id UNSIGNED INTEGER,"
   "import_type UNSIGNED INTEGER NOT NULL,"
   "group_id UNSIGNED INTEGER,"
   "forced_title BOOLEAN NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Media::Table::Name + "_backup "
"SELECT id_media, type, subtype, duration, last_position, last_time,"
    "play_count, last_played_date, insertion_date, release_date,"
    "title, filename, is_favorite, is_present, device_id, nb_playlists, folder_id,"
    "import_type, group_id, forced_title FROM " + Media::Table::Name,


"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 33 ),

"INSERT INTO " + Media::Table::Name +
    " SELECT * FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    "id_playlist PRIMARY KEY,"
    "name TEXT,"
    "file_id UNSIGNED INT,"
    "creation_date UNSIGNED INT,"
    "artwork_mrl TEXT,"
    "nb_media UNSIGNED INT,"
    "nb_present_media UNSIGNED INT"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup "
    " SELECT * FROM " + Playlist::Table::Name,

"DROP TABLE " + Playlist::Table::Name,
Playlist::schema( Playlist::Table::Name, 33 ),

"INSERT INTO " + Playlist::Table::Name + " SELECT *,"
    " (SELECT TOTAL(IIF(m.duration > 0, m.duration, 0)) "
        " FROM " + Playlist::MediaRelationTable::Name + " mrt"
        " INNER JOIN " + Media::Table::Name + " m ON m.id_media = mrt.media_id "
        " WHERE mrt.playlist_id = id_playlist)"
    " FROM " + Playlist::Table::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",

Media::trigger( Media::Triggers::InsertFts, 33 ),
Media::trigger( Media::Triggers::UpdateFts, 33 ),
Media::trigger( Media::Triggers::DeleteFts, 33 ),

Media::index( Media::Indexes::LastPlayedDate, 33 ),
Media::index( Media::Indexes::Presence, 33 ),
Media::index( Media::Indexes::Types, 33 ),
Media::index( Media::Indexes::LastUsageDate, 33 ),
Media::index( Media::Indexes::Folder, 33 ),
Media::index( Media::Indexes::MediaGroup, 33 ),
Media::index( Media::Indexes::Progress, 33 ),

Album::trigger( Album::Triggers::IsPresent, 33 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 33 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 33 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 33 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 33 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 33 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 33 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 33 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 33 ),
Playlist::trigger( Playlist::Triggers::UpdateNbPresentMediaOnPresenceChange, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 33 ),

Playlist::trigger( Playlist::Triggers::UpdateDurationOnMediaChange, 33 ),
Playlist::trigger( Playlist::Triggers::InsertFts, 33 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 33 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 33 ),

Playlist::index( Playlist::Indexes::FileId, 33 ),
parser::Task::trigger( parser::Task::Triggers::DeletePlaylistLinkingTask, 33 ),
