/******************* Migrate Media table **************************************/
"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER,"
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail TEXT,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT * FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 14 ),

"INSERT INTO " + Media::Table::Name + "("
    "id_media, type, subtype, duration, play_count, last_played_date, real_last_played_date, insertion_date,"
    "release_date, thumbnail_id, title, filename, is_favorite,"
    "is_present) "
"SELECT id_media, type, ifnull(subtype, " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                    IMedia::SubType::Unknown ) )
    + "), duration, play_count, last_played_date,"
    "strftime('%s', 'now'),"
    "insertion_date, release_date, 0,"
    "title, filename, is_favorite, is_present FROM " + Media::Table::Name + "_backup",

"DROP TABLE " + Media::Table::Name + "_backup",

/******************* Populate new media.nb_playlists **************************/

"UPDATE " + Media::Table::Name + " SET nb_playlists = "
"(SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
" WHERE media_id = id_media )"
"WHERE id_media IN (SELECT media_id FROM " + Playlist::MediaRelationTable::Name + ")",

/*************** Populate new media.device_id & folder_id *********************/

"UPDATE " + Media::Table::Name + " SET (device_id, folder_id) = "
"(SELECT d.id_device, f.id_folder FROM " + Device::Table::Name + " d "
"INNER JOIN " + Folder::Table::Name + " f ON d.id_device = f.device_id "
"INNER JOIN " + File::Table::Name + " fi ON fi.folder_id = f.id_folder "
"WHERE fi.type = " +
    std::to_string( static_cast<typename std::underlying_type<IFile::Type>::type>(
        IFile::Type::Main) ) + " "
"AND fi.media_id = " + Media::Table::Name + ".id_media"
")",

/************ Playlist external media were stored as Unknown ******************/
/* External type was removed in model 23 but used to be equal to 3 */
"UPDATE " + Media::Table::Name + " SET type = 3 "
"WHERE nb_playlists > 0 AND "
"type = " + std::to_string( static_cast<typename std::underlying_type<IMedia::Type>::type>(
IMedia::Type::Unknown ) ),

/******************* Migrate metadata table ***********************************/
"CREATE TEMPORARY TABLE " + Metadata::Table::Name + "_backup"
"("
    "id_media INTEGER,"
    "type INTEGER,"
    "value TEXT"
")",

"INSERT INTO " + Metadata::Table::Name + "_backup SELECT * FROM MediaMetadata",

"DROP TABLE MediaMetadata",

// Recreate the new table
Metadata::schema( Metadata::Table::Name, 14 ),

"INSERT INTO " + Metadata::Table::Name + " "
"SELECT "
    "id_media, " + std::to_string(
        static_cast<typename std::underlying_type<IMetadata::EntityType>::type>(
            IMetadata::EntityType::Media ) ) +
    ", type, value "
"FROM " + Metadata::Table::Name + "_backup",

"DROP TABLE " + Metadata::Table::Name + "_backup",

/******************* Migrate the playlist table *******************************/
"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup"
"("
    + Playlist::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT"
")",

"CREATE TEMPORARY TABLE " + Playlist::MediaRelationTable::Name + "_backup"
"("
    "media_id INTEGER,"
    "playlist_id INTEGER,"
    "position INTEGER"
")",

"INSERT INTO " + Playlist::Table::Name + "_backup SELECT * FROM " + Playlist::Table::Name,
"INSERT INTO " + Playlist::MediaRelationTable::Name + "_backup SELECT * FROM " + Playlist::MediaRelationTable::Name,

"DROP TABLE " + Playlist::Table::Name,
"DROP TABLE " + Playlist::MediaRelationTable::Name,

Playlist::schema( Playlist::Table::Name, 14 ),
Playlist::schema( Playlist::MediaRelationTable::Name, 14 ),

"INSERT INTO " + Playlist::Table::Name + " SELECT * FROM " + Playlist::Table::Name + "_backup",
"INSERT INTO " + Playlist::MediaRelationTable::Name + " SELECT media_id, NULL, playlist_id, position "
    "FROM " + Playlist::MediaRelationTable::Name + "_backup",

"DROP TABLE " + Playlist::Table::Name + "_backup",
"DROP TABLE " + Playlist::MediaRelationTable::Name + "_backup",

/******************* Migrate Device table *************************************/

"CREATE TEMPORARY TABLE " + Device::Table::Name + "_backup"
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT UNIQUE ON CONFLICT FAIL,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN"
")",

"INSERT INTO " + Device::Table::Name + "_backup SELECT * FROM " + Device::Table::Name,

"DROP TABLE " + Device::Table::Name,

Device::schema( Device::Table::Name, 14 ),

"INSERT INTO " + Device::Table::Name + " SELECT id_device, uuid, scheme, is_removable, is_present,"
    "strftime('%s', 'now') FROM " + Device::Table::Name + "_backup",

