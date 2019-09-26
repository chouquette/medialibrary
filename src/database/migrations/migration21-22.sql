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
    " AND folder_id = f.id_folder)"
