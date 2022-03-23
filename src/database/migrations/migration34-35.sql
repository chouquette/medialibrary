/* Ensure we update is_present before the total number of track */
"DROP TRIGGER " + Genre::triggerName( Genre::Triggers::UpdateOnTrackDelete, 34 ),
Genre::trigger( Genre::Triggers::UpdateOnTrackDelete, 35 ),
