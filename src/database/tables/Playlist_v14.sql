"CREATE TABLE IF NOT EXISTS " + Playlist::Table::Name + "("
    + Playlist::Table::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT,"
    "FOREIGN KEY (file_id) REFERENCES " + File::Table::Name
    + "(id_file) ON DELETE CASCADE"
")",

"CREATE TABLE IF NOT EXISTS " + Playlist::MediaRelationTable::Name +
"("
    "media_id INTEGER,"
    "mrl STRING,"
    "playlist_id INTEGER,"
    "position INTEGER,"
    "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name + "("
        + Media::Table::PrimaryKeyColumn + ") ON DELETE SET NULL,"
    "FOREIGN KEY(playlist_id) REFERENCES " + Playlist::Table::Name + "("
        + Playlist::Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
")",

"CREATE VIRTUAL TABLE IF NOT EXISTS " + Playlist::FtsTable::Name + " USING FTS3"
"("
    "name"
")",