"DROP TABLE " + Device::Table::Name + "_backup",

/******************* Migrate Folder table *************************************/

"CREATE TEMPORARY TABLE " + Folder::Table::Name + "_backup"
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "parent_id UNSIGNED INTEGER,"
    "is_blacklisted BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "is_removable BOOLEAN NOT NULL"
")",

"INSERT INTO " + Folder::Table::Name + "_backup SELECT * FROM " + Folder::Table::Name,

"DROP TABLE " + Folder::Table::Name,

Folder::schema( Folder::Table::Name, 14 ),
Folder::schema( Folder::FtsTable::Name, 14 ),

Folder::index( Folder::Indexes::DeviceId, 14 ),
Folder::index( Folder::Indexes::ParentId, 14 ),
Folder::trigger( Folder::Triggers::InsertFts, 14 ),
Folder::trigger( Folder::Triggers::DeleteFts, 14 ),

"INSERT INTO " + Folder::Table::Name + "("
    "id_folder, path, parent_id, is_banned, device_id, is_removable"
") "
"SELECT id_folder, path, parent_id, is_blacklisted, device_id, is_removable "
"FROM " + Folder::Table::Name + "_backup",

"UPDATE " + Folder::Table::Name + " SET "
    "nb_audio = (SELECT COUNT() FROM " + Media::Table::Name +
        " m WHERE m.folder_id = id_folder AND m.type = "
                + std::to_string( static_cast<std::underlying_type<IMedia::Type>::type>(
                                    IMedia::Type::Audio ) ) + "),"
    "nb_video = (SELECT COUNT() FROM " + Media::Table::Name +
        " m WHERE m.folder_id = id_folder AND m.type = "
            + std::to_string( static_cast<std::underlying_type<IMedia::Type>::type>(
                                IMedia::Type::Video ) ) + ")",

"DROP TABLE " + Folder::Table::Name + "_backup",

/******************* Migrate File table *************************************/

"CREATE TEMPORARY TABLE " + File::Table::Name + "_backup"
"("
    "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
    "media_id UNSIGNED INT DEFAULT NULL,"
    "playlist_id UNSIGNED INT DEFAULT NULL,"
    "mrl TEXT,"
    "type UNSIGNED INTEGER,"
    "last_modification_date UNSIGNED INT,"
    "size UNSIGNED INT,"
    "folder_id UNSIGNED INTEGER,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "is_removable BOOLEAN NOT NULL,"
    "is_external BOOLEAN NOT NULL"
")",

"INSERT INTO " + File::Table::Name + "_backup SELECT * FROM " + File::Table::Name,

"DROP TABLE " + File::Table::Name,

File::schema( File::Table::Name, 14 ),

"INSERT INTO " + File::Table::Name + "("
"id_file, media_id, playlist_id, mrl, type, last_modification_date, size,"
"folder_id, is_removable, is_external, is_network) "
"SELECT id_file, media_id, playlist_id, mrl, type, last_modification_date, size,"
"folder_id, is_removable, is_external, 0 FROM " + File::Table::Name + "_backup",

"DROP TABLE " + File::Table::Name + "_backup",

/******************* Delete removed triggers **********************************/

"DROP TRIGGER on_track_genre_changed",

// This trigger has changed and needs to be recreated
// However if migrating from a model below version 13, migration 12->13 will have
// already dropped it
"DROP TRIGGER IF EXISTS is_album_present",

// Other outdated triggers that we want to regenerate, however those we are sure
// to find and be able to delete (hence no `IF EXISTS` in the request)
"DROP TRIGGER add_album_track",
"DROP TRIGGER delete_album_track",

// Old Folder -> File is_present trigger
// is_folder_present has been implicitely removed by dropping the File table

// Old File -> Media is_present trigger
// has_files_present has been implicitely removed by dropping the Folder table

// Old Device -> Folder is_present trigger
// is_device_present has been implicitely removed by dropping the Device table

/******************* Delete other tables **************************************/

"DROP TABLE " + Album::Table::Name,
"DROP TABLE AlbumArtistRelation",
"DELETE FROM " + Album::Table::Name + "Fts",
"DROP TABLE " + Artist::Table::Name,
"DELETE FROM " + Artist::Table::Name + "Fts",
"DELETE FROM MediaArtistRelation",
"DROP TABLE " + Movie::Table::Name,
"DROP TABLE " + Show::Table::Name,
// No need to delete the ShowFts table since... it didn't exist
"DROP TABLE " + VideoTrack::Table::Name,
// Flush the audio track table to recreate all tracks
"DELETE FROM " + AudioTrack::Table::Name,

// History table & its triggers were removed for good:
"DROP TABLE History",

Media::trigger( Media::Triggers::IsPresent, 14 ),

Movie::schema( Movie::Table::Name, 14 ),
Movie::index( Movie::Indexes::MediaId, 14 ),
