"CREATE TRIGGER IF NOT EXISTS delete_playlist_linking_tasks "
"AFTER DELETE ON " + Playlist::Table::Name + " "
"BEGIN "
    "DELETE FROM " + parser::Task::Table::Name + " "
        "WHERE link_to_type = " +
                std::to_string( static_cast<std::underlying_type_t<parser::IItem::LinkType>>(
                    parser::IItem::LinkType::Playlist ) ) + " "
            "AND link_to_id = old.id_playlist "
            "AND type = " +
                std::to_string( static_cast<std::underlying_type_t<parser::Task::Type>>(
                    parser::Task::Type::Link ) ) + ";"
"END",
