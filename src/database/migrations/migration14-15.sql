/******************* Migrate Folder table *************************************/

"CREATE TEMPORARY TABLE " + Folder::Table::Name + "_backup"
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "name TEXT,"
    "parent_id UNSIGNED INTEGER,"
    "is_banned BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_removable BOOLEAN NOT NULL,"
    "nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Folder::Table::Name + "_backup SELECT * FROM " + Folder::Table::Name,

"DROP TABLE " + Folder::Table::Name,

Folder::schema( Folder::Table::Name, 15 ),

"INSERT INTO " + Folder::Table::Name + "("
    "id_folder, path, name, parent_id, is_banned, device_id, is_removable,"
    "nb_audio, nb_video"
") "
"SELECT id_folder, path, name, parent_id, is_banned, device_id, is_removable,"
    "nb_audio, nb_video "
"FROM " + Folder::Table::Name + "_backup",

"DROP TABLE " + Folder::Table::Name + "_backup",
