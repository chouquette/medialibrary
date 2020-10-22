/*** Migrate thumbnail table ***/
"DROP TABLE " + Thumbnail::Table::Name,
Thumbnail::schema( Thumbnail::Table::Name, 28 ),
/* Ensure we also flush the linking table since the foreign key
 * ON DELETE CASCADE won't be processed during a DROP TABLE */
"DELETE FROM " + Thumbnail::LinkingTable::Name,

Thumbnail::trigger( Thumbnail::Triggers::DeleteUnused, 23 ),
