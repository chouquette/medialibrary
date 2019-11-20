"DROP TRIGGER has_track_remaining",
"DROP TRIGGER has_album_remaining",

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
    "folder_id UNSIGNED INTEGER"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 23 ),

"INSERT INTO " + Media::Table::Name + " SELECT "
"id_media, "
// Convert old Media::Type::External & Media::Type::Stream to Unknown
"CASE WHEN type = 3 OR type = 4 THEN " +
    std::to_string( static_cast<std::underlying_type_t<Media::Type>>( Media::Type::Unknown ) ) +
    " ELSE type END, "
"subtype, duration, play_count, last_played_date, real_last_played_date, insertion_date, "
"release_date, title, filename, is_favorite, is_present, device_id, nb_playlists, "
"folder_id, "
"CASE WHEN type = 3 THEN " +
    std::to_string( static_cast<std::underlying_type_t<Media::ImportType>>( Media::ImportType::External ) ) +
    " WHEN type = 4 THEN " +
    std::to_string( static_cast<std::underlying_type_t<Media::ImportType>>( Media::ImportType::Stream ) ) +
    " ELSE " +
    std::to_string( static_cast<std::underlying_type_t<Media::ImportType>>( Media::ImportType::Internal ) ) +
    " END"
" FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

/******* Migrate show table *******/

"DROP TABLE " + Show::Table::Name,
Show::schema( Show::Table::Name, 23 ),

/*** Migrate media presence trigger ***/

"DROP TRIGGER is_media_device_present",

/*** is_album_present was implicitely deleted when deleting the Media table ***/
