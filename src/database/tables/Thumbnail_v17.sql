"CREATE TABLE IF NOT EXISTS " + Thumbnail::Table::Name +
"("
    "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
    "mrl TEXT,"
    "is_generated BOOLEAN NOT NULL"
")",

"CREATE TABLE IF NOT EXISTS " + Thumbnail::LinkingTable::Name +
"("
    "entity_id UNSIGNED INTEGER NOT NULL,"
    "entity_type UNSIGNED INTEGER NOT NULL,"
    "size_type UNSIGNED INTEGER NOT NULL,"
    "thumbnail_id UNSIGNED INTEGER NOT NULL,"
    "origin UNSIGNED INT NOT NULL,"

    "PRIMARY KEY(entity_id, entity_type, size_type),"
    "FOREIGN KEY(thumbnail_id) REFERENCES " +
        Thumbnail::Table::Name + "(id_thumbnail) ON DELETE CASCADE"
")",

"CREATE INDEX IF NOT EXISTS thumbnail_link_index "
"ON " + Thumbnail::Table::Name + "(id_thumbnail)",
