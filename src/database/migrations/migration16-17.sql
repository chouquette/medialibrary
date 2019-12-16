/******************************************************************
 * Remove media.thumbnail_id field                                *
 *****************************************************************/

"CREATE TEMPORARY TABLE " + Media::Table::Name + "_backup"
"("
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
    "title TEXT COLLATE NOCASE,"
    "filename TEXT COLLATE NOCASE,"
    "is_favorite BOOLEAN NOT NULL DEFAULT 0,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "device_id INTEGER,"
    "nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "folder_id UNSIGNED INTEGER"
")",

"INSERT INTO " + Media::Table::Name + "_backup SELECT "
    "id_media, type, subtype, duration, play_count, last_played_date,"
    "real_last_played_date, insertion_date, release_date, title, filename,"
    "is_favorite, is_present, device_id, nb_playlists, folder_id "
"FROM " + Media::Table::Name,

"DROP TABLE " + Media::Table::Name,

Media::schema( Media::Table::Name, 17 ),

"INSERT INTO " + Media::Table::Name + " SELECT * FROM " + Media::Table::Name + "_backup",
"DROP TABLE " + Media::Table::Name + "_backup",

/******************************************************************
 * Remove album.thumbnail_id field                                *
 *****************************************************************/

"CREATE TEMPORARY TABLE " + Album::Table::Name + "_backup"
"("
    "id_album INTEGER PRIMARY KEY AUTOINCREMENT,"
    "title TEXT COLLATE NOCASE,"
    "artist_id UNSIGNED INTEGER,"
    "release_year UNSIGNED INTEGER,"
    "short_summary TEXT,"
    "nb_tracks UNSIGNED INTEGER DEFAULT 0,"
    "duration UNSIGNED INTEGER NOT NULL DEFAULT 0,"
    "nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,"
    "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0"
")",

"INSERT INTO " + Album::Table::Name + "_backup SELECT "
    "id_album, title, artist_id, release_year, short_summary, nb_tracks,"
    "duration, nb_discs, is_present "
"FROM " + Album::Table::Name,

"DROP TABLE " + Album::Table::Name,

Album::schema( Album::Table::Name, 17 ),

"INSERT INTO " + Album::Table::Name + " SELECT * FROM " + Album::Table::Name + "_backup",

"DROP TABLE " + Album::Table::Name + "_backup",

/******************************************************************
 * Remove artist.thumbnail_id field                               *
 *****************************************************************/

"CREATE TEMPORARY TABLE IF NOT EXISTS " + Artist::Table::Name + "_backup"
"("
    "id_artist INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "shortbio TEXT,"
    "nb_albums UNSIGNED INT DEFAULT 0,"
    "nb_tracks UNSIGNED INT DEFAULT 0,"
    "mb_id TEXT,"
    "is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 "
")",

"INSERT INTO " + Artist::Table::Name + "_backup SELECT "
    "id_artist, name, shortbio, nb_albums, nb_tracks, mb_id, is_present "
"FROM " + Artist::Table::Name,

"DROP TABLE " + Artist::Table::Name,

Artist::schema( Artist::Table::Name, 17 ),

"INSERT INTO " + Artist::Table::Name + " SELECT * FROM " + Artist::Table::Name + "_backup",

"DROP TABLE " + Artist::Table::Name + "_backup",

/******************************************************************
 * Remove Thumbnail.origin field                                  *
 * Since this migration is asking for a thumbnail re-scan, don't  *
 * bother migrating the content, it would be erased anyway.       *
 *****************************************************************/

"DROP TABLE " + Thumbnail::Table::Name,

Thumbnail::schema( Thumbnail::Table::Name, 17 ),
Thumbnail::schema( Thumbnail::LinkingTable::Name, 17 ),

Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteAlbum, 17 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteArtist, 17 ),
Thumbnail::trigger( Thumbnail::Triggers::AutoDeleteMedia, 17 ),
Thumbnail::trigger( Thumbnail::Triggers::DeleteUnused, 17 ),
Thumbnail::trigger( Thumbnail::Triggers::DeleteAfterLinkingDelete, 17 ),
Thumbnail::index( Thumbnail::Indexes::ThumbnailId, 17 ),

/**********************************************
 * Remove an old incorrectly migrated trigger *
 **********************************************/

// This trigger was incorrectly recreated during the 13 -> 14 migration, so
// ensure it's the correct one now.
// Artists triggers are all recreated as part of the 16 -> 17 migration
"DROP TRIGGER has_track_remaining",

Artist::trigger( Artist::Triggers::HasTrackPresent, 17 ),
Artist::trigger( Artist::Triggers::HasAlbumRemaining, 17 ),
Artist::trigger( Artist::Triggers::DeleteArtistsWithoutTracks, 17 ),
Artist::trigger( Artist::Triggers::InsertFts, 17 ),
Artist::trigger( Artist::Triggers::DeleteFts, 17 ),


