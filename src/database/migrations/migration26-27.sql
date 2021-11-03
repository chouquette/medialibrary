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
   "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
   "folder_id UNSIGNED INTEGER,"
   "import_type UNSIGNED INTEGER NOT NULL,"
   "group_id UNSIGNED INTEGER,"
   "forced_title BOOLEAN NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Media::Table::Name + "_backup "
    "SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 27 ),

"INSERT INTO " + Media::Table::Name + " SELECT "
    "id_media, type, subtype, duration, -1, play_count, last_played_date,"
    "real_last_played_date, insertion_date, release_date, title, filename,"
    "is_favorite, is_present, device_id, nb_playlists, folder_id, import_type,"
    "group_id, forced_title FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

Media::trigger( Media::Triggers::InsertFts, 27 ),
Media::trigger( Media::Triggers::UpdateFts, 27 ),
Media::trigger( Media::Triggers::DeleteFts, 27 ),
Media::trigger( Media::Triggers::CascadeFileUpdate, 27 ),

Media::index( Media::Indexes::LastPlayedDate, 27 ),
Media::index( Media::Indexes::Presence, 27 ),
Media::index( Media::Indexes::Types, 27 ),
Media::index( Media::Indexes::InsertionDate, 27 ),
Media::index( Media::Indexes::Folder, 27 ),
Media::index( Media::Indexes::MediaGroup, 27 ),
Media::index( Media::Indexes::Progress, 27 ),

Album::trigger( Album::Triggers::IsPresent, 27 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 27 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 27 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 27 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 27 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 27 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 27 ),

MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateTotalNbMedia, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 27 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 27 ),

#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)

"UPDATE " + Media::Table::Name + " AS m SET progress = (" +

#ifdef __ANDROID__
"SELECT CAST(value AS REAL) / m.duration"
#else
"SELECT CAST(value AS REAL)"
#endif

" FROM " + Metadata::Table::Name + " md "
    " WHERE entity_type = " +
        utils::enum_to_string( IMetadata::EntityType::Media ) +
    " AND type = 50 AND md.id_media = m.id_media "
")"
#ifdef __ANDROID__
" WHERE m.duration > 0"
#endif
/* terminate the request, otherwise it will be concatenated with the next one
 * see #270
 */
,

#endif

"CREATE TEMPORARY TABLE " + SubtitleTrack::Table::Name + "_backup"
"("
    "id_track INTEGER PRIMARY KEY,"
    "codec TEXT,"
    "language TEXT,"
    "description TEXT,"
    "encoding TEXT,"
    "media_id UNSIGNED INT"
")",

"INSERT INTO " + SubtitleTrack::Table::Name + "_backup "
    "SELECT * FROM " + SubtitleTrack::Table::Name,

"DROP TABLE " + SubtitleTrack::Table::Name,

SubtitleTrack::schema( SubtitleTrack::Table::Name, 27 ),

"INSERT INTO " + SubtitleTrack::Table::Name +
    " SELECT id_track, codec, language, description, encoding, media_id, NULL"
    " FROM " + SubtitleTrack::Table::Name,

"DROP TABLE " + SubtitleTrack::Table::Name + "_backup",

SubtitleTrack::index( SubtitleTrack::Indexes::MediaId, 27 ),

/* Migrate AudioTrack table */

"CREATE TEMPORARY TABLE " + AudioTrack::Table::Name + "_backup"
"("
    "id_track INTEGER PRIMARY KEY,"
    "codec TEXT,"
    "bitrate UNSIGNED INTEGER,"
    "samplerate UNSIGNED INTEGER,"
    "nb_channels UNSIGNED INTEGER,"
    "language TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INT"
")",

"INSERT INTO " + AudioTrack::Table::Name + "_backup "
    "SELECT * FROM " + AudioTrack::Table::Name,

"DROP TABLE " + AudioTrack::Table::Name,

AudioTrack::schema( AudioTrack::Table::Name, 27 ),

"INSERT INTO " + AudioTrack::Table::Name +
    " SELECT id_track, codec, bitrate, samplerate, nb_channels, language, "
        "description, media_id, NULL"
    " FROM " + AudioTrack::Table::Name,

"DROP TABLE " + AudioTrack::Table::Name + "_backup",

AudioTrack::index( AudioTrack::Indexes::MediaId, 27 ),

/*
 * Recreate the entire settings table. We don't properly handle schema changes
 * for that table, and only altering the table would cause prior changes to be
 * applied too soon.
 * For instance, when migrating from model 23 to 24, we already recreate the table
 * with the new settings that should be added in model 27, causing the alter
 * table to fail because the colums already exist
 */
"DROP TABLE Settings",

"CREATE TEMPORARY TABLE " + parser::Task::Table::Name + "_backup"
"("
    "id_task INTEGER PRIMARY KEY,"
    "step INTEGER,"
    "retry_count INTEGER,"
    "type INTEGER,"
    "mrl TEXT,"
    "file_type INTEGER,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER,"
    "link_to_id UNSIGNED INTEGER,"
    "link_to_type UNSIGNED INTEGER,"
    "link_extra UNSIGNED INTEGER,"
    "link_to_mrl TEXT"
")",

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT * FROM " + parser::Task::Table::Name,

"DROP TABLE " + parser::Task::Table::Name,

parser::Task::schema( parser::Task::Table::Name, 27 ),

/*
 * Don't bother migrate the retry_count since we force a rescan which will
 * reset it anyway
 */
"INSERT INTO " + parser::Task::Table::Name +
    " SELECT * FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " + parser::Task::Table::Name + "_backup",

parser::Task::index( parser::Task::Indexes::ParentFolderId, 27 ),

Device::schema( Device::MountpointTable::Name, 27 ),
