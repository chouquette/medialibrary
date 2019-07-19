"CREATE TRIGGER IF NOT EXISTS auto_delete_thumbnails_after_delete"
" AFTER DELETE ON " + Thumbnail::LinkingTable::Name +
" BEGIN "
" DELETE FROM " + Thumbnail::Table::Name +
" WHERE id_thumbnail = old.thumbnail_id"
" AND (SELECT COUNT(*) FROM " + Thumbnail::LinkingTable::Name +
    " WHERE thumbnail_id = old.thumbnail_id) = 0;"
"END;",

"CREATE TRIGGER IF NOT EXISTS auto_delete_thumbnails_after_update"
" AFTER UPDATE OF thumbnail_id ON " + Thumbnail::LinkingTable::Name +
" BEGIN "
" DELETE FROM " + Thumbnail::Table::Name +
" WHERE id_thumbnail = old.thumbnail_id"
" AND (SELECT COUNT(*) FROM " + Thumbnail::LinkingTable::Name +
    " WHERE thumbnail_id = old.thumbnail_id) = 0;"
"END;",

"CREATE TRIGGER IF NOT EXISTS auto_delete_album_thumbnail"
" AFTER DELETE ON " + Album::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_album AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Album ) ) + ";"
" END;",

"CREATE TRIGGER IF NOT EXISTS auto_delete_artist_thumbnail"
" AFTER DELETE ON " + Artist::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_artist AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Artist ) ) + ";"
" END;",

"CREATE TRIGGER IF NOT EXISTS auto_delete_media_thumbnail"
" AFTER DELETE ON " + Media::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_media AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Media ) ) + ";"
" END;",
