"CREATE TEMPORARY TABLE " + Label::FileRelationTable::Name + "_backup"
"("
    "label_id INTEGER,"
    "media_id INTEGER,"
    "PRIMARY KEY(label_id,media_id)"
")",

"INSERT INTO " + Label::FileRelationTable::Name + "_backup "
    "SELECT * FROM " + Label::FileRelationTable::Name,

"DROP TABLE " + Label::FileRelationTable::Name,
Label::schema( Label::FileRelationTable::Name, 37 ),

"INSERT INTO " + Label::FileRelationTable::Name +
    " SELECT label_id, media_id, " + utils::enum_to_string( Label::EntityType::Media ) +
    " FROM " + Label::FileRelationTable::Name + "_backup",

"DROP TABLE " + Label::FileRelationTable::Name + "_backup",

"CREATE TEMPORARY TABLE " + Folder::Table::Name + "_backup"
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "name TEXT COLLATE NOCASE,"
    "parent_id UNSIGNED INTEGER,"
    "is_banned BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Folder::Table::Name + "_backup "
    "SELECT * FROM " + Folder::Table::Name,

"DROP TABLE " + Folder::Table::Name,

Folder::schema( Folder::Table::Name, 37 ),

"INSERT INTO " + Folder::Table::Name +
    " SELECT *, (SELECT TOTAL(IIF(duration > 0, duration, 0)) FROM "
        + Media::Table::Name + " WHERE folder_id = id_folder), FALSE"
    " FROM " + Folder::Table::Name + "_backup",


"DROP TABLE " + Folder::Table::Name + "_backup",

/* Triggers and indexes deleted by the table deletion */
Folder::trigger( Folder::Triggers::InsertFts, 37 ),
Folder::trigger( Folder::Triggers::DeleteFts, 37 ),
Folder::trigger( Folder::Triggers::UpdateIsPublic, 37 ),

Folder::index( Folder::Indexes::DeviceId, 37 ),
Folder::index( Folder::Indexes::ParentId, 37 ),

Subscription::schema( Subscription::Table::Name, 37 ),
Subscription::schema( Subscription::MediaRelationTable::Name, 37 ),
Subscription::index( Subscription::Indexes::ServiceId, 37 ),
Subscription::index( Subscription::Indexes::RelationMediaId, 37 ),
Subscription::index( Subscription::Indexes::RelationSubscriptionId, 37 ),

parser::Task::trigger( parser::Task::Triggers::DeleteSubscriptionLinkingTask, 37 ),

"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        utils::enum_to_string( Media::SubType::Unknown ) + ","
    "duration INTEGER DEFAULT -1,"
    "last_position REAL DEFAULT -1,"
    "last_time INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER NOT NULL DEFAULT 0,"
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
    "forced_title BOOLEAN NOT NULL DEFAULT 0,"
    "artist_id UNSIGNED INTEGER,"
    "genre_id UNSIGNED INTEGER,"
    "track_number UNSIGEND INTEGER,"
    "album_id UNSIGNED INTEGER,"
    "disc_number UNSIGNED INTEGER,"
    "lyrics TEXT"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,
Media::schema( Media::Table::Name, 37 ),

"INSERT INTO " + Media::Table::Name +  " SELECT id_media, type, subtype, "
"duration, last_position, last_time, play_count, last_played_date, insertion_date, "
"IIF(release_date < 3000, STRFTIME( '%s', CAST(release_date as TEXT) || '-01-01' ), release_date), "
"title, filename, is_favorite, is_present, device_id, nb_playlists, folder_id, "
"import_type, group_id, forced_title, artist_id, genre_id, track_number, album_id, "
"disc_number, lyrics, FALSE, 0, '' FROM "
    + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

Media::trigger( Media::Triggers::InsertFts, 37 ),
Media::trigger( Media::Triggers::UpdateFts, 37 ),
Media::trigger( Media::Triggers::DeleteFts, 37 ),
Media::trigger( Media::Triggers::UpdateIsPublic, 37 ),
Media::trigger( Media::Triggers::IncrementNbSubscription, 37 ),
Media::trigger( Media::Triggers::DecrementNbSubscription, 37 ),

Media::index( Media::Indexes::LastPlayedDate, 37 ),
Media::index( Media::Indexes::Presence, 37 ),
Media::index( Media::Indexes::Types, 37 ),
Media::index( Media::Indexes::InsertionDate, 37 ),
Media::index( Media::Indexes::Folder, 37 ),
Media::index( Media::Indexes::MediaGroup, 37 ),
Media::index( Media::Indexes::Progress, 37 ),
Media::index( Media::Indexes::AlbumTrack, 37 ),
Media::index( Media::Indexes::Duration, 37 ),
Media::index( Media::Indexes::ReleaseDate, 37 ),
Media::index( Media::Indexes::PlayCount, 37 ),
Media::index( Media::Indexes::Title, 37 ),
Media::index( Media::Indexes::FileName, 37 ),
Media::index( Media::Indexes::GenreId, 37 ),
Media::index( Media::Indexes::ArtistId, 37 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 37 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 37 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 37 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 37 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 37 ),
Playlist::trigger( Playlist::Triggers::UpdateDurationOnMediaChange, 37 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaChange, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 37 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 37 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 37 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 37 ),
Genre::trigger( Genre::Triggers::UpdateOnMediaGenreIdChange, 37 ),
Album::trigger( Album::Triggers::IsPresent, 37 ),
Album::trigger( Album::Triggers::DeleteTrack, 37 ),
Album::trigger( Album::Triggers::UpdateOnMediaAlbumIdChange, 37 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 37 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 37 ),
Label::trigger( Label::Triggers::DeleteMediaLabel, 37 ),
Album::index( Album::Indexes::Title, 37 ),


"ALTER TABLE " + Album::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",
"ALTER TABLE " + Artist::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",
"ALTER TABLE " + Genre::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",

"CREATE TEMPORARY TABLE " + File::Table::Name + "_backup"
"("
    "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
    "media_id UNSIGNED INT DEFAULT NULL,"
    "playlist_id UNSIGNED INT DEFAULT NULL,"
    "mrl TEXT,"
    "type UNSIGNED INTEGER,"
    "last_modification_date UNSIGNED INT,"
    "size UNSIGNED INT,"
    "folder_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "is_external BOOLEAN NOT NULL,"
    "is_network BOOLEAN NOT NULL"
")",

"INSERT INTO " + File::Table::Name + "_backup "
    "SELECT * FROM " + File::Table::Name,

"DROP TABLE " + File::Table::Name,
File::schema( File::Table::Name, 37 ),

"INSERT INTO " + File::Table::Name +
    " SELECT *, NULL FROM " + File::Table::Name + "_backup",

"DROP TABLE " + File::Table::Name + "_backup",

File::index( File::Indexes::MediaId, 37 ),
File::index( File::Indexes::FolderId, 37 ),
File::index( File::Indexes::PlaylistId, 37 ),

Media::trigger( Media::Triggers::CascadeFileDeletion, 37 ),
Media::trigger( Media::Triggers::CascadeFileUpdate, 37 ),

Playlist::trigger( Playlist::Triggers::CascadeFileDeletion, 37 ),
