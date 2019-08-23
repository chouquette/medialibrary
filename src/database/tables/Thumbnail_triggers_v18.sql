// Delete linking record associated with an album when the album gets deleted
"CREATE TRIGGER IF NOT EXISTS auto_delete_album_thumbnail"
" AFTER DELETE ON " + Album::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_album AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Album ) ) + ";"
" END",

// Delete linking record associated with an artist when the artist gets deleted
"CREATE TRIGGER IF NOT EXISTS auto_delete_artist_thumbnail"
" AFTER DELETE ON " + Artist::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_artist AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Artist ) ) + ";"
" END",

// Delete linking record associated with a media when the media gets deleted
"CREATE TRIGGER IF NOT EXISTS auto_delete_media_thumbnail"
" AFTER DELETE ON " + Media::Table::Name +
" BEGIN"
    " DELETE FROM " + Thumbnail::LinkingTable::Name + " WHERE"
        " entity_id = old.id_media AND"
        " entity_type = " + std::to_string(
            static_cast<std::underlying_type_t<Thumbnail::EntityType>>(
                Thumbnail::EntityType::Media ) ) + ";"
" END",

/* Update the thumbnail refcount based on the insertion/deletion/updates of the linking table */

"CREATE TRIGGER IF NOT EXISTS incr_thumbnail_refcount "
"AFTER INSERT ON " + Thumbnail::LinkingTable::Name + " "
"BEGIN "
    "UPDATE " + Thumbnail::Table::Name + " "
        "SET shared_counter = shared_counter + 1 "
        "WHERE id_thumbnail = new.thumbnail_id;"
"END",

"CREATE TRIGGER IF NOT EXISTS decr_thumbnail_refcount "
"AFTER DELETE ON " + Thumbnail::LinkingTable::Name + " "
"BEGIN "
    "UPDATE " + Thumbnail::Table::Name + " "
        "SET shared_counter = shared_counter - 1 "
        "WHERE id_thumbnail = old.thumbnail_id;"
"END",

"CREATE TRIGGER IF NOT EXISTS update_thumbnail_refcount "
"AFTER UPDATE OF thumbnail_id ON " + Thumbnail::LinkingTable::Name + " "
"WHEN old.thumbnail_id != new.thumbnail_id "
"BEGIN "
    "UPDATE " + Thumbnail::Table::Name +
        " SET shared_counter = shared_counter - 1 WHERE id_thumbnail = old.thumbnail_id;"
    "UPDATE " + Thumbnail::Table::Name +
        " SET shared_counter = shared_counter + 1 WHERE id_thumbnail = new.thumbnail_id;"
"END",

/* Delete thumbnail with a refcount of 0 */
"CREATE TRIGGER IF NOT EXISTS delete_unused_thumbnail "
"AFTER UPDATE OF shared_counter ON " + Thumbnail::Table::Name + " "
"WHEN new.shared_counter = 0 "
"BEGIN "
    "DELETE FROM " + Thumbnail::Table::Name + " WHERE id_thumbnail = new.id_thumbnail;"
"END",

"CREATE INDEX IF NOT EXISTS thumbnail_link_index "
"ON " + Thumbnail::Table::Name + "(id_thumbnail)",
