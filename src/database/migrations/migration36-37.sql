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
