"CREATE TABLE IF NOT EXISTS " + Artist::Table::Name +
"("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "thumbnail_id TEXT,"
    // Contains the number of albums where this artist is the album
    // artist. Some or all albums might not be present.
    "nb_albums UNSIGNED INT DEFAULT 0,"
    // Contains the number of tracks by this artist, regardless of
    // the album they appear on. Some or all tracks might not be present
    "nb_tracks UNSIGNED INT DEFAULT 0,"
    "mb_id TEXT,"
    // A presence flag, that represents the number of present tracks.
    // This is not linked to the album presence at all.
    // An artist of which all tracks are not present is considered
    // not present, even if one of its album contains a present
    // track from another artist (the album will be present, however)
    "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
        "CHECK (is_present <= nb_tracks),"
    "FOREIGN KEY(thumbnail_id) REFERENCES " + Thumbnail::Table::Name
    + "(id_thumbnail)"
")",

"CREATE TABLE IF NOT EXISTS " + Artist::MediaRelationTable::Name +
"("
    "media_id INTEGER NOT NULL,"
    "artist_id INTEGER,"
    "PRIMARY KEY (media_id, artist_id),"
    "FOREIGN KEY(media_id) REFERENCES " + Media::Table::Name +
    "(id_media) ON DELETE CASCADE,"
    "FOREIGN KEY(artist_id) REFERENCES " + Artist::Table::Name + "("
        + Artist::Table::PrimaryKeyColumn + ") ON DELETE CASCADE"
")",

"CREATE VIRTUAL TABLE IF NOT EXISTS " + Artist::Table::Name + "Fts USING FTS3"
"("
    "name"
")",
