"DROP TABLE " + AlbumTrack::Table::Name,
AlbumTrack::schema( AlbumTrack::Table::Name, 20 ),

"DROP TABLE " + ShowEpisode::Table::Name,
ShowEpisode::schema( ShowEpisode::Table::Name, 20 ),

"DROP TABLE " + Genre::Table::Name,
"DROP TABLE " + Genre::FtsTable::Name,
Genre::schema( Genre::Table::Name, 20 ),
Genre::schema( Genre::FtsTable::Name, 20 ),

/* Migrate the task table to update the Task.link_to_id field to a NOT NULL one
 * and update the UNIQUE constaint on (task type/mrl/link_to_id) accordingly */

parser::Task::schema( parser::Task::Table::Name, 19, true ),

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT * FROM " + parser::Task::Table::Name,

"DROP TABLE " + parser::Task::Table::Name,

parser::Task::schema( parser::Task::Table::Name, 20, false ),

"INSERT INTO " + parser::Task::Table::Name +
    " SELECT id_task, step, retry_count, type, mrl, file_type, file_id,"
        "parent_folder_id, ifnull(link_to_id, 0), link_to_type, link_extra"
    " FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " +  parser::Task::Table::Name + "_backup",

/*******************************************************************************
 * Migrate tables so their schema matches our expectations
*******************************************************************************/

/* Folder table */

"CREATE TABLE " + Folder::Table::Name + "_backup"
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "name TEXT COLLATE NOCASE,"
    "parent_id UNSIGNED INTEGER,"
    "is_banned BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Folder::Table::Name + "_backup "
    "SELECT * FROM " + Folder::Table::Name,

"DROP TABLE " + Folder::Table::Name,

Folder::schema( Folder::Table::Name, 20 ),

"INSERT INTO " + Folder::Table::Name + " "
    "SELECT * FROM " + Folder::Table::Name + "_backup ",

"DROP TABLE " + Folder::Table::Name + "_backup",

/* ExcludedFolderTable */

"CREATE TABLE " + Folder::ExcludedFolderTable::Name + "_backup"
"("
    "folder_id UNSIGNED INTEGER NOT NULL"
")",

"INSERT INTO " + Folder::ExcludedFolderTable::Name + "_backup "
    "SELECT * FROM " + Folder::ExcludedFolderTable::Name,

"DROP TABLE " + Folder::ExcludedFolderTable::Name,

Folder::schema( Folder::ExcludedFolderTable::Name, 20 ),

"INSERT INTO " + Folder::ExcludedFolderTable::Name + " "
    "SELECT * FROM " + Folder::ExcludedFolderTable::Name + "_backup ",

"DROP TABLE " + Folder::ExcludedFolderTable::Name + "_backup",

/* Thumbnail Linking Table */

"DROP TABLE " + Thumbnail::LinkingTable::Name,
Thumbnail::schema( Thumbnail::LinkingTable::Name, 20 ),

/* File table */
"CREATE TABLE " + File::Table::Name + "_backup"
"("
    "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
    "media_id UNSIGNED INT DEFAULT NULL,"
    "playlist_id UNSIGNED INT DEFAULT NULL,"
    "mrl TEXT,"
    "type UNSIGNED INTEGER,"
    "last_modification_date UNSIGNED INT,"
    "size UNSIGNED INT,"
    "folder_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "is_external BOOLEAN NOT NULL,"
    "is_network BOOLEAN NOT NULL"
")",

"INSERT INTO " + File::Table::Name + "_backup "
    "SELECT * FROM " + File::Table::Name,

"DROP TABLE " + File::Table::Name,

File::schema( File::Table::Name, 20 ),

"INSERT INTO " + File::Table::Name + " "
    "SELECT * FROM " + File::Table::Name + "_backup",

"DROP TABLE " + File::Table::Name + "_backup",

/* Label table */
"CREATE TEMPORARY TABLE " + Label::Table::Name + "_backup"
"("
    "id_label INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT UNIQUE ON CONFLICT FAIL"
")",

"INSERT INTO " + Label::Table::Name + "_backup "
    "SELECT * FROM " + Label::Table::Name,

"DROP TABLE " + Label::Table::Name,

Label::schema( Label::Table::Name, 20 ),

"INSERT INTO " + Label::Table::Name + " "
    "SELECT * FROM " + Label::Table::Name + "_backup",

"DROP TABLE " + Label::Table::Name + "_backup",

/* Label/File relation table */
"CREATE TEMPORARY TABLE " + Label::FileRelationTable::Name + "_backup"
"("
    "label_id INTEGER,"
    "media_id INTEGER,"
    "PRIMARY KEY(label_id,media_id)"
")",

"INSERT INTO " + Label::FileRelationTable::Name + "_backup "
    "SELECT * FROM " + Label::FileRelationTable::Name,

"DROP TABLE " + Label::FileRelationTable::Name,

Label::schema( Label::FileRelationTable::Name, 20 ),

"INSERT INTO " + Label::FileRelationTable::Name + " "
    "SELECT * FROM " + Label::FileRelationTable::Name + "_backup",

/* Playlist table */
"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    "id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup "
    "SELECT * FROM " + Playlist::Table::Name,

"DROP TABLE " + Playlist::Table::Name,

Playlist::schema( Playlist::Table::Name, 20 ),

"INSERT INTO " + Playlist::Table::Name + " "
    "SELECT * FROM " + Playlist::Table::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",

/* Album table */
"DROP TABLE " + Album::Table::Name,
"DROP TABLE " + Album::FtsTable::Name,
Album::schema( Album::Table::Name, 20 ),
Album::schema( Album::FtsTable::Name, 20 ),

