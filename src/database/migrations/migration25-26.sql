/*
 * Ensure we don't have left over shows in the FTS table while the actual one
 * is empty
 */
"CREATE TEMPORARY TABLE " + Show::Table::Name + "_backup"
"("
    "id_show INTEGER PRIMARY KEY,"
    "title TEXT,"
    "nb_episodes UNSIGNED INTEGER,"
    "release_date UNSIGNED INTEGER,"
    "short_summary TEXT,"
    "artwork_mrl TEXT,"
    "tvdb_id TEXT,"
    "is_present UNSIGNED INTEGER"
")",

"INSERT INTO " + Show::Table::Name + "_backup SELECT * FROM " + Show::Table::Name,

"DROP TABLE " + Show::Table::Name,
"DROP TABLE " + Show::FtsTable::Name,

Show::schema( Show::Table::Name, 26 ),
Show::schema( Show::FtsTable::Name, 26 ),

"INSERT INTO " + Show::Table::Name + " SELECT * FROM " + Show::Table::Name + "_backup",

"DROP TABLE " + Show::Table::Name + "_backup",

Show::trigger( Show::Triggers::InsertFts, 26 ),
Show::trigger( Show::Triggers::DeleteFts, 26 ),

/* Update MediaGroup triggers */
"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::IncrementNbMediaOnGroupChange, 25 ),
"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::DecrementNbMediaOnGroupChange, 25 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMedia, 26 ),


/*
 * Ensure we don't have restore tasks with no file_type specified. At this point
 * the only restore tasks that can be found in database are playlist restoration
 * tasks.
 */
"UPDATE " + parser::Task::Table::Name + " SET file_type = " +
    std::to_string( static_cast<std::underlying_type_t<IFile::Type>>(
        IFile::Type::Playlist ) ) +
    " WHERE type = " +
    std::to_string( static_cast<std::underlying_type_t<parser::Task::Type>>(
        parser::Task::Type::Restore ) ) +
    " AND file_type = " +
    std::to_string( static_cast<std::underlying_type_t<IFile::Type>>(
        IFile::Type::Unknown ) ),
