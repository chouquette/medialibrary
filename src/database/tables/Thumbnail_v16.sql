"CREATE TABLE IF NOT EXISTS " + Thumbnail::Table::Name +
"("
    "id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,"
    "mrl TEXT,"
    "origin INTEGER NOT NULL,"
    "is_generated BOOLEAN NOT NULL"
")"
