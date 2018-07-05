"CREATE TABLE IF NOT EXISTS " + policy::MetadataTable::Name +
"("
    "id_media INTEGER,"
    "entity_type INTEGER,"
    "type INTEGER,"
    "value TEXT,"
    "PRIMARY KEY (id_media, entity_type, type)"
")",
