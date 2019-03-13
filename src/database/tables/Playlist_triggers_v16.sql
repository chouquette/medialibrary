"CREATE TRIGGER IF NOT EXISTS update_playlist_order_on_insert AFTER INSERT"
" ON PlaylistMediaRelation"
" WHEN new.position IS NOT NULL"
" BEGIN "
    "UPDATE PlaylistMediaRelation SET position = position + 1"
    " WHERE playlist_id = new.playlist_id"
    " AND position >= new.position"
    " AND rowid != new.rowid;"
" END",

"CREATE TRIGGER IF NOT EXISTS update_playlist_order_on_delete AFTER DELETE"
" ON PlaylistMediaRelation"
" BEGIN "
    "UPDATE PlaylistMediaRelation SET position = position - 1"
    " WHERE playlist_id = old.playlist_id"
    " AND position > old.position;"
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
