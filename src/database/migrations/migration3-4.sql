"CREATE TEMPORARY TABLE " + FileTable::Name + "_backup("
  "id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
  "media_id INT NOT NULL,"
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
  "FOREIGN KEY (folder_id) REFERENCES " + FolderTable::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + FileTable::Name + "_backup SELECT * FROM " + FileTable::Name + ";",

"DROP TABLE " + FileTable::Name + ";",

"CREATE TABLE " + FileTable::Name + "(id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
                  "media_id UNSIGNED INT DEFAULT NULL,"
                  "playlist_id UNSIGNED INT DEFAULT NULL," // Added
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
  "FOREIGN KEY (playlist_id) REFERENCES " + PlaylistTable::Name + "(id_playlist) ON DELETE CASCADE," // Added
  "FOREIGN KEY (folder_id) REFERENCES " + FolderTable::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + FileTable::Name + "(id_file,media_id,mrl,type,last_modification_date,size,parser_step,parser_retries,folder_id,is_present,is_removable,is_external)"
 " SELECT * FROM " + FileTable::Name + "_backup;",

"DROP TABLE " + FileTable::Name + "_backup;",

"CREATE TEMPORARY TABLE " + PlaylistTable::Name + "_backup("
  "id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,"
  "name TEXT UNIQUE,"
  "creation_date UNSIGNED INT NOT NULL);",

"INSERT INTO " + PlaylistTable::Name + "_backup SELECT * FROM Playlist;",

"DROP TABLE " + PlaylistTable::Name + ";",

"CREATE TABLE " + PlaylistTable::Name + "("
  "id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,"
  "name TEXT UNIQUE,"
  "file_id UNSIGNED INT DEFAULT NULL," // Added
  "creation_date UNSIGNED INT NOT NULL,"
  "artwork_mrl TEXT," // Added
  "FOREIGN KEY (file_id) REFERENCES " + FileTable::Name + "(id_file) ON DELETE CASCADE);", // Added

"INSERT INTO " + PlaylistTable::Name + "(id_playlist,name,creation_date)"
 " SELECT * FROM " + PlaylistTable::Name + "_backup;",

"DROP TABLE " + PlaylistTable::Name + "_backup;",
