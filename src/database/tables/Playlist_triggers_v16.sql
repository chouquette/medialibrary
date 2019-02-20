"CREATE TRIGGER IF NOT EXISTS update_playlist_order AFTER UPDATE OF position"
" ON PlaylistMediaRelation"
" BEGIN "
    "UPDATE PlaylistMediaRelation SET position = position + 1"
    " WHERE playlist_id = new.playlist_id"
    " AND position = new.position"
    // We don't want to trigger a self-update when the insert trigger fires.
    " AND media_id != new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS append_new_playlist_record AFTER INSERT"
" ON PlaylistMediaRelation"
" WHEN new.position IS NULL"
" BEGIN "
    // The record is already inserted when the trigger runs, so the count will always
    // be at least 1, hence the - 1 operation afterward. We want the first item
    // at position 0
    " UPDATE PlaylistMediaRelation SET position = ("
        "SELECT COUNT(media_id) FROM PlaylistMediaRelation WHERE playlist_id = new.playlist_id"
    ") - 1 WHERE playlist_id=new.playlist_id AND media_id = new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS update_playlist_order_on_insert AFTER INSERT"
" ON PlaylistMediaRelation"
" WHEN new.position IS NOT NULL"
" BEGIN "
    "UPDATE PlaylistMediaRelation SET position = position + 1"
    " WHERE playlist_id = new.playlist_id"
    " AND position = new.position"
    " AND media_id != new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS insert_playlist_fts AFTER INSERT ON "
+ Playlist::Table::Name +
" BEGIN"
    " INSERT INTO " + Playlist::Table::Name + "Fts(rowid, name) VALUES(new.id_playlist, new.name);"
" END",

"CREATE TRIGGER IF NOT EXISTS update_playlist_fts AFTER UPDATE OF name"
" ON " + Playlist::Table::Name +
" BEGIN"
    " UPDATE " + Playlist::Table::Name + "Fts SET name = new.name WHERE rowid = new.id_playlist;"
" END",

"CREATE TRIGGER IF NOT EXISTS delete_playlist_fts BEFORE DELETE ON "
+ Playlist::Table::Name +
" BEGIN"
" DELETE FROM " + Playlist::Table::Name + "Fts WHERE rowid = old.id_playlist;"
" END",
