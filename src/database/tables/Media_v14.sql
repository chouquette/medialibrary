"CREATE TABLE IF NOT EXISTS " + Media::Table::Name + "("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER NOT NULL DEFAULT " +
        std::to_string( static_cast<typename std::underlying_type<IMedia::SubType>::type>(
                            IMedia::SubType::Unknown ) ) + ","
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "real_last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail_id INTEGER,"
    "thumbnail_generated BOOLEAN NOT NULL DEFAULT 0,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT COLLATE NOCASE,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"

    "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
    + "(id_thumbnail)"
")",

"CREATE INDEX IF NOT EXISTS media_types_idx ON " + Media::Table::Name +
    "(type, subtype)",

"CREATE VIRTUAL TABLE IF NOT EXISTS "
    + Media::Table::Name + "Fts USING FTS3("
    "title,"
    "labels"
")",
