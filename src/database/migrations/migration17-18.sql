/******************************************************************
 * Add thumbnail shared_counter                                   *
 *****************************************************************/
"DROP TRIGGER auto_delete_thumbnails_after_delete",
"DROP TRIGGER auto_delete_thumbnails_after_update",

"DROP TABLE " + Thumbnail::Table::Name,
"DROP TABLE " + Thumbnail::LinkingTable::Name,

Thumbnail::schema( Thumbnail::Table::Name, 18 ),
Thumbnail::schema( Thumbnail::LinkingTable::Name, 18 ),

#include "database/tables/Thumbnail_triggers_v18.sql"

/******************************************************************
 * Split Task into Task & Item                                    *
 *****************************************************************/

"CREATE TEMPORARY TABLE " + parser::Task::Table::Name + "_backup"
"("
    "id_task INTEGER PRIMARY KEY AUTOINCREMENT,"
    "mrl TEXT,"
    "file_type INTEGER NOT NULL,"
    "file_id UNSIGNED INTEGER,"
    "parent_folder_id UNSIGNED INTEGER"
")",

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT "
        "id_task, mrl, file_type, file_id, parent_folder_id"
    " FROM " + parser::Task::Table::Name +
        " WHERE parent_playlist_id IS NULL",

"DROP TABLE " + parser::Task::Table::Name,

#include "database/tables/Task_v18.sql"
#include "database/tables/Task_triggers_v18.sql"

"INSERT INTO " + parser::Task::Table::Name +
    "(id_task, type, mrl, file_type, file_id, parent_folder_id)"
"SELECT "
    "id_task, " +
    std::to_string( static_cast<std::underlying_type_t<parser::Task::Type>>(
        parser::Task::Type::Creation ) ) +
    ", mrl, file_type, file_id, parent_folder_id "
"FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " + parser::Task::Table::Name + "_backup",
