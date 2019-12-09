MediaGroup::schema( MediaGroup::Table::Name, 24 ),
MediaGroup::schema( MediaGroup::FtsTable::Name, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::InsertFts, 24 ),
MediaGroup::trigger( MediaGroup::Triggers::DeleteFts, 24 ),
