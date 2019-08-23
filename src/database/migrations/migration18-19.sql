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

parser::Task::schema( parser::Task::Table::Name, 19 ),
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
