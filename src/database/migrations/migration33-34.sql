"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        utils::enum_to_string( Media::SubType::Unknown ) + ","
    "duration INTEGER DEFAULT -1,"
    "last_position REAL DEFAULT -1,"
    "last_time INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER NON NULL DEFAULT 0,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT COLLATE NOCASE,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "device_id INTEGER,"
    "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "folder_id UNSIGNED INTEGER,"
    "import_type UNSIGNED INTEGER NOT NULL,"
    "group_id UNSIGNED INTEGER,"
    "forced_title BOOLEAN NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Media::Table::Name + "_backup"
    " SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,
Media::schema( Media::Table::Name, 34 ),

"INSERT INTO " + Media::Table::Name +
    " SELECT *, NULL, NULL, 0, NULL, 0 FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

"DROP TABLE " + AlbumTrack::Table::Name,

Media::trigger( Media::Triggers::InsertFts, 34 ),
Media::trigger( Media::Triggers::UpdateFts, 34 ),
Media::trigger( Media::Triggers::DeleteFts, 34 ),

Media::index( Media::Indexes::LastPlayedDate, 34 ),
Media::index( Media::Indexes::Presence, 34 ),
Media::index( Media::Indexes::Types, 34 ),
Media::index( Media::Indexes::LastUsageDate, 34 ),
Media::index( Media::Indexes::Folder, 34 ),
Media::index( Media::Indexes::MediaGroup, 34 ),
Media::index( Media::Indexes::Progress, 34 ),
Media::index( Media::Indexes::AlbumTrack, 34 ),
Album::trigger( Album::Triggers::IsPresent, 34 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 34 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 34 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 34 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateDurationOnMediaChange, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 34 ),

// This trigger was automatically deleted with the AlbumTrack table
Album::trigger( Album::Triggers::DeleteTrack, 34 ),

"DROP TRIGGER " + Album::triggerName( Album::Triggers::IsPresent, 33 ),
Album::trigger( Album::Triggers::IsPresent, 34 ),

"DROP TRIGGER " + Artist::triggerName( Artist::Triggers::HasTrackPresent, 33 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 34 ),

// This trigger was automatically deleted with the AlbumTrack table
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 34 ),

// New triggers
Album::trigger( Album::Triggers::DeleteEmpty, 34 ),
Genre::trigger( Genre::Triggers::DeleteEmpty, 34 ),

// New indexes
Album::index( Album::Indexes::NbTracks, 34 ),
