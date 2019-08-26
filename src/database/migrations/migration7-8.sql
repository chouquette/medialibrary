"CREATE TEMPORARY TABLE " + Artist::Table::Name + "_backup("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + Artist::Table::Name + "_backup SELECT * FROM " + Artist::Table::Name + ";",

"DROP TABLE " + Artist::Table::Name + ";",

"CREATE TABLE " + Artist::Table::Name + "("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "nb_tracks UNSIGNED INT DEFAULT 0," // Added
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + Artist::Table::Name + "("
    "id_artist,"
    "name,"
    "shortbio,"
    "artwork_mrl,"
    "nb_albums,"
    "mb_id,"
    "is_present"
")"
" SELECT * FROM " + Artist::Table::Name + "_backup;",

"DROP TABLE " + Artist::Table::Name + "_backup;",

"UPDATE " + Artist::Table::Name + " SET nb_tracks = ("
    "SELECT COUNT(id_track) FROM " + AlbumTrack::Table::Name + " WHERE "
    "artist_id = " + Artist::Table::Name + ".id_artist"
")",

"INSERT INTO " + parser::Task::Table::Name + " (file_type, step, retry_count, file_id, parent_folder_id) "
"SELECT " + std::to_string( static_cast<std::underlying_type<IFile::Type>::type>(
    IFile::Type::Main ) ) + ","
"parser_step, parser_retries, id_file, folder_id FROM " + File::Table::Name,

"CREATE TEMPORARY TABLE " + File::Table::Name + "_backup("
  "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
  "media_id INT NOT NULL,"
  "playlist_id UNSIGNED INT DEFAULT NULL,"
  "mrl TEXT,"
  "type UNSIGNED INTEGER,"
  "last_modification_date UNSIGNED INT,"
  "size UNSIGNED INT,"
  "parser_step INTEGER NOT NULL DEFAULT 0,"
  "parser_retries INTEGER NOT NULL DEFAULT 0,"
  "folder_id UNSIGNED INTEGER,"
  "is_present BOOLEAN NOT NULL DEFAULT 1,"
  "is_removable BOOLEAN NOT NULL,"
  "is_external BOOLEAN NOT NULL,"
  "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name + "(id_playlist) ON DELETE CASCADE,"
  "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + File::Table::Name + "_backup SELECT * FROM " + File::Table::Name + ";",

"DROP TABLE " + File::Table::Name + ";",

"CREATE TABLE " + File::Table::Name + "(id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "media_id UNSIGNED INT DEFAULT NULL,"
                  "playlist_id UNSIGNED INT DEFAULT NULL,"
                  "mrl TEXT,"
                  "type UNSIGNED INTEGER,"
                  "last_modification_date UNSIGNED INT,"
                  "size UNSIGNED INT,"
                  // Removed parsed_step & parser_retries
                  "folder_id UNSIGNED INTEGER,"
                  "is_present BOOLEAN NOT NULL DEFAULT 1,"
                  "is_removable BOOLEAN NOT NULL,"
                  "is_external BOOLEAN NOT NULL,"
  "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name + "(id_playlist) ON DELETE CASCADE,"
  "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + File::Table::Name + "("
    "id_file, media_id, playlist_id, mrl, type, last_modification_date, size, folder_id,"
    "is_present, is_removable, is_external)"
 " SELECT "
    "id_file, media_id, playlist_id, mrl, type, last_modification_date, size, folder_id,"
    "is_present, is_removable, is_external"
 " FROM " + File::Table::Name + "_backup;",

"DROP TABLE " + File::Table::Name + "_backup;",
