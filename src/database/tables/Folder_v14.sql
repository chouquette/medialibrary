"CREATE TABLE IF NOT EXISTS " + policy::FolderTable::Name +
"("
    "id_folder INTEGER PRIMARY KEY AUTOINCREMENT,"
    "path TEXT,"
    "parent_id UNSIGNED INTEGER,"
    "is_blacklisted BOOLEAN NOT NULL DEFAULT 0,"
    "device_id UNSIGNED INTEGER,"
    "is_present BOOLEAN NOT NULL DEFAULT 1,"
    "is_removable BOOLEAN NOT NULL,"

    "FOREIGN KEY (parent_id) REFERENCES " + policy::FolderTable::Name +
    "(id_folder) ON DELETE CASCADE,"

    "FOREIGN KEY (device_id) REFERENCES " + policy::DeviceTable::Name +
    "(id_device) ON DELETE CASCADE,"

    "UNIQUE(path, device_id) ON CONFLICT FAIL"
")",

"CREATE TABLE IF NOT EXISTS ExcludedEntryFolder"
"("
    "folder_id UNSIGNED INTEGER NOT NULL,"

    "FOREIGN KEY (folder_id) REFERENCES " + policy::FolderTable::Name +
    "(id_folder) ON DELETE CASCADE,"

    "UNIQUE(folder_id) ON CONFLICT FAIL"
")"
