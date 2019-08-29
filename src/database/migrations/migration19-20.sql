"DROP TABLE " + AlbumTrack::Table::Name,
AlbumTrack::schema( AlbumTrack::Table::Name, 20 ),

"DROP TABLE " + ShowEpisode::Table::Name,
ShowEpisode::schema( ShowEpisode::Table::Name, 20 ),

"DROP TABLE " + Genre::Table::Name,
"DROP TABLE " + Genre::FtsTable::Name,
Genre::schema( Genre::Table::Name, 20 ),
Genre::schema( Genre::FtsTable::Name, 20 ),

/* Migrate the task table to update the Task.link_to_id field to a NOT NULL one
 * and update the UNIQUE constaint on (task type/mrl/link_to_id) accordingly */

parser::Task::schema( parser::Task::Table::Name, 19, true ),

"INSERT INTO " + parser::Task::Table::Name + "_backup"
    " SELECT * FROM " + parser::Task::Table::Name,

"DROP TABLE " + parser::Task::Table::Name,

parser::Task::schema( parser::Task::Table::Name, 20, false ),

"INSERT INTO " + parser::Task::Table::Name +
    " SELECT id_task, step, retry_count, type, mrl, file_type, file_id,"
        "parent_folder_id, ifnull(link_to_id, 0), link_to_type, link_extra"
    " FROM " + parser::Task::Table::Name + "_backup",

"DROP TABLE " +  parser::Task::Table::Name + "_backup",
