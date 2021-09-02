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
    "IFNULL(play_count, 0), last_played_date, insertion_date, release_date,"
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
    "artwork_mrl TEXT"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup "
    " SELECT id_playlist, name, file_id, creation_date, artwork_mrl FROM "
        + Playlist::Table::Name,

"DROP TABLE " + Playlist::Table::Name,
Playlist::schema( Playlist::Table::Name, 33 ),

"INSERT INTO " + Playlist::Table::Name +
    " SELECT *, 0, 0, 0, 0, 0, 0, 0"
    " FROM " + Playlist::Table::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",

/*
 * Compute the duration and nb_non_audio in a second phase. This computation
 * requires joining with MediaRelationTable but it would exclude empty playlist
 * which don't have any associated record in that table.
 * While it would most likely be possible to to everything in a single phase, it
 * would probably lead to a needlessly complex request, especially since it will
 * only be run once during the migration
 */
"UPDATE " + Playlist::Table::Name + " SET"
    " nb_video = sub.nb_video, nb_present_video = sub.nb_present_video,"
    " nb_audio = sub.nb_audio, nb_present_audio = sub.nb_present_audio,"
    " nb_unknown = sub.nb_unknown, nb_present_unknown = sub.nb_present_unknown,"
    " duration = sub.duration"
" FROM (SELECT"
    " mrt.playlist_id AS playlist_id,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Video ) ) + ", 1, 0)) AS nb_video,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Video ) ) + " AND m.is_present != 0, 1, 0)) AS nb_present_video,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Audio ) ) + ", 1, 0)) AS nb_audio,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Audio ) ) + " AND m.is_present != 0, 1, 0)) AS nb_present_audio,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Unknown) ) + ", 1, 0)) AS nb_unknown,"
    " TOTAL(IIF(m.type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Unknown) ) + " AND m.is_present != 0, 1, 0)) AS nb_present_unknown,"
    " TOTAL(IIF(m.duration > 0, m.duration, 0)) AS duration"
    " FROM " + Playlist::MediaRelationTable::Name + " mrt"
    " LEFT OUTER JOIN " + Media::Table::Name + " m ON m.id_media = mrt.media_id"
    " GROUP BY mrt.playlist_id) AS sub"
" WHERE id_playlist = sub.playlist_id",

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
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 33 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 33 ),

Playlist::trigger( Playlist::Triggers::UpdateDurationOnMediaChange, 33 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaChange, 33 ),
Playlist::trigger( Playlist::Triggers::InsertFts, 33 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 33 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 33 ),

Playlist::index( Playlist::Indexes::FileId, 33 ),
parser::Task::trigger( parser::Task::Triggers::DeletePlaylistLinkingTask, 33 ),
