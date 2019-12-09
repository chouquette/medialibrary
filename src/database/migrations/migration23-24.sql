MediaGroup::schema( MediaGroup::Table::Name, 24 ),
MediaGroup::schema( MediaGroup::FtsTable::Name, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 24 ),

/** Add group_id column for media */
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
    "import_type UNSIGNED INTEGER"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 24 ),

"INSERT INTO " + Media::Table::Name + " SELECT "
"id_media, type, subtype, duration, play_count, last_played_date, real_last_played_date,"
"insertion_date, release_date, title, filename, is_favorite, is_present, device_id,"
"nb_playlists, folder_id, import_type, NULL "
" FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",
