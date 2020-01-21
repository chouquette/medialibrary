MediaGroup::schema( MediaGroup::Table::Name, 24 ),
MediaGroup::schema( MediaGroup::FtsTable::Name, 24 ),

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
"nb_playlists, folder_id, import_type, NULL, 0, 0 "
" FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

// Recreate the IncrementNbPlaylist & DecrementNbPlaylist triggers so they
// pass the integrity checks
"DROP TRIGGER " + Media::triggerName( Media::Triggers::IncrementNbPlaylist, 23 ),
"DROP TRIGGER " + Media::triggerName( Media::Triggers::DecrementNbPlaylist, 23 ),

Media::trigger( Media::Triggers::IncrementNbPlaylist, 24 ),
Media::trigger( Media::Triggers::DecrementNbPlaylist, 24 ),
Media::trigger( Media::Triggers::InsertFts, 24 ),
Media::trigger( Media::Triggers::UpdateFts, 24 ),
Media::trigger( Media::Triggers::DeleteFts, 24 ),

Media::index( Media::Indexes::LastPlayedDate, 24 ),
Media::index( Media::Indexes::Presence, 24 ),
Media::index( Media::Indexes::Types, 24 ),
Media::index( Media::Indexes::LastUsageDate, 24 ),
Media::index( Media::Indexes::Folder, 24 ),
Media::index( Media::Indexes::MediaGroup, 24 ),

Album::trigger( Album::Triggers::IsPresent, 24 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 24 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 24 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 24 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 24 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 24 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 24 ),

/* Create the new media group triggers after recreating the media table, since
   deleting the media table would delete the triggers, as they depend on it */
MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::IncrementNbMediaOnGroupChange, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnGroupChange, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 24 ),
MediaGroup::index( MediaGroup::Indexes::ParentId, 24 ),

/* Remove the 2 VideoGroups related columns from the Settings table */
"DROP TABLE Settings",

/* Recreate PlaylistIdPosition trigger so that it passes sanity check */
"DROP INDEX " + Playlist::indexName( Playlist::Indexes::PlaylistIdPosition, 23 ),
Playlist::index( Playlist::Indexes::PlaylistIdPosition, 24 ),

/* Recreate MediaId index so that it passes integrity checks */
"DROP INDEX " + SubtitleTrack::indexName( SubtitleTrack::Indexes::MediaId, 23 ),
SubtitleTrack::index( SubtitleTrack::Indexes::MediaId, 24 ),

/* Recreate thumbnail linking index as it was invalid and indexing the thumbnail
   primary key */
"DROP INDEX " + Thumbnail::indexName( Thumbnail::Indexes::ThumbnailId, 23 ),
Thumbnail::index( Thumbnail::Indexes::ThumbnailId, 24 ),

/* Recreate the device table to update the UNIQUE constraint from (uuid) to
   (uuid, scheme) */
"CREATE TEMPORARY TABLE " + Device::Table::Name + "_backup"
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN,"
    "last_seen UNSIGNED INTEGER"
")",

"INSERT INTO " + Device::Table::Name + "_backup SELECT * FROM " + Device::Table::Name,

"DROP TABLE " + Device::Table::Name,
Device::schema( Device::Table::Name, 24 ),

"INSERT INTO " + Device::Table::Name + " SELECT * FROM " + Device::Table::Name + "_backup",

"DROP TABLE " + Device::Table::Name + "_backup",

/* Now recreate triggers based on the Device table */
Media::trigger( Media::Triggers::IsPresent, 24 ),

/* Create a new Task index */
parser::Task::index( parser::Task::Indexes::ParentFolderId, 24 ),

/* Add the ShowEpisode.episode_title column */
"CREATE TEMPORARY TABLE " + ShowEpisode::Table::Name + "_backup"
"("
    "id_episode INTEGER PRIMARY KEY AUTOINCREMENT,"
    "media_id UNSIGNED INTEGER NOT NULL,"
    "episode_number UNSIGNED INT,"
    "season_number UNSIGNED INT,"
    "episode_summary TEXT,"
    "tvdb_id TEXT,"
    "show_id UNSIGNED INT"
")",

"INSERT INTO " + ShowEpisode::Table::Name + "_backup "
    "SELECT * FROM " + ShowEpisode::Table::Name,

"DROP TABLE " + ShowEpisode::Table::Name,

ShowEpisode::schema( ShowEpisode::Table::Name, 24 ),

"INSERT INTO " + ShowEpisode::Table::Name +
    " SELECT id_episode, media_id, episode_number, season_number, "
    "(SELECT title FROM " + Media::Table::Name + " WHERE id_media = media_id), "
    "episode_summary, tvdb_id, show_id FROM " + ShowEpisode::Table::Name + "_backup",

"DROP TABLE " + ShowEpisode::Table::Name + "_backup",

Show::trigger( Show::Triggers::IncrementNbEpisode, 24 ),
Show::trigger( Show::Triggers::DecrementNbEpisode, 24 ),
ShowEpisode::index( ShowEpisode::Indexes::MediaIdShowId, 24 ),
