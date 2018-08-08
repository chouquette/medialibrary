"CREATE TABLE IF NOT EXISTS " + parser::Task::Table::Name +
"("
    "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
    "step INTEGER NOT NULL DEFAULT 0,"
    "retry_count INTEGER NOT NULL DEFAULT 0,"
    "mrl TEXT,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER,"
    "parent_playlist_id INTEGER,"
    "parent_playlist_index UNSIGNED INTEGER,"
    "is_refresh BOOLEAN NOT NULL DEFAULT 0,"
    "UNIQUE(mrl, parent_playlist_id, is_refresh) ON CONFLICT FAIL,"
    "FOREIGN KEY (parent_folder_id) REFERENCES " + Folder::Table::Name
    + "(id_folder) ON DELETE CASCADE,"
    "FOREIGN KEY (file_id) REFERENCES " + File::Table::Name
    + "(id_file) ON DELETE CASCADE,"
    "FOREIGN KEY (parent_playlist_id) REFERENCES " + Playlist::Table::Name
    + "(id_playlist) ON DELETE CASCADE"
")",
