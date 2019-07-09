"CREATE TABLE IF NOT EXISTS " + parser::Task::Table::Name +
"("
    "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
    "step INTEGER NOT NULL DEFAULT 0,"
    "retry_count INTEGER NOT NULL DEFAULT 0,"
    "type INTEGER NOT NULL,"
    "mrl TEXT,"
    "file_type INTEGER NOT NULL,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER,"

    // For linking purposes
    "link_to_id UNSIGNED INTEGER,"
    "link_to_type UNSIGNED INTEGER,"
    "link_extra UNSIGNED INTEGER,"

    "UNIQUE(mrl, type) ON CONFLICT FAIL,"
    "FOREIGN KEY (parent_folder_id) REFERENCES " + Folder::Table::Name
    + "(id_folder) ON DELETE CASCADE,"
    "FOREIGN KEY (file_id) REFERENCES " + File::Table::Name
    + "(id_file) ON DELETE CASCADE"
")",