/* Show table */
"DROP TABLE " + Show::Table::Name,
"DROP TABLE " + Show::FtsTable::Name,
Show::schema( Show::Table::Name, 20 ),
Show::schema( Show::FtsTable::Name, 20 ),

/* VideoTrack Table */
"DROP TABLE " + VideoTrack::Table::Name,
VideoTrack::schema( VideoTrack::Table::Name, 20 ),

/* Audio Track table */
"DROP TABLE " + AudioTrack::Table::Name,
AudioTrack::schema( AudioTrack::Table::Name, 20 ),

/* Artist table */
"DROP TABLE " + Artist::Table::Name,
"DROP TABLE " + Artist::FtsTable::Name,
Artist::schema( Artist::Table::Name, 20 ),
Artist::schema( Artist::FtsTable::Name, 20 ),

/* Artist MediaRelation table */

"DROP TABLE " + Artist::MediaRelationTable::Name,
Artist::schema( Artist::MediaRelationTable::Name, 20 ),

/* Metadata table */
"CREATE TEMPORARY TABLE " + Metadata::Table::Name + "_backup"
"("
    "id_media INTEGER,"
    "entity_type INTEGER,"
    "type INTEGER,"
    "value TEXT,"
    "PRIMARY KEY(id_media,entity_type,type)"
")",

"INSERT INTO " + Metadata::Table::Name + "_backup "
    "SELECT * FROM " + Metadata::Table::Name,

"DROP TABLE " + Metadata::Table::Name,

Metadata::schema( Metadata::Table::Name, 20 ),

"INSERT INTO " + Metadata::Table::Name + " "
    "SELECT * FROM " + Metadata::Table::Name + "_backup",

"DROP TABLE " + Metadata::Table::Name + "_backup",

/* Subtitle track table */
"DROP TABLE " + SubtitleTrack::Table::Name,
SubtitleTrack::schema( SubtitleTrack::Table::Name, 20 ),

/* Chapter table */
"DROP TABLE IF EXISTS " + Chapter::Table::Name,
Chapter::schema( Chapter::Table::Name, 20 ),

/* Bookmark table */
"CREATE TEMPORARY TABLE " + Bookmark::Table::Name + "_backup"
"("
    "id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,"
    "time UNSIGNED INTEGER NOT NULL,"
    "name TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INTEGER NOT NULL"
")",

"INSERT INTO " + Bookmark::Table::Name + "_backup "
    "SELECT * FROM " + Bookmark::Table::Name,

"DROP TABLE " + Bookmark::Table::Name,

Bookmark::schema( Bookmark::Table::Name, 20 ),

"INSERT INTO " + Bookmark::Table::Name + " "
    "SELECT * FROM " + Bookmark::Table::Name + "_backup",

"DROP TABLE " + Bookmark::Table::Name + "_backup",

Album::trigger( Album::Triggers::AddTrack, 20 ),
Album::trigger( Album::Triggers::DeleteTrack, 20 ),
Album::trigger( Album::Triggers::InsertFts, 20 ),
Album::trigger( Album::Triggers::DeleteFts, 20 ),
Album::index( Album::Indexes::ArtistId, 20 ),

AlbumTrack::index( AlbumTrack::Indexes::AlbumGenreArtist, 20 ),
AlbumTrack::index( AlbumTrack::Indexes::MediaArtistGenreAlbum, 20 ),

ShowEpisode::index( ShowEpisode::Indexes::MediaIdShowId, 20 ),

Artist::trigger( Artist::Triggers::DeleteArtistsWithoutTracks, 20 ),
Artist::trigger( Artist::Triggers::HasAlbumRemaining, 20 ),
Artist::trigger( Artist::Triggers::InsertFts, 20 ),
Artist::trigger( Artist::Triggers::DeleteFts, 20 ),

Media::trigger( Media::Triggers::CascadeFileDeletion, 20 ),

Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteAlbum, 20 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteArtist, 20 ),
Thumbnail::trigger( Thumbnail::Triggers::IncrementRefcount, 20 ),
Thumbnail::trigger( Thumbnail::Triggers::DecrementRefcount, 20 ),
Thumbnail::trigger( Thumbnail::Triggers::UpdateRefcount, 20 ),

Folder::trigger( Folder::Triggers::InsertFts, 20 ),
Folder::trigger( Folder::Triggers::DeleteFts, 20 ),

Genre::trigger( Genre::Triggers::InsertFts, 20 ),
Genre::trigger( Genre::Triggers::DeleteFts, 20 ),
Genre::trigger( Genre::Triggers::UpdateOnNewTrack, 20 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 20 ),

Label::trigger( Label::Triggers::DeleteFts, 20 ),

Playlist::trigger( Playlist::Triggers::InsertFts, 20 ),
Playlist::trigger( Playlist::Triggers::UpdateFts, 20 ),
Playlist::trigger( Playlist::Triggers::DeleteFts, 20 ),
Playlist::index( Playlist::Indexes::FileId, 20 ),

parser::Task::trigger( parser::Task::Triggers::DeletePlaylistLinkingTask, 20 ),

AudioTrack::index( AudioTrack::Indexes::MediaId, 20 ),

VideoTrack::index( VideoTrack::Indexes::MediaId, 20 ),

SubtitleTrack::index( SubtitleTrack::Indexes::MediaId, 20 ),

Folder::index( Folder::Indexes::DeviceId, 20 ),
Folder::index( Folder::Indexes::ParentId, 20 ),

File::index( File::Indexes::MediaId, 20 ),
File::index( File::Indexes::FolderId, 20 ),
