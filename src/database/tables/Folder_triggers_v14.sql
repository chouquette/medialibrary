"CREATE INDEX IF NOT EXISTS folder_device_id_idx ON " +
    Folder::Table::Name + " (device_id)",

"CREATE INDEX IF NOT EXISTS parent_folder_id_idx ON " +
    Folder::Table::Name + " (parent_id)",

"CREATE TRIGGER IF NOT EXISTS insert_folder_fts "
    "AFTER INSERT ON " + Folder::Table::Name + " "
"BEGIN "
    "INSERT INTO " + Folder::Table::Name + "Fts(rowid,name) "
        "VALUES(new.id_folder,new.name);"
"END",

"CREATE TRIGGER IF NOT EXISTS delete_folder_fts "
"BEFORE DELETE ON " + Folder::Table::Name + " "
"BEGIN "
    "DELETE FROM " + Folder::Table::Name + "Fts WHERE rowid = old.id_folder;"
"END",

