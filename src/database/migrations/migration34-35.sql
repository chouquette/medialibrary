/* Ensure we update is_present before the total number of track */
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::UpdateOnTrackDelete, 34 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 35 ),

/* Fix a broken trigger deleting all genres when nb_tracks reaches 0 */
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::DeleteEmpty, 34 ),
Genre::trigger( Genre::Triggers::DeleteEmpty, 35 ),

