"CREATE TABLE IF NOT EXISTS " + policy::PlaylistTable::Name + "("
    + policy::PlaylistTable::PrimaryKeyColumn + " INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT,"
    "file_id UNSIGNED INT DEFAULT NULL,"
    "creation_date UNSIGNED INT NOT NULL,"
    "artwork_mrl TEXT,"
    "FOREIGN KEY (file_id) REFERENCES " + policy::FileTable::Name
    + "(id_file) ON DELETE CASCADE"
")",

"CREATE TABLE IF NOT EXISTS PlaylistMediaRelation"
"("
    "media_id INTEGER,"
    "mrl STRING NOT NULL,"
    "playlist_id INTEGER,"
    "position INTEGER,"
    "FOREIGN KEY(media_id) REFERENCES " + policy::MediaTable::Name + "("
        + policy::MediaTable::PrimaryKeyColumn + ") ON DELETE SET NULL,"
    "FOREIGN KEY(playlist_id) REFERENCES " + policy::PlaylistTable::Name + "("
        + policy::PlaylistTable::PrimaryKeyColumn + ") ON DELETE CASCADE"
")",

"CREATE INDEX IF NOT EXISTS playlist_media_pl_id_index "
    "ON PlaylistMediaRelation(media_id, playlist_id)",

"CREATE VIRTUAL TABLE IF NOT EXISTS " + policy::PlaylistTable::Name + "Fts USING FTS3"
"("
    "name"
")",
