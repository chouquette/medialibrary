"CREATE INDEX IF NOT EXISTS index_last_played_date ON "
            + policy::MediaTable::Name + "(last_played_date DESC)",

"CREATE TRIGGER IF NOT EXISTS has_files_present AFTER UPDATE OF "
"is_present ON " + policy::FileTable::Name + " "
"BEGIN "
"UPDATE " + policy::MediaTable::Name + " SET is_present="
    "(SELECT EXISTS("
        "SELECT id_file FROM " + policy::FileTable::Name +
        " WHERE media_id=new.media_id AND is_present != 0 LIMIT 1"
    ") )"
    "WHERE id_media=new.media_id;"
"END;",

"CREATE TRIGGER IF NOT EXISTS cascade_file_deletion AFTER DELETE ON "
+ policy::FileTable::Name +
" BEGIN "
" DELETE FROM " + policy::MediaTable::Name + " WHERE "
    "(SELECT COUNT(id_file) FROM " + policy::FileTable::Name +
        " WHERE media_id=old.media_id) = 0"
        " AND id_media=old.media_id;"
" END;",

"CREATE TRIGGER IF NOT EXISTS insert_media_fts"
" AFTER INSERT ON " + policy::MediaTable::Name +
" BEGIN"
    " INSERT INTO " + policy::MediaTable::Name + "Fts(rowid,title,labels) VALUES(new.id_media, new.title, '');"
" END",

"CREATE TRIGGER IF NOT EXISTS delete_media_fts"
" BEFORE DELETE ON " + policy::MediaTable::Name +
" BEGIN"
    " DELETE FROM " + policy::MediaTable::Name + "Fts WHERE rowid = old.id_media;"
" END",

"CREATE TRIGGER IF NOT EXISTS update_media_title_fts"
" AFTER UPDATE OF title ON " + policy::MediaTable::Name +
" BEGIN"
    " UPDATE " + policy::MediaTable::Name + "Fts SET title = new.title WHERE rowid = new.id_media;"
" END",
