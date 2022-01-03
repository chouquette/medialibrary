"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        utils::enum_to_string( Media::SubType::Unknown ) + ","
    "duration INTEGER DEFAULT -1,"
    "last_position REAL DEFAULT -1,"
    "last_time INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER NON NULL DEFAULT 0,"
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
    "forced_title BOOLEAN NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Media::Table::Name + "_backup"
    " SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,
Media::schema( Media::Table::Name, 34 ),

"INSERT INTO " + Media::Table::Name +
    " SELECT *, NULL, NULL, 0, NULL, 0, NULL FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

"DROP TABLE " + AlbumTrack::Table::Name,

"CREATE TEMPORARY TABLE " + AudioTrack::Table::Name + "_backup"
"("
    + AudioTrack::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "codec TEXT,"
    "bitrate UNSIGNED INTEGER,"
    "samplerate UNSIGNED INTEGER,"
    "nb_channels UNSIGNED INTEGER,"
    "language TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INT,"
    "attached_file_id UNSIGNED INT,"
    "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
        + "(id_media) ON DELETE CASCADE,"
    "FOREIGN KEY(attached_file_id) REFERENCES " + File::Table::Name +
        "(id_file) ON DELETE CASCADE"
")",

"INSERT INTO " + AudioTrack::Table::Name + "_backup "
    "SELECT * FROM " + AudioTrack::Table::Name,

"DROP TABLE " + AudioTrack::Table::Name,

AudioTrack::schema( AudioTrack::Table::Name, 34 ),

"INSERT INTO " + AudioTrack::Table::Name +
    " SELECT * FROM " + AudioTrack::Table::Name + "_backup",

"DROP TABLE " + AudioTrack::Table::Name + "_backup",

"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    + Playlist::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT,"
    "nb_video UNSIGNED INT NOT NULL DEFAULT 0,"
    "nb_audio UNSIGNED INT NOT NULL DEFAULT 0,"
    "nb_unknown UNSIGNED INT NOT NULL DEFAULT 0,"
    "nb_present_video UNSIGNED INT NOT NULL DEFAULT 0,"
    "nb_present_audio UNSIGNED INT NOT NULL DEFAULT 0,"
    "nb_present_unknown UNSIGNED INT NOT NULL DEFAULT 0,"
    "duration UNSIGNED INT NOT NULL DEFAULT 0,"
    "FOREIGN KEY(file_id) REFERENCES " + File::Table::Name
    + "(id_file) ON DELETE CASCADE"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup "
    "SELECT * FROM " + Playlist::Table::Name,

"DROP TABLE " + Playlist::Table::Name,

Playlist::schema( Playlist::Table::Name, 34 ),

"INSERT INTO " + Playlist::Table::Name +
    " SELECT *, (SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name + " mrt "
        " INNER JOIN " + Media::Table::Name + " m ON m.id_media = mrt.media_id "
        " WHERE m.duration <= 0 AND mrt.playlist_id = id_playlist)"
    " FROM " + Playlist::Table::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",

AudioTrack::index( AudioTrack::Indexes::MediaId, 34 ),

Media::trigger( Media::Triggers::InsertFts, 34 ),
Media::trigger( Media::Triggers::UpdateFts, 34 ),
Media::trigger( Media::Triggers::DeleteFts, 34 ),

Media::index( Media::Indexes::LastPlayedDate, 34 ),
Media::index( Media::Indexes::Presence, 34 ),
Media::index( Media::Indexes::Types, 34 ),
Media::index( Media::Indexes::InsertionDate, 34 ),
Media::index( Media::Indexes::Folder, 34 ),
Media::index( Media::Indexes::MediaGroup, 34 ),
Media::index( Media::Indexes::Progress, 34 ),
Media::index( Media::Indexes::AlbumTrack, 34 ),
Media::index( Media::Indexes::Duration, 34 ),
Media::index( Media::Indexes::ReleaseDate, 34 ),
Media::index( Media::Indexes::PlayCount, 34 ),
Media::index( Media::Indexes::Title, 34 ),
Media::index( Media::Indexes::FileName, 34 ),
Media::index( Media::Indexes::GenreId, 34 ),
Media::index( Media::Indexes::ArtistId, 34 ),
Album::trigger( Album::Triggers::IsPresent, 34 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 34 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 34 ),
Show::trigger( Show::Triggers::UpdateIsPresent, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnIndex, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 34 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 34 ),
Genre::trigger( Genre::Triggers::UpdateIsPresent, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaDeletion, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateDurationOnMediaChange, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateNbMediaOnMediaChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateMediaCountOnPresenceChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::DecrementNbMediaOnDeletion, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::RenameForcedSingleton, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaChange, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateDurationOnMediaDeletion, 34 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 34 ),
Playlist::trigger( Playlist::Triggers::InsertFts, 34 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 34 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 34 ),
Playlist::index( Playlist::Indexes::FileId, 34 ),
parser::Task::trigger( parser::Task::Triggers::DeletePlaylistLinkingTask, 34 ),

// This trigger was automatically deleted with the AlbumTrack table
Album::trigger( Album::Triggers::DeleteTrack, 34 ),

"DROP TRIGGER " + Album::triggerName( Album::Triggers::IsPresent, 33 ),
Album::trigger( Album::Triggers::IsPresent, 34 ),

"DROP TRIGGER " + Artist::triggerName( Artist::Triggers::HasTrackPresent, 33 ),
Artist::trigger( Artist::Triggers::HasTrackPresent, 34 ),

// This trigger was automatically deleted with the AlbumTrack table
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 34 ),

// New triggers
Album::trigger( Album::Triggers::DeleteEmpty, 34 ),
Genre::trigger( Genre::Triggers::DeleteEmpty, 34 ),
Playlist::trigger( Playlist::Triggers::CascadeFileDeletion, 34 ),

// New indexes
Album::index( Album::Indexes::NbTracks, 34 ),
AudioTrack::index( AudioTrack::Indexes::AttachedFileId, 34 ),
Bookmark::index( Bookmark::Indexes::MediaId, 34 ),
Chapter::index( Chapter::Indexes::MediaId, 34 ),
File::index( File::Indexes::PlaylistId, 34 ),
Label::index( Label::Indexes::MediaId, 34 ),
Artist::index( Artist::Indexes::MediaRelArtistId, 34 ),
Playlist::index( Playlist::Indexes::PlaylistRelMediaId, 34 ),
ShowEpisode::index( ShowEpisode::Indexes::ShowId, 34 ),
SubtitleTrack::index( SubtitleTrack::Indexes::AttachedFileId, 34 ),
parser::Task::index( parser::Task::Indexes::FileId, 34 ),

// Renamed indexes
"DROP INDEX " + ShowEpisode::indexName( ShowEpisode::Indexes::MediaId, 33 ),
ShowEpisode::index( ShowEpisode::Indexes::MediaId, 34 ),

