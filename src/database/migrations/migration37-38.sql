Subscription::schema( Subscription::FtsTable::Name, 38 ),

Subscription::trigger( Subscription::Triggers::InsertFts, 38 ),
Subscription::trigger( Subscription::Triggers::UpdateFts, 38 ),
Subscription::trigger( Subscription::Triggers::DeleteFts, 38 ),

"ALTER TABLE " + Subscription::Table::Name + " ADD COLUMN artwork_mrl TEXT",

"ALTER TABLE " + Playlist::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",
"ALTER TABLE " + MediaGroup::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",
"ALTER TABLE " + Folder::Table::Name + " ADD COLUMN is_favorite BOOLEAN NOT NULL DEFAULT FALSE",
