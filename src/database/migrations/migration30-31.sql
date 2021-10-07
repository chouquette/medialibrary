"DROP TRIGGER " + Folder::triggerName( Folder::Triggers::UpdateNbMediaOnUpdate, 30 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 31 ),

"CREATE TEMPORARY TABLE " + MediaGroup::Table::Name + "_backup "
"("
    "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "nb_video UNSIGNED INTEGER DEFAULT 0,"
    "nb_audio UNSIGNED INTEGER DEFAULT 0,"
    "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_video UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_audio UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_unknown UNSIGNED INTEGER DEFAULT 0,"
    "duration INTEGER DEFAULT 0,"
    "creation_date INTEGER NOT NULL,"
    "last_modification_date INTEGER NOT NULL,"
    "user_interacted BOOLEAN,"
    "forced_singleton BOOLEAN"
")",

"INSERT INTO " + MediaGroup::Table::Name + "_backup "
    "SELECT * FROM " + MediaGroup::Table::Name,

"DROP TABLE " + MediaGroup::Table::Name,

MediaGroup::schema( MediaGroup::Table::Name, 31 ),

"INSERT INTO " + MediaGroup::Table::Name + " SELECT id_group, name, nb_video,"
    "nb_audio, nb_unknown, 0, nb_present_video, nb_present_audio,"
    "nb_present_unknown, duration, creation_date, last_modification_date,"
    "user_interacted, forced_singleton FROM "
    + MediaGroup::Table::Name + "_backup",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

"UPDATE " + MediaGroup::Table::Name + " SET "
    "nb_video = nb_video - (CASE MQ.type WHEN " +
        utils::enum_to_string( IMedia::Type::Video ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_video = nb_present_video - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                utils::enum_to_string( IMedia::Type::Video ) +
                " THEN 1 ELSE 0 END)"
        "END),"
    "nb_audio = nb_audio - (CASE MQ.type WHEN " +
        utils::enum_to_string( IMedia::Type::Audio ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_audio = nb_present_audio - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                utils::enum_to_string( IMedia::Type::Audio ) +
                " THEN 1 ELSE 0 END)"
        "END),"
    "nb_unknown = nb_unknown - (CASE MQ.type WHEN " +
        utils::enum_to_string( IMedia::Type::Unknown ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_unknown = nb_present_unknown - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                utils::enum_to_string( IMedia::Type::Unknown ) +
                " THEN 1 ELSE 0 END)"
        "END) "
    "FROM (SELECT * FROM " + Media::Table::Name + " WHERE import_type = "
        + utils::enum_to_string( Media::ImportType::External ) + ") as MQ "
    "WHERE MQ.group_id = id_group",

"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
   "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
   "type INTEGER,"
   "subtype INTEGER,"
   "duration INTEGER,"
   "progress REAL,"
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

Media::schema( Media::Table::Name, 31 ),

"INSERT INTO " + Media::Table::Name + " SELECT "
    "id_media, type, subtype, duration, progress,"
    "IIF(duration >= 0 AND progress >= 0, duration * progress, -1),"
    "play_count, last_played_date, real_last_played_date, insertion_date, release_date,"
    "title, filename, is_favorite, is_present, device_id, nb_playlists, folder_id,"
    "import_type, group_id, forced_title FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

Media::trigger( Media::Triggers::InsertFts, 31 ),
Media::trigger( Media::Triggers::UpdateFts, 31 ),
Media::trigger( Media::Triggers::DeleteFts, 31 ),

Media::index( Media::Indexes::LastPlayedDate, 31 ),
Media::index( Media::Indexes::Presence, 31 ),
Media::index( Media::Indexes::Types, 31 ),
Media::index( Media::Indexes::LastUsageDate, 31 ),
Media::index( Media::Indexes::Folder, 31 ),
Media::index( Media::Indexes::MediaGroup, 31 ),
Media::index( Media::Indexes::Progress, 31 ),

Album::trigger( Album::Triggers::IsPresent, 31 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 31 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 31 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 31 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 31 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 31 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 31 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 31 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 31 ),
Playlist::trigger( Playlist::Triggers::UpdateNbPresentMediaOnPresenceChange, 31 ),

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 31 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::DeleteEmptyGroups, 30 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 31 ),

MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 31 ),

MediaGroup::index( MediaGroup::Indexes::ForcedSingleton, 31 ),
MediaGroup::index( MediaGroup::Indexes::Duration, 31 ),
MediaGroup::index( MediaGroup::Indexes::CreationDate, 31 ),
MediaGroup::index( MediaGroup::Indexes::LastModificationDate, 31 ),

#if defined(__ANDROID__)

/*
 * Recover play_count from the seen meta to work around a bug in previous vlc-android
 * release. See #354
 */
"UPDATE " + Media::Table::Name + " AS m"
" SET play_count = meta.number"
" FROM (SELECT id_media, CAST(value AS decimal) AS number FROM " + Metadata::Table::Name +
    " WHERE entity_type = " + utils::enum_to_string( IMetadata::EntityType::Media ) +
        " AND type = 55"
            /* std::to_string( static_cast<std::underlying_type_t<IMedia::MetadataType>>(
                IMedia::MetadataType::Seen ) ) */
    ") AS meta"
" WHERE m.id_media = meta.id_media",

#endif
