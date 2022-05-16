"CREATE TEMPORARY TABLE " + Label::FileRelationTable::Name + "_backup"
"("
    "label_id INTEGER,"
    "media_id INTEGER,"
    "PRIMARY KEY(label_id,media_id)"
")",

"INSERT INTO " + Label::FileRelationTable::Name + "_backup "
    "SELECT * FROM " + Label::FileRelationTable::Name,

"DROP TABLE " + Label::FileRelationTable::Name,
Label::schema( Label::FileRelationTable::Name, 37 ),

"INSERT INTO " + Label::FileRelationTable::Name +
    " SELECT label_id, media_id, " + utils::enum_to_string( Label::EntityType::Media ) +
    " FROM " + Label::FileRelationTable::Name + "_backup",

"DROP TABLE " + Label::FileRelationTable::Name + "_backup",

Label::trigger( Label::Triggers::DeleteMediaLabel, 37 ),

"CREATE TEMPORARY TABLE " + Folder::Table::Name + "_backup"
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

Folder::schema( Folder::Table::Name, 37 ),

"INSERT INTO " + Folder::Table::Name +
    " SELECT *, (SELECT TOTAL(IIF(duration > 0, duration, 0)) FROM "
        + Media::Table::Name + " WHERE folder_id = id_folder), FALSE"
    " FROM " + Folder::Table::Name + "_backup",


"DROP TABLE " + Folder::Table::Name + "_backup",

/* Triggers and indexes deleted by the table deletion */
Folder::trigger( Folder::Triggers::InsertFts, 37 ),
Folder::trigger( Folder::Triggers::DeleteFts, 37 ),
Folder::index( Folder::Indexes::DeviceId, 37 ),
Folder::index( Folder::Indexes::ParentId, 37 ),

/* Updated folder triggers */
"DROP TRIGGER " + Folder::triggerName( Folder::Triggers::UpdateNbMediaOnUpdate, 36 ),
"DROP TRIGGER " + Folder::triggerName( Folder::Triggers::UpdateNbMediaOnDelete, 36 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 37 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnDelete, 37 ),
