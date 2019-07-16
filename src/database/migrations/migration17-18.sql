"DROP TRIGGER auto_delete_thumbnails_after_delete",
"DROP TRIGGER auto_delete_thumbnails_after_update",

"DROP TABLE " + Thumbnail::Table::Name,

#include "database/tables/Thumbnail_v18.sql"

#include "database/tables/Thumbnail_triggers_v18.sql"
