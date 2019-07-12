"CREATE TABLE IF NOT EXISTS " + Bookmark::Table::Name +
"("
    "id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,"
    "time UNSIGNED INTEGER NOT NULL,"
    "name TEXT,"
    "description TEXT,"
    "media_id UNSIGNED INTEGER NOT NULL,"

    "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name
    + "(id_media),"
    "UNIQUE (time, media_id) ON CONFLICT FAIL"
")",
