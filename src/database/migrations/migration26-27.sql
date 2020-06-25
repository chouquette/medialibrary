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
Media::index( Media::Indexes::LastUsageDate, 27 ),
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

#if defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE))

"UPDATE " + Media::Table::Name + " AS m SET progress = (" +

#ifdef __ANDROID__
"SELECT CAST(value AS REAL) / m.duration"
#else
"SELECT CAST(value AS REAL)"
#endif

" FROM " + Metadata::Table::Name + " md "
    " WHERE entity_type = " +
        std::to_string(static_cast<std::underlying_type_t<IMetadata::EntityType>>(
            IMetadata::EntityType::Media ) ) +
    " AND type = 50 AND md.id_media = m.id_media "
")"
#ifdef __ANDROID__
" WHERE m.duration > 0",
#endif

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
