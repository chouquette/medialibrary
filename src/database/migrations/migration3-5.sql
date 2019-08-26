"CREATE TEMPORARY TABLE " + File::Table::Name + "_backup("
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
  "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + File::Table::Name + "_backup SELECT * FROM " + File::Table::Name + ";",

"DROP TABLE " + File::Table::Name + ";",

"CREATE TABLE " + File::Table::Name + "(id_file INTEGER PRIMARY KEY AUTOINCREMENT,"
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
  "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "(id_media) ON DELETE CASCADE,"
  "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name + "(id_playlist) ON DELETE CASCADE," // Added
  "FOREIGN KEY(folder_id) REFERENCES " + Folder::Table::Name + "(id_folder) ON DELETE CASCADE,"
  "UNIQUE( mrl, folder_id ) ON CONFLICT FAIL);",

"INSERT INTO " + File::Table::Name + "(id_file,media_id,mrl,type,last_modification_date,size,parser_step,parser_retries,folder_id,is_present,is_removable,is_external)"
 " SELECT * FROM " + File::Table::Name + "_backup;",

"DROP TABLE " + File::Table::Name + "_backup;",

"CREATE TEMPORARY TABLE " + Playlist::Table::Name + "_backup("
  "id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,"
  "name TEXT UNIQUE,"
  "creation_date UNSIGNED INT NOT NULL);",

"INSERT INTO " + Playlist::Table::Name + "_backup SELECT * FROM Playlist;",

"DROP TABLE " + Playlist::Table::Name + ";",

"CREATE TABLE " + Playlist::Table::Name + "("
  "id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,"
  "name TEXT UNIQUE,"
  "file_id UNSIGNED INT DEFAULT NULL," // Added
  "creation_date UNSIGNED INT NOT NULL,"
  "artwork_mrl TEXT," // Added
  "FOREIGN KEY(file_id) REFERENCES " + File::Table::Name + "(id_file) ON DELETE CASCADE);", // Added

"INSERT INTO " + Playlist::Table::Name + "(id_playlist,name,creation_date)"
 " SELECT * FROM " + Playlist::Table::Name + "_backup;",

"DROP TABLE " + Playlist::Table::Name + "_backup;",

Folder::schema( Folder::ExcludedFolderTable::Name, 5 ),
