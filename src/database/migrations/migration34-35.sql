/* Ensure we update is_present before the total number of track */
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::UpdateOnTrackDelete, 34 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 35 ),

/* Fix a broken trigger deleting all genres when nb_tracks reaches 0 */
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::DeleteEmpty, 34 ),
Genre::trigger( Genre::Triggers::DeleteEmpty, 35 ),

/* Add a missing ON DELETE CASCADE on media_id foreign key */
"CREATE TEMPORARY TABLE " + Bookmark::Table::Name + "_backup"
"("
    "id_bookmark INTEGER PRIMARY KEY,"
    "time UNSIGNED INTEGER,"
    "name TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INTEGER,"
    "creation_date UNSIGNED INTEGER NOT NULL,"
    "type UNSIGNED INTEGER NOT NULL"
")",

"INSERT INTO " + Bookmark::Table::Name + "_backup "
    "SELECT * FROM " + Bookmark::Table::Name,

"DROP TABLE " + Bookmark::Table::Name,

Bookmark::schema( Bookmark::Table::Name, 35 ),

"INSERT INTO " + Bookmark::Table::Name +
    " SELECT * FROM " + Bookmark::Table::Name + "_backup",

"DROP TABLE " + Bookmark::Table::Name + "_backup",

Bookmark::index( Bookmark::Indexes::MediaId, 35 ),
