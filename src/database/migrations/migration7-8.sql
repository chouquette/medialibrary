"CREATE TEMPORARY TABLE " + ArtistTable::Name + "_backup("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + ArtistTable::Name + "_backup SELECT * FROM " + ArtistTable::Name + ";",

"DROP TABLE " + ArtistTable::Name + ";",

"CREATE TABLE " + ArtistTable::Name + "("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "nb_tracks UNSIGNED INT DEFAULT 0," // Added
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + ArtistTable::Name + "("
    "id_artist,"
    "name,"
    "shortbio,"
    "artwork_mrl,"
    "nb_albums,"
    "mb_id,"
    "is_present"
")"
" SELECT * FROM " + ArtistTable::Name + "_backup;",

"DROP TABLE " + ArtistTable::Name + "_backup;",

"UPDATE " + ArtistTable::Name + " SET nb_tracks = ("
    "SELECT COUNT(id_track) FROM " + AlbumTrackTable::Name + " WHERE "
    "artist_id = " + ArtistTable::Name + ".id_artist"
")",

"CREATE TEMPORARY TABLE " + FileTable::Name + "_backup("
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
  "FOREIGN KEY (media_id) REFERENCES " + MediaTable::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY (playlist_id) REFERENCES " + PlaylistTable::Name + "(id_playlist) ON DELETE CASCADE,"
  "FOREIGN KEY (folder_id) REFERENCES " + FolderTable::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + FileTable::Name + "_backup SELECT * FROM " + FileTable::Name + ";",

"DROP TABLE " + FileTable::Name + ";",

"CREATE TABLE " + FileTable::Name + "(id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
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
  "FOREIGN KEY (media_id) REFERENCES " + MediaTable::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY (playlist_id) REFERENCES " + PlaylistTable::Name + "(id_playlist) ON DELETE CASCADE,"
  "FOREIGN KEY (folder_id) REFERENCES " + FolderTable::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + FileTable::Name + "("
    "id_file, media_id, playlist_id, mrl, type, last_modification_date, size, folder_id,"
    "is_present, is_removable, is_external)"
 " SELECT "
    "id_file, media_id, playlist_id, mrl, type, last_modification_date, size, folder_id,"
    "is_present, is_removable, is_external"
 " FROM " + FileTable::Name + "_backup;",

"DROP TABLE " + FileTable::Name + "_backup;",
