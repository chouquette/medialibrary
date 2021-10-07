"CREATE TEMPORARY TABLE " + MediaGroup::Table::Name + "_backup"
"("
   "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
   "parent_id INTEGER,"
   "name TEXT COLLATE NOCASE,"
   "nb_video UNSIGNED INTEGER DEFAULT 0,"
   "nb_audio UNSIGNED INTEGER DEFAULT 0,"
   "nb_unknown UNSIGNED INTEGER DEFAULT 0"
")",

"INSERT INTO " + MediaGroup::Table::Name + "_backup "
    "SELECT * FROM " + MediaGroup::Table::Name,

"DROP TABLE " + MediaGroup::Table::Name,
MediaGroup::schema( MediaGroup::Table::Name, 25 ),

"INSERT INTO " + MediaGroup::Table::Name +
    " SELECT id_group, name, nb_video, nb_audio, nb_unknown, "
    " (SELECT SUM(max(duration, 0)) FROM " + Media::Table::Name +
        " WHERE group_id = id_group), 0, 0, false, false"
    " FROM " + MediaGroup::Table::Name + "_backup ",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 25 ),
MediaGroup::index( MediaGroup::Indexes::ForcedSingleton, 25 ),
MediaGroup::index( MediaGroup::Indexes::Duration, 25 ),
MediaGroup::index( MediaGroup::Indexes::CreationDate, 25 ),
MediaGroup::index( MediaGroup::Indexes::LastModificationDate, 25 ),

"CREATE TEMPORARY TABLE " + parser::Task::Table::Name + "_backup"
"("
    "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
    "step INTEGER NOT NULL DEFAULT 0,"
    "retry_count INTEGER NOT NULL DEFAULT 0,"
    "type INTEGER NOT NULL,"
    "mrl TEXT,"
    "file_type INTEGER NOT NULL,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER,"
    "link_to_id UNSIGNED INTEGER NOT NULL,"
    "link_to_type UNSIGNED INTEGER NOT NULL,"
    "link_extra UNSIGNED INTEGER NOT NULL"
")",

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT * FROM " + parser::Task::Table::Name,

"DROP TABLE " + parser::Task::Table::Name,

parser::Task::schema( parser::Task::Table::Name, 25 ),

"INSERT INTO " + parser::Task::Table::Name +
    " SELECT *, '' FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " + parser::Task::Table::Name + "_backup",

parser::Task::index( parser::Task::Indexes::ParentFolderId, 25 ),

/* Remove Media.has_been_grouped column */
"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER,"
    "duration INTEGER,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "real_last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT COLLATE NOCASE,"
    "is_favorite BOOLEAN,"
    "is_present BOOLEAN,"
    "device_id INTEGER,"
    "nb_playlists UNSIGNED INTEGER,"
    "folder_id UNSIGNED INTEGER,"
    "import_type UNSIGNED INTEGER,"
    "group_id UNSIGNED INTEGER,"
    "forced_title BOOLEAN"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT "
    "id_media, type, subtype, duration, play_count, last_played_date, real_last_played_date,"
    "insertion_date, release_date, title, filename, is_favorite, is_present, device_id,"
    "nb_playlists, folder_id, import_type, group_id, forced_title "
    " FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 25 ),

"INSERT INTO " + Media::Table::Name +
    " SELECT * FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

"CREATE TEMPORARY TABLE " + Bookmark::Table::Name + "_backup"
"("
    "id_bookmark INTEGER PRIMARY KEY,"
    "time UNSIGNED INTEGER,"
    "name TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INTEGER"
")",

"INSERT INTO " + Bookmark::Table::Name + "_backup "
    "SELECT * FROM " + Bookmark::Table::Name,

"DROP TABLE " + Bookmark::Table::Name,

Bookmark::schema( Bookmark::Table::Name, 25 ),

"INSERT INTO " + Bookmark::Table::Name +
    " SELECT *, 0, " + utils::enum_to_string( IBookmark::Type::Simple ) +
    " FROM " + Bookmark::Table::Name + "_backup",

"CREATE TEMPORARY TABLE " + Device::Table::Name + "_backup"
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT COLLATE NOCASE,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN,"
    "last_seen UNSIGNED INTEGER"
")",

"INSERT INTO " + Device::Table::Name + "_backup SELECT * FROM " + Device::Table::Name,

"DROP TABLE " + Device::Table::Name,
Device::schema( Device::Table::Name, 25 ),

"INSERT INTO " + Device::Table::Name +
    " SELECT id_device, uuid, scheme, is_removable, is_present,"
    "(is_removable != 0 AND scheme != 'file://'), last_seen FROM "
    + Device::Table::Name + "_backup",

"DROP TABLE " + Device::Table::Name + "_backup",

/* Now recreate triggers based on the Device table */
Media::trigger( Media::Triggers::IsPresent, 25 ),


Media::trigger( Media::Triggers::InsertFts, 25 ),
Media::trigger( Media::Triggers::UpdateFts, 25 ),
Media::trigger( Media::Triggers::DeleteFts, 25 ),

Media::index( Media::Indexes::LastPlayedDate, 25 ),
Media::index( Media::Indexes::Presence, 25 ),
Media::index( Media::Indexes::Types, 25 ),
Media::index( Media::Indexes::LastUsageDate, 25 ),
Media::index( Media::Indexes::Folder, 25 ),
Media::index( Media::Indexes::MediaGroup, 25 ),

Album::trigger( Album::Triggers::IsPresent, 25 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 25 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 25 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 25 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 25 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 25 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 25 ),

MediaGroup::trigger( MediaGroup::Triggers::IncrementNbMediaOnGroupChange, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnGroupChange, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 25 ),

MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 25 ),
