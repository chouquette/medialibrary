"CREATE TEMPORARY TABLE " + MediaGroup::Table::Name + "_backup"
"("
   "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
   "parent_id INTEGER,"
   "name TEXT COLLATE NOCASE,"
   "nb_video UNSIGNED INTEGER DEFAULT 0,"
   "nb_audio UNSIGNED INTEGER DEFAULT 0,"
   "nb_unknown UNSIGNED INTEGER DEFAULT 0"
")",

"INSERT INTO " + MediaGroup::Table::Name + "_backup "
    "SELECT * FROM " + MediaGroup::Table::Name,

"DROP TABLE " + MediaGroup::Table::Name,
MediaGroup::schema( MediaGroup::Table::Name, 25 ),

"INSERT INTO " + MediaGroup::Table::Name +
    " SELECT id_group, name, nb_video, nb_audio, nb_unknown, false"
    " FROM " + MediaGroup::Table::Name + "_backup ",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 25 ),

"CREATE TEMPORARY TABLE " + parser::Task::Table::Name + "_backup"
"("
    "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
    "step INTEGER NOT NULL DEFAULT 0,"
    "retry_count INTEGER NOT NULL DEFAULT 0,"
    "type INTEGER NOT NULL,"
    "mrl TEXT,"
    "file_type INTEGER NOT NULL,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER,"
    "link_to_id UNSIGNED INTEGER NOT NULL,"
    "link_to_type UNSIGNED INTEGER NOT NULL,"
    "link_extra UNSIGNED INTEGER NOT NULL"
")",

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT * FROM " + parser::Task::Table::Name,

"DROP TABLE " + parser::Task::Table::Name,

parser::Task::schema( parser::Task::Table::Name, 25 ),

"INSERT INTO " + parser::Task::Table::Name +
    " SELECT *, '' FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " + parser::Task::Table::Name + "_backup",

parser::Task::index( parser::Task::Indexes::ParentFolderId, 25 ),
