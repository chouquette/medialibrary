"CREATE TRIGGER IF NOT EXISTS is_folder_present AFTER UPDATE OF is_present ON "
    + policy::FolderTable::Name +
" BEGIN"
    " UPDATE " + policy::FileTable::Name + " SET is_present = new.is_present WHERE folder_id = new.id_folder;"
" END",

"CREATE INDEX IF NOT EXISTS file_media_id_index ON " +
    policy::FileTable::Name + "(media_id)",

"CREATE INDEX IF NOT EXISTS file_folder_id_index ON " +
    policy::FileTable::Name + "(folder_id)",
