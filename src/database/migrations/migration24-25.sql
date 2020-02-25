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
    " SELECT id_group, parent_id, name, nb_video, nb_audio, nb_unknown, false"
    " FROM " + MediaGroup::Table::Name + "_backup ",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 25 ),
MediaGroup::index( MediaGroup::Indexes::ParentId, 25 ),
