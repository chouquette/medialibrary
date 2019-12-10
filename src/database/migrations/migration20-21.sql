/* VideoGroup support was replaced by media groups starting from version 24
 * Don't bother create it, especially since the class & schema are removed, so
 * we can't use it from here.
 * If the migration is ran using this code, it means we are migrating to version
 * 24 or later
 */
/* VideoGroup::schema( VideoGroup::Table::Name, 21 ), */

"ALTER TABLE Settings ADD COLUMN video_groups_prefix_length UNSIGNED INTEGER NOT NULL DEFAULT 6",
