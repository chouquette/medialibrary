"CREATE INDEX IF NOT EXISTS folder_device_id_idx ON " +
    Folder::Table::Name + " (device_id)",

"CREATE INDEX IF NOT EXISTS parent_folder_id_idx ON " +
    Folder::Table::Name + " (parent_id)"
