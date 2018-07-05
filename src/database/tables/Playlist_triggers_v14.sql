"CREATE TRIGGER IF NOT EXISTS update_playlist_order AFTER UPDATE OF position"
" ON PlaylistMediaRelation"
" BEGIN "
    "UPDATE PlaylistMediaRelation SET position = position + 1"
    " WHERE playlist_id = new.playlist_id"
    " AND position = new.position"
    // We don't to trigger a self-update when the insert trigger fires.
    " AND media_id != new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS append_new_playlist_record AFTER INSERT"
" ON PlaylistMediaRelation"
" WHEN new.position IS NULL"
" BEGIN "
    " UPDATE PlaylistMediaRelation SET position = ("
        "SELECT COUNT(media_id) FROM PlaylistMediaRelation WHERE playlist_id = new.playlist_id"
    ") WHERE playlist_id=new.playlist_id AND media_id = new.media_id;"
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
+ policy::PlaylistTable::Name +
" BEGIN"
    " INSERT INTO " + policy::PlaylistTable::Name + "Fts(rowid, name) VALUES(new.id_playlist, new.name);"
" END",

"CREATE TRIGGER IF NOT EXISTS update_playlist_fts AFTER UPDATE OF name"
" ON " + policy::PlaylistTable::Name +
" BEGIN"
    " UPDATE " + policy::PlaylistTable::Name + "Fts SET name = new.name WHERE rowid = new.id_playlist;"
" END",

"CREATE TRIGGER IF NOT EXISTS delete_playlist_fts BEFORE DELETE ON "
+ policy::PlaylistTable::Name +
" BEGIN"
" DELETE FROM " + policy::PlaylistTable::Name + "Fts WHERE rowid = old.id_playlist;"
" END",
