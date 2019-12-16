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
/*** has_tracks_present was implicitely deleted when deleting the Media table ***/
"DROP TRIGGER cascade_file_deletion",

/*** Migrate thumbnail table ***/
"DROP TABLE " + Thumbnail::Table::Name,
Thumbnail::schema( Thumbnail::Table::Name, 23 ),
/* Ensure we also flush the linking table since the foreign key
 * ON DELETE CASCADE won't be processed during a DROP TABLE */
"DELETE FROM " + Thumbnail::LinkingTable::Name,
Thumbnail::index( Thumbnail::Indexes::ThumbnailId, 23 ),


"UPDATE " + parser::Task::Table::Name + " AS t SET file_type = " +
    std::to_string( static_cast<std::underlying_type_t<IFile::Type>>(
        IFile::Type::Playlist ) ) +
    " WHERE t.file_id IS NOT NULL"
    " AND (SELECT playlist_id FROM " + File::Table::Name + " p"
        " WHERE p.id_file = t.file_id) IS NOT NULL",

"DELETE FROM " + parser::Task::Table::Name +
    " WHERE file_id IS NULL AND type = " +
    std::to_string( static_cast<std::underlying_type_t<parser::Task::Type>>(
        parser::Task::Type::Creation ) ),

Media::trigger( Media::Triggers::InsertFts, 23 ),
Media::trigger( Media::Triggers::UpdateFts, 23 ),
Media::trigger( Media::Triggers::DeleteFts, 23 ),
Media::trigger( Media::Triggers::CascadeFileDeletion, 23 ),
Media::trigger( Media::Triggers::IsPresent, 23 ),

Media::index( Media::Indexes::LastPlayedDate, 23 ),
Media::index( Media::Indexes::Presence, 23 ),
Media::index( Media::Indexes::Types, 23 ),
Media::index( Media::Indexes::LastUsageDate, 23 ),
Media::index( Media::Indexes::Folder, 23 ),

Show::trigger( Show::Triggers::InsertFts, 23 ),
Show::trigger( Show::Triggers::DeleteFts, 23 ),
Show::trigger( Show::Triggers::IncrementNbEpisode, 23 ),
Show::trigger( Show::Triggers::DecrementNbEpisode, 23 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 23 ),

/* Recreate triggers that reference the media table */
Album::trigger( Album::Triggers::IsPresent, 23 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 23 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 23 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 23 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 23 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 23 ),

Artist::trigger( Artist::Triggers::UpdateNbAlbums, 23 ),
Artist::trigger( Artist::Triggers::DecrementNbAlbums, 23 ),
Artist::trigger( Artist::Triggers::IncrementNbAlbums, 23 ),
Artist::trigger( Artist::Triggers::DecrementNbTracks, 23 ),
Artist::trigger( Artist::Triggers::IncrementNbTracks, 23 ),
Artist::trigger( Artist::Triggers::DeleteArtistsWithoutTracks, 23 ),

Thumbnail::trigger( Thumbnail::Triggers::DeleteUnused, 23 ),
