"CREATE TEMPORARY TABLE " + ArtistTable::Name + "_backup("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + ArtistTable::Name + "_backup SELECT * FROM " + ArtistTable::Name + ";",

"DROP TABLE " + ArtistTable::Name + ";",

"CREATE TABLE " + ArtistTable::Name + "("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "artwork_mrl TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "nb_tracks UNSIGNED INT DEFAULT 0," // Added
    "mb_id TEXT,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + ArtistTable::Name + "("
    "id_artist,"
    "name,"
    "shortbio,"
    "artwork_mrl,"
    "nb_albums,"
    "mb_id,"
    "is_present"
")"
" SELECT * FROM " + ArtistTable::Name + "_backup;",

"DROP TABLE " + ArtistTable::Name + "_backup;",

"UPDATE " + ArtistTable::Name + " SET nb_tracks = ("
    "SELECT COUNT(id_track) FROM " + AlbumTrackTable::Name + " WHERE "
    "artist_id = " + ArtistTable::Name + ".id_artist"
")"
