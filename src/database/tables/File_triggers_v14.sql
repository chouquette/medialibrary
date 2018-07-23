"CREATE TRIGGER IF NOT EXISTS is_folder_present AFTER UPDATE OF is_present ON "
    + Folder::Table::Name +
" BEGIN"
    " UPDATE " + File::Table::Name + " SET is_present = new.is_present WHERE folder_id = new.id_folder;"
" END",

"CREATE INDEX IF NOT EXISTS file_media_id_index ON " +
    File::Table::Name + "(media_id)",

"CREATE INDEX IF NOT EXISTS file_folder_id_index ON " +
    File::Table::Name + "(folder_id)",
