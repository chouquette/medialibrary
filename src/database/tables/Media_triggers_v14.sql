"CREATE INDEX IF NOT EXISTS index_last_played_date ON "
            + Media::Table::Name + "(last_played_date DESC)",

"CREATE INDEX IF NOT EXISTS index_media_presence ON "
            + Media::Table::Name + "(is_present)",

"CREATE TRIGGER IF NOT EXISTS is_media_device_present AFTER UPDATE OF "
"is_present ON " + Device::Table::Name + " "
"BEGIN "
"UPDATE " + Media::Table::Name + " "
    "SET is_present=new.is_present "
    "WHERE device_id=new.id_device;"
"END;",

"CREATE TRIGGER IF NOT EXISTS cascade_file_deletion AFTER DELETE ON "
+ File::Table::Name +
" BEGIN "
" DELETE FROM " + Media::Table::Name + " WHERE "
    "(SELECT COUNT(id_file) FROM " + File::Table::Name +
        " WHERE media_id=old.media_id) = 0"
        " AND id_media=old.media_id;"
" END;",

"CREATE TRIGGER IF NOT EXISTS insert_media_fts"
" AFTER INSERT ON " + Media::Table::Name +
" BEGIN"
    " INSERT INTO " + Media::Table::Name + "Fts(rowid,title,labels) VALUES(new.id_media, new.title, '');"
" END",

"CREATE TRIGGER IF NOT EXISTS delete_media_fts"
" BEFORE DELETE ON " + Media::Table::Name +
" BEGIN"
    " DELETE FROM " + Media::Table::Name + "Fts WHERE rowid = old.id_media;"
" END",

"CREATE TRIGGER IF NOT EXISTS update_media_title_fts"
" AFTER UPDATE OF title ON " + Media::Table::Name +
" BEGIN"
    " UPDATE " + Media::Table::Name + "Fts SET title = new.title WHERE rowid = new.id_media;"
" END",

"CREATE INDEX IF NOT EXISTS media_types_idx ON " + Media::Table::Name +
    "(type, subtype)",
