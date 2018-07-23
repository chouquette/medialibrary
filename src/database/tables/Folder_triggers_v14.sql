"CREATE TRIGGER IF NOT EXISTS is_device_present AFTER UPDATE OF is_present ON "
    + Device::Table::Name +
" WHEN old.is_present != new.is_present"
" BEGIN"
    " UPDATE " + Folder::Table::Name + " SET is_present = new.is_present WHERE device_id = new.id_device;"
" END",

"CREATE INDEX IF NOT EXISTS folder_device_id_idx ON " +
    Folder::Table::Name + " (device_id)",

"CREATE INDEX IF NOT EXISTS parent_folder_id_idx ON " +
    Folder::Table::Name + " (parent_id)"
