/******* Fix NULL device_id & folder_id in media table ********/

/*
 * First, update the folder_id since we will need it to fetch the device ID
 * from the folder
 */
"UPDATE " + Media::Table::Name + " AS m SET folder_id = "
    "(SELECT folder_id FROM " + File::Table::Name + " f "
        "WHERE f.media_id = m.id_media "
            " AND f.type = " + std::to_string(
                static_cast<std::underlying_type_t<IFile::Type>>( IFile::Type::Main ) ) +
    ")"
    "WHERE m.folder_id IS NULL",

/*
 * Now we can use the folder to fetch the device id
 */
 "UPDATE " + Media::Table::Name + " AS m SET device_id = "
     "(SELECT device_id FROM " + Folder::Table::Name + " f "
         "WHERE f.id_folder = m.folder_id)"
     "WHERE m.device_id IS NULL",

/*
 * Now we have to update all folders media count. Since the media/folder relation
 * was broken, some media might have been removed, and we can't guess that from
 * the database, so we have to recompute everything.
 */
"UPDATE " + Folder::Table::Name + " AS f SET "
    "nb_audio = (SELECT COUNT() FROM " + Media::Table::Name + " WHERE type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Audio ) ) +
        " AND folder_id = f.id_folder),"
    "nb_video = (SELECT COUNT() FROM " + Media::Table::Name + " WHERE type = " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
            IMedia::Type::Video ) ) +
    " AND folder_id = f.id_folder)",

/* cf migration 20 -> 21 for the reasoning about videogroup being commented out */
/*
"DROP VIEW " + VideoGroup::Table::Name,
VideoGroup::schema( VideoGroup::Table::Name, 22 ),
*/

"ALTER TABLE Settings ADD COLUMN video_groups_minimum_media_count UNSIGNED INTEGER NOT NULL DEFAULT 1",

/*
 * Update UNIQUE contraint and NOT NULL contraints for Taks.link_* fields
 */
 parser::Task::schema( parser::Task::Table::Name, 21, true ),

 "INSERT INTO " + parser::Task::Table::Name + "_backup"
     " SELECT * FROM " + parser::Task::Table::Name,

 "DROP TABLE " + parser::Task::Table::Name,

 parser::Task::schema( parser::Task::Table::Name, 22, false ),

 "INSERT INTO " + parser::Task::Table::Name +
     " SELECT id_task, step, retry_count, type, mrl, file_type, file_id,"
         "parent_folder_id, link_to_id, ifnull(link_to_type, 0), ifnull(link_extra, 0)"
     " FROM " + parser::Task::Table::Name + "_backup",

 "DROP TABLE " +  parser::Task::Table::Name + "_backup",
