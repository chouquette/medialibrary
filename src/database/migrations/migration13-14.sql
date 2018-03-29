/******************* Migrate Media table **************************************/
"CREATE TEMPORARY TABLE " + MediaTable::Name + "_backup("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER,"
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail TEXT,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1"
")",

"INSERT INTO " + MediaTable::Name + "_backup SELECT * FROM " + MediaTable::Name,

"INSERT INTO " + ThumbnailTable::Name + "(id_thumbnail, mrl, origin) "
    "SELECT id_media, thumbnail, " +
    std::to_string( static_cast<ThumbnailType>( Thumbnail::Origin::UserProvided ) ) +
    " FROM " + MediaTable::Name + " WHERE thumbnail IS NOT NULL AND thumbnail != ''",

"DROP TABLE " + MediaTable::Name,

"CREATE TABLE " + MediaTable::Name + "("
    "id_media INTEGER PRIMARY KEY AUTOINCREMENT,"
    "type INTEGER,"
    "subtype INTEGER,"
    "duration INTEGER DEFAULT -1,"
    "play_count UNSIGNED INTEGER,"
    "last_played_date UNSIGNED INTEGER,"
    "insertion_date UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "thumbnail_id INTEGER,"
    "thumbnail_generated BOOLEAN NOT NULL DEFAULT 0,"
    "title TEXT COLLATE NOCASE,"
    "filename TEXT,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "FOREIGN KEY(thumbnail_id) REFERENCES " + ThumbnailTable::Name
    + "(id_thumbnail)"
")",

"INSERT INTO " + MediaTable::Name + "("
    "id_media, type, subtype, duration, play_count, last_played_date, insertion_date,"
    "release_date, thumbnail_id, thumbnail_generated, title, filename, is_favorite,"
    "is_present) "
"SELECT id_media, type, subtype, duration, play_count, last_played_date,"
    "insertion_date, release_date, "
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE id_media END,"
    "CASE thumbnail WHEN NULL THEN 0 WHEN '' THEN 0 ELSE 1 END,"
    "title, filename, is_favorite, is_present FROM " + MediaTable::Name + "_backup",

/******************* Delete other tables **************************************/

"DROP TABLE " + AlbumTable::Name,
"DELETE FROM " + AlbumTable::Name + "Fts",
"DROP TABLE " + ArtistTable::Name,
"DELETE FROM " + ArtistTable::Name + "Fts",
"DELETE FROM MediaArtistRelation",
