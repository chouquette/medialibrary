"CREATE TABLE IF NOT EXISTS " + Album::Table::Name +
"("
    "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
    "title TEXT COLLATE NOCASE,"
    "artist_id UNSIGNED INTEGER,"
    "release_year UNSIGNED INTEGER,"
    "short_summary TEXT,"
    // Number of tracks in this album, regardless of their presence
    // state.
    "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
    "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,"
    // The album presence state, which is the number of present tracks
    // in this album
    "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
        "CHECK(is_present <= nb_tracks),"
    "FOREIGN KEY( artist_id ) REFERENCES " + Artist::Table::Name
    + "(id_artist) ON DELETE CASCADE"
")",

"CREATE VIRTUAL TABLE IF NOT EXISTS " + Album::FtsTable::Name + " USING FTS3("
    "title,"
    "artist"
")",
