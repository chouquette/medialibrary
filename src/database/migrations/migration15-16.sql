/**
 * If migrating from an older version, this trigger might already be removed
 * due to a migration on the playlist table, and updated Playlist_trigger file.
 */
"DROP TRIGGER IF EXISTS update_playlist_order",
/* This trigger however was always recreated until now, and must be present */
"DROP TRIGGER update_playlist_order_on_insert",

#include "database/tables/Playlist_triggers_v16.sql"
