"CREATE TRIGGER IF NOT EXISTS auto_delete_thumbnails_after_delete"
" AFTER DELETE ON " + Thumbnail::LinkingTable::Name +
" BEGIN "
" DELETE FROM " + Thumbnail::Table::Name +
" WHERE id_thumbnail = old.thumbnail_id"
" AND (SELECT COUNT(*) FROM " + Thumbnail::LinkingTable::Name +
    " WHERE thumbnail_id = old.thumbnail_id) = 0;"
"END;",

