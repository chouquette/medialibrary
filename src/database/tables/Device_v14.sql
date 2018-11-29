"CREATE TABLE IF NOT EXISTS " + Device::Table::Name +
"("
    "id_device INTEGER PRIMARY KEY AUTOINCREMENT,"
    "uuid TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,"
    "scheme TEXT,"
    "is_removable BOOLEAN,"
    "is_present BOOLEAN,"
    "last_seen UNSIGNED INTEGER"
")",
