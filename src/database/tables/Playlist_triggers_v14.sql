"CREATE TRIGGER IF NOT EXISTS update_playlist_order AFTER UPDATE OF position"
" ON " + Playlist::MediaRelationTable::Name +
" BEGIN "
    "UPDATE " + Playlist::MediaRelationTable::Name +
    " SET position = position + 1"
    " WHERE playlist_id = new.playlist_id"
    " AND position = new.position"
    // We don't want to trigger a self-update when the insert trigger fires.
    " AND media_id != new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS append_new_playlist_record AFTER INSERT"
" ON " + Playlist::MediaRelationTable::Name +
" WHEN new.position IS NULL"
" BEGIN "
    " UPDATE " + Playlist::MediaRelationTable::Name + " SET position = ("
        "SELECT COUNT(media_id) FROM " + Playlist::MediaRelationTable::Name +
        " WHERE playlist_id = new.playlist_id"
    ") WHERE playlist_id=new.playlist_id AND media_id = new.media_id;"
" END",

"CREATE TRIGGER IF NOT EXISTS update_playlist_order_on_insert AFTER INSERT"
" ON " + Playlist::MediaRelationTable::Name +
" WHEN new.position IS NOT NULL"
" BEGIN "
    "UPDATE " + Playlist::MediaRelationTable::Name +
    " SET position = position + 1"
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
