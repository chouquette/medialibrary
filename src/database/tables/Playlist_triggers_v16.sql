"CREATE INDEX IF NOT EXISTS playlist_position_pl_id_index "
    "ON " + Playlist::MediaRelationTable::Name + "(playlist_id, position)",
