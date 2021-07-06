"DROP TRIGGER " + Folder::triggerName( Folder::Triggers::UpdateNbMediaOnUpdate, 30 ),
Folder::trigger( Folder::Triggers::UpdateNbMediaOnUpdate, 31 ),

"CREATE TEMPORARY TABLE " + MediaGroup::Table::Name + "_backup "
"("
    "id_group INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT COLLATE NOCASE,"
    "nb_video UNSIGNED INTEGER DEFAULT 0,"
    "nb_audio UNSIGNED INTEGER DEFAULT 0,"
    "nb_unknown UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_video UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_audio UNSIGNED INTEGER DEFAULT 0,"
    "nb_present_unknown UNSIGNED INTEGER DEFAULT 0,"
    "duration INTEGER DEFAULT 0,"
    "creation_date INTEGER NOT NULL,"
    "last_modification_date INTEGER NOT NULL,"
    "user_interacted BOOLEAN,"
    "forced_singleton BOOLEAN"
")",

"INSERT INTO " + MediaGroup::Table::Name + "_backup "
    "SELECT * FROM " + MediaGroup::Table::Name,

"DROP TABLE " + MediaGroup::Table::Name,

MediaGroup::schema( MediaGroup::Table::Name, 31 ),

"INSERT INTO " + MediaGroup::Table::Name + " SELECT id_group, name, nb_video,"
    "nb_audio, nb_unknown, 0, nb_present_video, nb_present_audio,"
    "nb_present_unknown, duration, creation_date, last_modification_date,"
    "user_interacted, forced_singleton FROM "
    + MediaGroup::Table::Name + "_backup",

"DROP TABLE " + MediaGroup::Table::Name + "_backup",

"UPDATE " + MediaGroup::Table::Name + " SET "
    "nb_video = nb_video - (CASE MQ.type WHEN " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                        IMedia::Type::Video ) ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_video = nb_present_video - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                IMedia::Type::Video ) ) +
                " THEN 1 ELSE 0 END)"
        "END),"
    "nb_audio = nb_audio - (CASE MQ.type WHEN " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                        IMedia::Type::Audio ) ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_audio = nb_present_audio - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                IMedia::Type::Audio ) ) +
                " THEN 1 ELSE 0 END)"
        "END),"
    "nb_unknown = nb_unknown - (CASE MQ.type WHEN " +
        std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                        IMedia::Type::Unknown ) ) +
        " THEN 1 ELSE 0 END),"
    "nb_present_unknown = nb_present_unknown - "
        "(CASE MQ.is_present WHEN 0 THEN 0 ELSE "
            "(CASE MQ.type WHEN " +
                std::to_string( static_cast<std::underlying_type_t<IMedia::Type>>(
                                IMedia::Type::Unknown ) ) +
                " THEN 1 ELSE 0 END)"
        "END) "
    "FROM (SELECT * FROM " + Media::Table::Name + " WHERE import_type = "
        + std::to_string( static_cast<std::underlying_type_t<Media::ImportType>>(
            Media::ImportType::External ) ) + ") as MQ "
    "WHERE MQ.group_id = id_group",

MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 31 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 31 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::UpdateNbMediaPerType, 30 ),
MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaPerType, 31 ),

"DROP TRIGGER " + MediaGroup::triggerName( MediaGroup::Triggers::DeleteEmptyGroups, 30 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteEmptyGroups, 31 ),

MediaGroup::trigger( MediaGroup::Triggers::UpdateNbMediaOnImportTypeChange, 31 ),

MediaGroup::index( MediaGroup::Indexes::ForcedSingleton, 31 ),
MediaGroup::index( MediaGroup::Indexes::Duration, 31 ),
MediaGroup::index( MediaGroup::Indexes::CreationDate, 31 ),
MediaGroup::index( MediaGroup::Indexes::LastModificationDate, 31 ),
