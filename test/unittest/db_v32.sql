BEGIN TRANSACTION;
CREATE TABLE Settings(db_model_version UNSIGNED INTEGER NOT NULL,max_task_attempts UNSIGNED INTEGER NOT NULL,max_link_task_attempts UNSIGNED INTEGER NOT NULL);
CREATE TABLE Device(id_device INTEGER PRIMARY KEY AUTOINCREMENT,uuid TEXT COLLATE NOCASE,scheme TEXT,is_removable BOOLEAN,is_present BOOLEAN,is_network BOOLEAN,last_seen UNSIGNED INTEGER,UNIQUE(uuid,scheme) ON CONFLICT FAIL);
CREATE TABLE DeviceMountpoint(device_id INTEGER,mrl TEXT COLLATE NOCASE,last_seen INTEGER,PRIMARY KEY(device_id, mrl) ON CONFLICT REPLACE,FOREIGN KEY(device_id) REFERENCES Device(id_device) ON DELETE CASCADE);
CREATE TABLE Folder(id_folder INTEGER PRIMARY KEY AUTOINCREMENT,path TEXT,name TEXT COLLATE NOCASE,parent_id UNSIGNED INTEGER,is_banned BOOLEAN NOT NULL DEFAULT 0,device_id UNSIGNED INTEGER,is_removable BOOLEAN NOT NULL,nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0,FOREIGN KEY(parent_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,FOREIGN KEY(device_id) REFERENCES Device(id_device) ON DELETE CASCADE,UNIQUE(path,device_id) ON CONFLICT FAIL);
CREATE VIRTUAL TABLE FolderFts USING FTS3(name)
CREATE TABLE Thumbnail(id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,mrl TEXT,status UNSIGNED INTEGER NOT NULL,nb_attempts UNSIGNED INTEGER DEFAULT 0,is_owned BOOLEAN NOT NULL,shared_counter INTEGER NOT NULL DEFAULT 0,file_size INTEGER,hash TEXT);
CREATE TABLE ThumbnailLinking(entity_id UNSIGNED INTEGER NOT NULL,entity_type UNSIGNED INTEGER NOT NULL,size_type UNSIGNED INTEGER NOT NULL,thumbnail_id UNSIGNED INTEGER NOT NULL,origin UNSIGNED INT NOT NULL,PRIMARY KEY(entity_id,entity_type,size_type),FOREIGN KEY(thumbnail_id) REFERENCES Thumbnail(id_thumbnail) ON DELETE CASCADE);
CREATE TABLE ThumbnailCleanup(id_request INTEGER PRIMARY KEY AUTOINCREMENT,mrl TEXT);
CREATE TABLE Media(id_media INTEGER PRIMARY KEY AUTOINCREMENT,type INTEGER,subtype INTEGER NOT NULL DEFAULT 0,duration INTEGER DEFAULT -1,last_position REAL DEFAULT -1,last_time INTEGER DEFAULT -1,play_count UNSIGNED INTEGER,last_played_date UNSIGNED INTEGER,real_last_played_date UNSIGNED INTEGER,insertion_date UNSIGNED INTEGER,release_date UNSIGNED INTEGER,title TEXT COLLATE NOCASE,filename TEXT COLLATE NOCASE,is_favorite BOOLEAN NOT NULL DEFAULT 0,is_present BOOLEAN NOT NULL DEFAULT 1,device_id INTEGER,nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,folder_id UNSIGNED INTEGER,import_type UNSIGNED INTEGER NOT NULL,group_id UNSIGNED INTEGER,forced_title BOOLEAN NOT NULL DEFAULT 0,FOREIGN KEY(group_id) REFERENCES MediaGroup(id_group) ON DELETE RESTRICT,FOREIGN KEY(folder_id) REFERENCES Folder(id_folder));
CREATE VIRTUAL TABLE MediaFts USING FTS3(title,labels)
CREATE TABLE File(id_file INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INT DEFAULT NULL,playlist_id UNSIGNED INT DEFAULT NULL,mrl TEXT,type UNSIGNED INTEGER,last_modification_date UNSIGNED INT,size UNSIGNED INT,folder_id UNSIGNED INTEGER,is_removable BOOLEAN NOT NULL,is_external BOOLEAN NOT NULL,is_network BOOLEAN NOT NULL,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(playlist_id) REFERENCES Playlist(id_playlist) ON DELETE CASCADE,FOREIGN KEY(folder_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,UNIQUE(mrl,folder_id) ON CONFLICT FAIL);
CREATE TABLE Label(id_label INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT UNIQUE ON CONFLICT FAIL);
CREATE TABLE LabelFileRelation(label_id INTEGER,media_id INTEGER,PRIMARY KEY(label_id,media_id),FOREIGN KEY(label_id) REFERENCES Label(id_label) ON DELETE CASCADE,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Playlist(id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE,file_id UNSIGNED INT DEFAULT NULL,creation_date UNSIGNED INT NOT NULL,artwork_mrl TEXT,nb_media UNSIGNED INT NOT NULL DEFAULT 0,nb_present_media UNSIGNED INT NOT NULL DEFAULT 0 CHECK(nb_present_media <= nb_media),FOREIGN KEY(file_id) REFERENCES File(id_file) ON DELETE CASCADE);
CREATE VIRTUAL TABLE PlaylistFts USING FTS3(name)
CREATE TABLE PlaylistMediaRelation(media_id INTEGER,playlist_id INTEGER,position INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE NO ACTION,FOREIGN KEY(playlist_id) REFERENCES Playlist(id_playlist) ON DELETE CASCADE);
CREATE TABLE Genre(id_genre INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,nb_tracks INTEGER NOT NULL DEFAULT 0,is_present INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_tracks));
CREATE VIRTUAL TABLE GenreFts USING FTS3(name)
CREATE TABLE Album(id_album INTEGER PRIMARY KEY AUTOINCREMENT,title TEXT COLLATE NOCASE,artist_id UNSIGNED INTEGER,release_year UNSIGNED INTEGER,short_summary TEXT,nb_tracks UNSIGNED INTEGER DEFAULT 0,duration UNSIGNED INTEGER NOT NULL DEFAULT 0,nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_tracks),FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE);
CREATE VIRTUAL TABLE AlbumFts USING FTS3(title,artist)
CREATE TABLE AlbumTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,media_id INTEGER UNIQUE,duration INTEGER NOT NULL,artist_id UNSIGNED INTEGER,genre_id INTEGER,track_number UNSIGNED INTEGER,album_id UNSIGNED INTEGER NOT NULL,disc_number UNSIGNED INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE,FOREIGN KEY(genre_id) REFERENCES Genre(id_genre),FOREIGN KEY(album_id) REFERENCES Album(id_album)  ON DELETE CASCADE);
CREATE TABLE Show(id_show INTEGER PRIMARY KEY AUTOINCREMENT,title TEXT,nb_episodes UNSIGNED INTEGER NOT NULL DEFAULT 0,release_date UNSIGNED INTEGER,short_summary TEXT,artwork_mrl TEXT,tvdb_id TEXT,is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_episodes));
CREATE VIRTUAL TABLE ShowFts USING FTS3(title)
CREATE TABLE ShowEpisode(id_episode INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INTEGER NOT NULL,episode_number UNSIGNED INT,season_number UNSIGNED INT,episode_title TEXT,episode_summary TEXT,tvdb_id TEXT,show_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(show_id) REFERENCES Show(id_show) ON DELETE CASCADE);
CREATE TABLE Movie(id_movie INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INTEGER NOT NULL,summary TEXT,imdb_id TEXT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE VideoTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,width UNSIGNED INTEGER,height UNSIGNED INTEGER,fps_num UNSIGNED INTEGER,fps_den UNSIGNED INTEGER,bitrate UNSIGNED INTEGER,sar_num UNSIGNED INTEGER,sar_den UNSIGNED INTEGER,media_id UNSIGNED INT,language TEXT,description TEXT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE AudioTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,bitrate UNSIGNED INTEGER,samplerate UNSIGNED INTEGER,nb_channels UNSIGNED INTEGER,language TEXT,description TEXT,media_id UNSIGNED INT,attached_file_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(attached_file_id) REFERENCES File(id_file) ON DELETE CASCADE,UNIQUE(media_id, attached_file_id) ON CONFLICT FAIL);
CREATE TABLE Artist(id_artist INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,shortbio TEXT,nb_albums UNSIGNED INT DEFAULT 0,nb_tracks UNSIGNED INT DEFAULT 0,mb_id TEXT,is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_tracks));
CREATE VIRTUAL TABLE ArtistFts USING FTS3(name)
CREATE TABLE MediaArtistRelation(media_id INTEGER NOT NULL,artist_id INTEGER,PRIMARY KEY(media_id,artist_id),FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE);
CREATE TABLE Task(id_task INTEGER PRIMARY KEY AUTOINCREMENT,step INTEGER NOT NULL DEFAULT 0,attempts_left INTEGER NOT NULL,type INTEGER NOT NULL,mrl TEXT,file_type INTEGER NOT NULL,file_id UNSIGNED INTEGER,parent_folder_id UNSIGNED INTEGER,link_to_id UNSIGNED INTEGER NOT NULL,link_to_type UNSIGNED INTEGER NOT NULL,link_extra UNSIGNED INTEGER NOT NULL,link_to_mrl TEXT NOT NULL,UNIQUE(mrl,type, link_to_id, link_to_type, link_extra, link_to_mrl) ON CONFLICT FAIL,FOREIGN KEY(parent_folder_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,FOREIGN KEY(file_id) REFERENCES File(id_file) ON DELETE CASCADE);
CREATE TABLE Metadata(id_media INTEGER,entity_type INTEGER,type INTEGER,value TEXT,PRIMARY KEY(id_media,entity_type,type));
CREATE TABLE SubtitleTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,language TEXT,description TEXT,encoding TEXT,media_id UNSIGNED INT,attached_file_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(attached_file_id) REFERENCES File(id_file) ON DELETE CASCADE,UNIQUE(media_id, attached_file_id) ON CONFLICT FAIL);
CREATE TABLE Chapter(id_chapter INTEGER PRIMARY KEY AUTOINCREMENT,offset INTEGER NOT NULL,duration INTEGER NOT NULL,name TEXT,media_id INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Bookmark(id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,time UNSIGNED INTEGER NOT NULL,name TEXT,description TEXT,media_id UNSIGNED INTEGER NOT NULL,creation_date UNSIGNED INTEGER NOT NULL,type UNSIGNED INTEGER NOT NULL,FOREIGN KEY(media_id) REFERENCES Media(id_media),UNIQUE(time,media_id) ON CONFLICT FAIL);
CREATE TABLE MediaGroup(id_group INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE,nb_video UNSIGNED INTEGER DEFAULT 0,nb_audio UNSIGNED INTEGER DEFAULT 0,nb_unknown UNSIGNED INTEGER DEFAULT 0,nb_external UNSIGNED INTEGER DEFAULT 0,nb_present_video UNSIGNED INTEGER DEFAULT 0 CHECK(nb_present_video <= nb_video),nb_present_audio UNSIGNED INTEGER DEFAULT 0 CHECK(nb_present_audio <= nb_audio),nb_present_unknown UNSIGNED INTEGER DEFAULT 0 CHECK(nb_present_unknown <= nb_unknown),duration INTEGER DEFAULT 0,creation_date INTEGER NOT NULL,last_modification_date INTEGER NOT NULL,user_interacted BOOLEAN,forced_singleton BOOLEAN);
CREATE VIRTUAL TABLE MediaGroupFts USING FTS3(name)
CREATE TRIGGER insert_folder_fts AFTER INSERT ON Folder BEGIN INSERT INTO FolderFts(rowid,name) VALUES(new.id_folder,new.name);END;
CREATE TRIGGER delete_folder_fts BEFORE DELETE ON Folder BEGIN DELETE FROM FolderFts WHERE rowid = old.id_folder;END;
CREATE TRIGGER update_folder_nb_media_on_insert AFTER INSERT ON Media WHEN new.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE new.type WHEN 1 THEN 1 ELSE 0 END) WHERE id_folder = new.folder_id;END;
CREATE TRIGGER update_folder_nb_media_on_delete AFTER DELETE ON Media WHEN old.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE old.type WHEN 2 THEN -1 ELSE 0 END),nb_video = nb_video + (CASE old.type WHEN 1 THEN -1 ELSE 0 END) WHERE id_folder = old.folder_id;END;
CREATE TRIGGER folder_update_nb_media_on_media_update AFTER UPDATE OF folder_id, type ON Media WHEN IFNULL(old.folder_id, 0) != IFNULL(new.folder_id, 0) OR old.type != new.type BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE new.type WHEN 1 THEN 1 ELSE 0 END)WHERE new.folder_id IS NOT NULL AND id_folder = new.folder_id;UPDATE Folder SET nb_audio = nb_audio - (CASE old.type WHEN 2 THEN -1 ELSE 0 END),nb_video = nb_video - (CASE old.type WHEN 1 THEN 1 ELSE 0 END)WHERE old.folder_id IS NOT NULL AND id_folder = old.folder_id; END;
CREATE INDEX folder_device_id_idx ON Folder (device_id);
CREATE INDEX parent_folder_id_idx ON Folder (parent_id);
CREATE TRIGGER album_is_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 AND old.is_present != new.is_present BEGIN  UPDATE Album SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_album = (SELECT album_id FROM AlbumTrack WHERE media_id = new.id_media); END;
CREATE TRIGGER delete_album_track AFTER DELETE ON AlbumTrack BEGIN  UPDATE Album SET nb_tracks = nb_tracks - 1, is_present = is_present - 1, duration = duration - old.duration WHERE id_album = old.album_id; DELETE FROM Album WHERE id_album=old.album_id AND nb_tracks = 0; END;
CREATE TRIGGER add_album_track AFTER INSERT ON AlbumTrack BEGIN UPDATE Album SET duration = duration + new.duration, nb_tracks = nb_tracks + 1, is_present = is_present + 1 WHERE id_album = new.album_id; END;
CREATE TRIGGER insert_album_fts AFTER INSERT ON Album WHEN new.title IS NOT NULL BEGIN INSERT INTO AlbumFts(rowid, title) VALUES(new.id_album, new.title); END;
CREATE TRIGGER delete_album_fts BEFORE DELETE ON Album WHEN old.title IS NOT NULL BEGIN DELETE FROM AlbumFts WHERE rowid = old.id_album; END;
CREATE INDEX album_artist_id_idx ON Album(artist_id);
CREATE INDEX album_media_artist_genre_album_idx ON AlbumTrack(media_id, artist_id, genre_id, album_id);
CREATE INDEX album_track_album_genre_artist_ids ON AlbumTrack(album_id, genre_id, artist_id);
CREATE TRIGGER artist_has_tracks_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 AND old.is_present != new.is_present BEGIN  UPDATE Artist SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_artist = (SELECT artist_id FROM AlbumTrack  WHERE media_id = new.id_media ); END;
CREATE TRIGGER insert_artist_fts AFTER INSERT ON Artist WHEN new.name IS NOT NULL BEGIN INSERT INTO ArtistFts(rowid,name) VALUES(new.id_artist, new.name); END;
CREATE TRIGGER delete_artist_fts BEFORE DELETE ON Artist WHEN old.name IS NOT NULL BEGIN DELETE FROM ArtistFts WHERE rowid=old.id_artist; END;
CREATE TRIGGER delete_artist_without_tracks AFTER UPDATE OF nb_tracks, nb_albums ON Artist WHEN new.nb_tracks = 0 AND new.nb_albums = 0 AND new.id_artist != 1 AND new.id_artist != 2 BEGIN DELETE FROM Artist WHERE id_artist = old.id_artist; END;
CREATE TRIGGER artist_increment_nb_tracks AFTER INSERT ON MediaArtistRelation BEGIN UPDATE Artist SET nb_tracks = nb_tracks + 1, is_present = is_present + 1 WHERE id_artist = new.artist_id; END;
CREATE TRIGGER artist_decrement_nb_tracks AFTER DELETE ON MediaArtistRelation BEGIN UPDATE Artist SET nb_tracks = nb_tracks - 1, is_present = is_present - 1 WHERE id_artist = old.artist_id; END;
CREATE TRIGGER artist_update_nb_albums AFTER UPDATE OF artist_id ON Album BEGIN UPDATE Artist SET nb_albums = nb_albums + 1 WHERE id_artist = new.artist_id; UPDATE Artist SET nb_albums = nb_albums - 1 WHERE id_artist = old.artist_id; END;
CREATE TRIGGER artist_decrement_nb_albums AFTER DELETE ON Album BEGIN UPDATE Artist SET nb_albums = nb_albums - 1 WHERE id_artist = old.artist_id; END;
CREATE TRIGGER artist_increment_nb_albums_unknown_album AFTER INSERT ON Album WHEN new.artist_id IS NOT NULL BEGIN UPDATE Artist SET nb_albums = nb_albums + 1 WHERE id_artist = new.artist_id; END;
CREATE TRIGGER media_update_device_presence AFTER UPDATE OF is_present ON Device WHEN old.is_present != new.is_present BEGIN UPDATE Media SET is_present=new.is_present WHERE device_id=new.id_device;END;
CREATE TRIGGER media_cascade_file_deletion AFTER DELETE ON File WHEN old.type = 1 OR old.type = 6 BEGIN  DELETE FROM Media WHERE id_media=old.media_id; END;
CREATE TRIGGER insert_media_fts AFTER INSERT ON Media BEGIN INSERT INTO MediaFts(rowid,title,labels) VALUES(new.id_media, new.title, ''); END;
CREATE TRIGGER media_cascade_file_update AFTER UPDATE OF media_id ON File WHEN old.media_id != new.media_id AND old.type = 1 BEGIN DELETE FROM Media WHERE id_media = old.media_id;END;
CREATE TRIGGER delete_media_fts BEFORE DELETE ON Media BEGIN DELETE FROM MediaFts WHERE rowid = old.id_media; END;
CREATE TRIGGER update_media_title_fts AFTER UPDATE OF title ON Media BEGIN UPDATE MediaFts SET title = new.title WHERE rowid = new.id_media; END;
CREATE TRIGGER increment_media_nb_playlist AFTER INSERT ON PlaylistMediaRelation BEGIN UPDATE Media SET nb_playlists = nb_playlists + 1  WHERE id_media = new.media_id; END;
CREATE TRIGGER decrement_media_nb_playlist AFTER DELETE ON PlaylistMediaRelation BEGIN UPDATE Media SET nb_playlists = nb_playlists - 1  WHERE id_media = old.media_id; END;
CREATE INDEX index_last_played_date ON Media(last_played_date DESC);
CREATE INDEX index_media_presence ON Media(is_present);
CREATE INDEX media_types_idx ON Media(type, subtype);
CREATE INDEX media_last_usage_dates_idx ON Media(last_played_date, real_last_played_date, insertion_date);
CREATE INDEX media_folder_id_idx ON Media(folder_id);
CREATE INDEX media_group_id_idx ON Media(group_id);
CREATE INDEX media_last_pos_time_idx ON Media(last_position, last_time);
CREATE INDEX file_media_id_index ON File(media_id);
CREATE INDEX file_folder_id_index ON File(folder_id);
CREATE TRIGGER insert_genre_fts AFTER INSERT ON Genre BEGIN INSERT INTO GenreFts(rowid,name) VALUES(new.id_genre, new.name); END;
CREATE TRIGGER delete_genre_fts BEFORE DELETE ON Genre BEGIN DELETE FROM GenreFts WHERE rowid = old.id_genre; END;
CREATE TRIGGER update_genre_on_new_track AFTER INSERT ON AlbumTrack WHEN new.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks + 1, is_present = is_present + 1 WHERE id_genre = new.genre_id; END;
CREATE TRIGGER update_genre_on_track_deleted AFTER DELETE ON AlbumTrack WHEN old.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks - 1, is_present = is_present - 1 WHERE id_genre = old.genre_id; DELETE FROM Genre WHERE nb_tracks = 0; END;
CREATE TRIGGER genre_update_is_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 AND old.is_present != new.is_present BEGIN UPDATE Genre SET is_present = is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) WHERE id_genre = (SELECT genre_id FROM AlbumTrack WHERE media_id = new.id_media); END;
CREATE TRIGGER update_playlist_order_on_insert AFTER INSERT ON PlaylistMediaRelation WHEN new.position IS NOT NULL BEGIN UPDATE PlaylistMediaRelation SET position = position + 1 WHERE playlist_id = new.playlist_id AND position >= new.position AND rowid != new.rowid; END;
CREATE TRIGGER update_playlist_order_on_delete AFTER DELETE ON PlaylistMediaRelation BEGIN UPDATE PlaylistMediaRelation SET position = position - 1 WHERE playlist_id = old.playlist_id AND position > old.position; END;
CREATE TRIGGER insert_playlist_fts AFTER INSERT ON Playlist BEGIN INSERT INTO PlaylistFts(rowid, name) VALUES(new.id_playlist, new.name); END;
CREATE TRIGGER update_playlist_fts AFTER UPDATE OF name ON Playlist BEGIN UPDATE PlaylistFts SET name = new.name WHERE rowid = new.id_playlist; END;
CREATE TRIGGER delete_playlist_fts BEFORE DELETE ON Playlist BEGIN DELETE FROM PlaylistFts WHERE rowid = old.id_playlist; END;
CREATE TRIGGER playlist_update_nb_media_on_media_deletion AFTER DELETE ON Media BEGIN UPDATE Playlist SET nb_present_media = nb_present_media - (CASE old.is_present WHEN 0 THEN 0 ELSE pl_cnt.cnt END), nb_media = nb_media - pl_cnt.cnt FROM (SELECT COUNT(media_id) AS cnt, playlist_id FROM PlaylistMediaRelation WHERE media_id = old.id_media GROUP BY playlist_id) AS pl_cnt WHERE id_playlist = pl_cnt.playlist_id;DELETE FROM PlaylistMediaRelation WHERE media_id = old.id_media; END;
CREATE TRIGGER playlist_update_nb_present_media AFTER UPDATE OF is_present ON Media WHEN old.is_present != new.is_present BEGIN UPDATE Playlist SET nb_present_media = nb_present_media + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) WHERE id_playlist IN (SELECT DISTINCT playlist_id FROM PlaylistMediaRelation WHERE media_id = new.id_media); END;
CREATE INDEX playlist_file_id ON Playlist(file_id);
CREATE INDEX playlist_position_pl_id_index ON PlaylistMediaRelation(playlist_id,position);
CREATE TRIGGER delete_label_fts BEFORE DELETE ON Label BEGIN UPDATE MediaFts SET labels = TRIM(REPLACE(labels, old.name, '')) WHERE labels MATCH old.name; END;
CREATE TRIGGER insert_show_fts AFTER INSERT ON Show BEGIN INSERT INTO ShowFts(rowid,title) VALUES(new.id_show, new.title); END;
CREATE TRIGGER delete_show_fts BEFORE DELETE ON Show BEGIN DELETE FROM ShowFts WHERE rowid = old.id_show; END;
CREATE TRIGGER show_increment_nb_episode AFTER INSERT ON ShowEpisode BEGIN UPDATE Show SET nb_episodes = nb_episodes + 1, is_present = is_present + 1 WHERE id_show = new.show_id; END;
CREATE TRIGGER show_decrement_nb_episode AFTER DELETE ON ShowEpisode BEGIN UPDATE Show SET nb_episodes = nb_episodes - 1, is_present = is_present - 1 WHERE id_show = old.show_id; END;
CREATE TRIGGER show_update_is_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 1 AND new.is_present != old.is_present BEGIN  UPDATE Show SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) WHERE id_show = (SELECT show_id FROM ShowEpisode WHERE media_id = new.id_media); END;
CREATE INDEX show_episode_media_show_idx ON ShowEpisode(media_id, show_id);
CREATE TRIGGER auto_delete_album_thumbnail AFTER DELETE ON Album BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_album AND entity_type = 1; END;
CREATE TRIGGER auto_delete_artist_thumbnail AFTER DELETE ON Artist BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_artist AND entity_type = 2; END;
CREATE TRIGGER auto_delete_media_thumbnail AFTER DELETE ON Media BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_media AND entity_type = 0; END;
CREATE TRIGGER incr_thumbnail_refcount AFTER INSERT ON ThumbnailLinking BEGIN UPDATE Thumbnail SET shared_counter = shared_counter + 1 WHERE id_thumbnail = new.thumbnail_id;END;
CREATE TRIGGER decr_thumbnail_refcount AFTER DELETE ON ThumbnailLinking BEGIN UPDATE Thumbnail SET shared_counter = shared_counter - 1 WHERE id_thumbnail = old.thumbnail_id;END;
CREATE TRIGGER update_thumbnail_refcount AFTER UPDATE OF thumbnail_id ON ThumbnailLinking WHEN old.thumbnail_id != new.thumbnail_id BEGIN UPDATE Thumbnail SET shared_counter = shared_counter - 1 WHERE id_thumbnail = old.thumbnail_id;UPDATE Thumbnail SET shared_counter = shared_counter + 1 WHERE id_thumbnail = new.thumbnail_id;END;
CREATE TRIGGER delete_unused_thumbnail AFTER UPDATE OF shared_counter ON Thumbnail WHEN new.shared_counter = 0 BEGIN DELETE FROM Thumbnail WHERE id_thumbnail = new.id_thumbnail;END;
CREATE TRIGGER thumbnail_insert_cleanup AFTER DELETE ON Thumbnail WHEN old.is_owned != 0 AND old.status = 1 BEGIN INSERT INTO ThumbnailCleanup(mrl) VALUES(old.mrl); END;
CREATE INDEX thumbnail_link_index ON ThumbnailLinking(thumbnail_id);
CREATE TRIGGER delete_playlist_linking_tasks AFTER DELETE ON Playlist BEGIN DELETE FROM Task WHERE link_to_type = 1 AND link_to_id = old.id_playlist AND type = 1;END;
CREATE INDEX audio_track_media_idx ON AudioTrack(media_id);
CREATE INDEX subtitle_track_media_idx ON SubtitleTrack(media_id);
CREATE INDEX video_track_media_idx ON VideoTrack(media_id);
CREATE TRIGGER media_group_insert_fts AFTER INSERT ON MediaGroup BEGIN INSERT INTO MediaGroupFts(rowid, name) VALUES(new.rowid, new.name); END;
CREATE TRIGGER media_group_delete_fts AFTER DELETE ON MediaGroup BEGIN DELETE FROM MediaGroupFts WHERE rowid = old.id_group; END;
CREATE TRIGGER media_group_update_nb_media_types AFTER UPDATE OF type, group_id ON Media WHEN (IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0) OR old.type != new.type) AND new.import_type = 0 BEGIN UPDATE MediaGroup SET nb_video = nb_video + (CASE new.type WHEN 1 THEN 1 ELSE 0 END), nb_present_video = nb_present_video + (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 1 THEN 1 ELSE 0 END) END), nb_audio = nb_audio + (CASE new.type WHEN 2 THEN 1 ELSE 0 END), nb_present_audio = nb_present_audio + (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 2 THEN 1 ELSE 0 END) END), nb_unknown = nb_unknown + (CASE new.type WHEN 0 THEN 1 ELSE 0 END), nb_present_unknown = nb_present_unknown + (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 0 THEN 1 ELSE 0 END) END), last_modification_date = strftime('%s') WHERE new.group_id IS NOT NULL AND id_group = new.group_id; UPDATE MediaGroup SET nb_present_video = nb_present_video - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 1 THEN 1 ELSE 0 END) END), nb_video = nb_video - (CASE old.type WHEN 1 THEN 1 ELSE 0 END), nb_present_audio = nb_present_audio - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 2 THEN 1 ELSE 0 END) END), nb_audio = nb_audio - (CASE old.type WHEN 2 THEN 1 ELSE 0 END), nb_present_unknown = nb_present_unknown - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 0 THEN 1 ELSE 0 END) END), nb_unknown = nb_unknown - (CASE old.type WHEN 0 THEN 1 ELSE 0 END), last_modification_date = strftime('%s') WHERE old.group_id IS NOT NULL AND id_group = old.group_id; END;
CREATE TRIGGER media_group_decrement_nb_media_on_deletion AFTER DELETE ON Media WHEN old.group_id IS NOT NULL BEGIN UPDATE MediaGroup SET nb_present_video = nb_present_video - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 1 THEN 1 ELSE 0 END) END), nb_video = nb_video - (CASE old.type WHEN 1 THEN 1 ELSE 0 END), nb_present_audio = nb_present_audio - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 2 THEN 1 ELSE 0 END) END), nb_audio = nb_audio - (CASE old.type WHEN 2 THEN 1 ELSE 0 END), nb_present_unknown = nb_present_unknown - (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 0 THEN 1 ELSE 0 END) END), nb_unknown = nb_unknown - (CASE old.type WHEN 0 THEN 1 ELSE 0 END), last_modification_date = strftime('%s') WHERE id_group = old.group_id; END;
CREATE TRIGGER media_group_delete_empty_group AFTER UPDATE OF nb_video, nb_audio, nb_unknown, nb_external ON MediaGroup WHEN new.nb_video = 0 AND new.nb_audio = 0 AND new.nb_unknown = 0 AND new.nb_external = 0 BEGIN DELETE FROM MediaGroup WHERE id_group = new.id_group; END;
CREATE TRIGGER media_group_rename_forced_singleton AFTER UPDATE OF title ON Media WHEN new.group_id IS NOT NULL BEGIN UPDATE MediaGroup SET name = new.title WHERE id_group = new.group_id AND forced_singleton != 0; END;
CREATE TRIGGER media_group_update_duration_on_media_change AFTER UPDATE OF duration, group_id ON Media BEGIN UPDATE MediaGroup SET duration = duration - max(old.duration, 0) WHERE id_group = old.group_id; UPDATE MediaGroup SET duration = duration + max(new.duration, 0) WHERE id_group = new.group_id; END;
CREATE TRIGGER media_group_update_duration_on_media_deletion AFTER DELETE ON Media WHEN old.group_id IS NOT NULL AND old.duration > 0 BEGIN UPDATE MediaGroup SET duration = duration - old.duration WHERE id_group = old.group_id; END;
CREATE TRIGGER media_group_update_nb_media_types_presence AFTER UPDATE OF is_present ON Media WHEN old.is_present != new.is_present AND new.group_id IS NOT NULL BEGIN UPDATE MediaGroup SET nb_present_video = nb_present_video +  (CASE new.type WHEN 1 THEN 1 ELSE 0 END) * (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END), nb_present_audio = nb_present_audio +  (CASE new.type WHEN 2 THEN 1 ELSE 0 END) * (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END), nb_present_unknown = nb_present_unknown +  (CASE new.type WHEN 0 THEN 1 ELSE 0 END) * (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END) WHERE id_group = new.group_id; END;
CREATE TRIGGER media_group_update_media_count_on_import_type_change AFTER UPDATE OF group_id, import_type ON Media WHEN ( IFNULL(old.group_id, 0) != IFNULL(new.group_id, 0)  AND new.import_type != 0 ) OR new.import_type != old.import_type BEGIN UPDATE MediaGroup SET nb_video = nb_video + (CASE new.import_type WHEN 0 THEN (CASE new.type WHEN 1 THEN 1 ELSE 0 END) ELSE 0 END), nb_present_video = nb_present_video + (CASE new.import_type WHEN 0 THEN (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 1 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_audio = nb_audio + (CASE new.import_type WHEN 0 THEN (CASE new.type WHEN 2 THEN 1 ELSE 0 END) ELSE 0 END), nb_present_audio = nb_present_audio + (CASE new.import_type WHEN 0 THEN (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 2 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_unknown = nb_unknown + (CASE new.import_type WHEN 0 THEN (CASE new.type WHEN 0 THEN 1 ELSE 0 END) ELSE 0 END), nb_present_unknown = nb_present_unknown + (CASE new.import_type WHEN 0 THEN (CASE new.is_present WHEN 0 THEN 0 ELSE (CASE new.type WHEN 0 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_external = nb_external + (CASE new.import_type WHEN 0 THEN 0 ELSE 1 END), last_modification_date = strftime('%s') WHERE new.group_id IS NOT NULL AND id_group = new.group_id; UPDATE MediaGroup SET nb_present_video = nb_present_video - (CASE old.import_type WHEN 0 THEN (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 1 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_video = nb_video - (CASE old.import_type WHEN 0 THEN (CASE old.type WHEN 1 THEN 1 ELSE 0 END) ELSE 0 END), nb_present_audio = nb_present_audio - (CASE old.import_type WHEN 0 THEN (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 2 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_audio = nb_audio - (CASE old.import_type WHEN 0 THEN (CASE old.type WHEN 2 THEN 1 ELSE 0 END) ELSE 0 END), nb_present_unknown = nb_present_unknown - (CASE old.import_type WHEN 0 THEN (CASE old.is_present WHEN 0 THEN 0 ELSE (CASE old.type WHEN 0 THEN 1 ELSE 0 END) END) ELSE 0 END), nb_unknown = nb_unknown - (CASE old.import_type WHEN 0 THEN (CASE old.type WHEN 0 THEN 1 ELSE 0 END) ELSE 0 END), nb_external = nb_external - (CASE old.import_type WHEN 0 THEN 0 ELSE 1 END), last_modification_date = strftime('%s') WHERE old.group_id IS NOT NULL AND id_group = old.group_id; END;
CREATE INDEX media_group_forced_singleton ON MediaGroup(forced_singleton);
CREATE INDEX media_group_duration ON MediaGroup(duration);
CREATE INDEX media_group_creation_date ON MediaGroup(creation_date);
CREATE INDEX media_group_last_modification_date ON MediaGroup(last_modification_date);
CREATE INDEX movie_media_idx ON Movie(media_id);
CREATE INDEX task_parent_folder_id_idx ON Task(parent_folder_id);
INSERT INTO Settings VALUES(32,2,6);
INSERT INTO MediaGroup VALUES(1,'test group',0,2,0,0,0,2,0,33,0,0,0,0);
INSERT INTO MediaGroup VALUES(2,'test group',0,2,0,0,0,2,0,20114,0,0,0,0);
INSERT INTO Device VALUES(1,'720f7152-67b7-41da-89cf-bf22cef92095','file://',0,1,0,0);
INSERT INTO Folder VALUES(1,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/banned/',NULL,NULL,1,1,0,0,0);
INSERT INTO Folder VALUES(2,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/','entrypoint1',NULL,0,1,0,0,0);
INSERT INTO Folder VALUES(3,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/','entrypoint2',NULL,0,1,0,0,0);
INSERT INTO Folder VALUES(4,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/','album1',2,0,1,0,3,0);
INSERT INTO Folder VALUES(5,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/','artist1',3,0,1,0,0,0);
INSERT INTO Folder VALUES(6,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/','album2',5,0,1,0,3,0);
INSERT INTO Folder VALUES(7,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/','album1',5,0,1,0,3,0);
INSERT INTO Media VALUES(1,2,3,11,-1.0,-1,123,NULL,NULL,1627997947,0,'ep1-album1-track1','track1.mp3',0,1,1,0,4,0,1,0);
INSERT INTO Media VALUES(2,2,3,-1,-1.0,-1,NULL,NULL,NULL,1627997947,0,'ep1-album1-track2','track2.mp3',0,1,1,0,4,0,NULL,0);
INSERT INTO Media VALUES(3,2,3,22,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep1-album1-track3','track3.mp3',0,1,1,0,4,0,1,0);
INSERT INTO Media VALUES(4,2,3,10057,-1.0,-1,1,NULL,NULL,1627997948,0,'ep2-album2-track1','track1.mp3',0,1,1,0,6,0,2,0);
INSERT INTO Media VALUES(5,2,3,10057,-1.0,-1,1,NULL,NULL,1627997948,0,'ep2-album2-track2','track2.mp3',0,1,1,0,6,0,2,0);
INSERT INTO Media VALUES(6,2,3,10057,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep2-album2-track3','track3.mp3',0,1,1,0,6,0,NULL,0);
INSERT INTO Media VALUES(7,2,3,10057,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep2-album1-track1','track1.mp3',0,1,1,0,7,0,NULL,0);
INSERT INTO Media VALUES(8,2,3,10057,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep2-album1-track2','track2.mp3',0,1,1,0,7,0,NULL,0);
INSERT INTO Media VALUES(9,2,3,10057,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep2-album1-track3','track3.mp3',0,1,1,0,7,0,NULL,0);
INSERT INTO Media VALUES(10,1,2,10057,-1.0,-1,NULL,NULL,NULL,1627997948,0,'ep2-album1-track3','track3.mp3',0,1,1,0,7,0,NULL,0);
INSERT INTO File VALUES(1,1,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track1.mp3',1,1619766627,139264,4,0,0,0);
INSERT INTO File VALUES(2,2,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track2.mp3',1,1619766627,139264,4,0,0,0);
INSERT INTO File VALUES(3,3,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track3.mp3',1,1619767120,139264,4,0,0,0);
INSERT INTO File VALUES(4,4,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track1.mp3',1,1619766853,139264,6,0,0,0);
INSERT INTO File VALUES(5,5,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track2.mp3',1,1619766864,139264,6,0,0,0);
INSERT INTO File VALUES(6,6,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track3.mp3',1,1619766881,139264,6,0,0,0);
INSERT INTO File VALUES(7,7,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track1.mp3',1,1619766837,139264,7,0,0,0);
INSERT INTO File VALUES(8,8,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track2.mp3',1,1619766837,139264,7,0,0,0);
INSERT INTO File VALUES(9,9,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track3.mp3',1,1619766834,139264,7,0,0,0);
INSERT INTO File VALUES(10,10,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/movie.mkv',1,1619766834,139264,7,0,0,0);
INSERT INTO File VALUES(11,NULL,2,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/playlist.xspf',5,1619766834,0,7,0,0,0);
INSERT INTO Album VALUES(1,'ep1-album1',3,NULL,NULL,3,30171,1,3);
INSERT INTO Album VALUES(2,'ep2-album2',4,NULL,NULL,3,30171,1,3);
INSERT INTO Album VALUES(3,'ep2-album1',4,NULL,NULL,3,30171,1,3);
INSERT INTO AlbumTrack VALUES(1,1,10057,3,NULL,1,1,0);
INSERT INTO AlbumTrack VALUES(2,2,10057,3,NULL,2,1,0);
INSERT INTO AlbumTrack VALUES(3,3,10057,3,NULL,3,1,0);
INSERT INTO AlbumTrack VALUES(4,4,10057,4,NULL,1,2,0);
INSERT INTO AlbumTrack VALUES(5,5,10057,4,NULL,2,2,0);
INSERT INTO AlbumTrack VALUES(6,6,10057,4,NULL,3,2,0);
INSERT INTO AlbumTrack VALUES(7,7,10057,4,NULL,1,3,0);
INSERT INTO AlbumTrack VALUES(8,8,10057,4,NULL,2,3,0);
INSERT INTO AlbumTrack VALUES(9,9,10057,4,NULL,3,3,0);
INSERT INTO Show VALUES(1,NULL,0,NULL,NULL,NULL,NULL,0);
INSERT INTO AudioTrack VALUES(1,'mpga',128000,44100,2,'','',1,NULL);
INSERT INTO AudioTrack VALUES(2,'mpga',128000,44100,2,'','',2,NULL);
INSERT INTO AudioTrack VALUES(3,'mpga',128000,44100,2,'','',3,NULL);
INSERT INTO AudioTrack VALUES(4,'mpga',128000,44100,2,'','',4,NULL);
INSERT INTO AudioTrack VALUES(5,'mpga',128000,44100,2,'','',5,NULL);
INSERT INTO AudioTrack VALUES(6,'mpga',128000,44100,2,'','',6,NULL);
INSERT INTO AudioTrack VALUES(7,'mpga',128000,44100,2,'','',7,NULL);
INSERT INTO AudioTrack VALUES(8,'mpga',128000,44100,2,'','',8,NULL);
INSERT INTO AudioTrack VALUES(9,'mpga',128000,44100,2,'','',9,NULL);
INSERT INTO Artist VALUES(1,NULL,NULL,0,0,NULL,0);
INSERT INTO Artist VALUES(2,NULL,NULL,0,0,NULL,0);
INSERT INTO Artist VALUES(3,'ep1-artist',NULL,1,3,NULL,3);
INSERT INTO Artist VALUES(4,'ep2-artist',NULL,2,6,NULL,6);
INSERT INTO MediaArtistRelation VALUES(1,3);
INSERT INTO MediaArtistRelation VALUES(2,3);
INSERT INTO MediaArtistRelation VALUES(3,3);
INSERT INTO MediaArtistRelation VALUES(4,4);
INSERT INTO MediaArtistRelation VALUES(5,4);
INSERT INTO MediaArtistRelation VALUES(6,4);
INSERT INTO MediaArtistRelation VALUES(7,4);
INSERT INTO MediaArtistRelation VALUES(8,4);
INSERT INTO MediaArtistRelation VALUES(9,4);
INSERT INTO Task VALUES(1,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track1.mp3',1,1,4,0,0,0,'');
INSERT INTO Task VALUES(2,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track2.mp3',1,2,4,0,0,0,'');
INSERT INTO Task VALUES(3,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint1/album1/track3.mp3',1,3,4,0,0,0,'');
INSERT INTO Task VALUES(4,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track1.mp3',1,4,6,0,0,0,'');
INSERT INTO Task VALUES(5,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track2.mp3',1,5,6,0,0,0,'');
INSERT INTO Task VALUES(6,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album2/track3.mp3',1,6,6,0,0,0,'');
INSERT INTO Task VALUES(7,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track1.mp3',1,7,7,0,0,0,'');
INSERT INTO Task VALUES(8,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track2.mp3',1,8,7,0,0,0,'');
INSERT INTO Task VALUES(9,3,2,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/various/entrypoint2/artist1/album1/track3.mp3',1,9,7,0,0,0,'');
INSERT INTO Playlist VALUES(1, 'audio playlist', NULL, 0, NULL, 2, 2);
INSERT INTO PlaylistMediaRelation VALUES(1, 1, 0);
INSERT INTO PlaylistMediaRelation VALUES(2, 1, 1);
INSERT INTO PlaylistMediaRelation VALUES(3, 1, 2);
INSERT INTO Playlist VALUES (2,'mixed playlist',11, 0, NULL, 4, 4);
INSERT INTO PlaylistMediaRelation VALUES (1,2,0);
INSERT INTO PlaylistMediaRelation VALUES (2,2,1);
INSERT INTO PlaylistMediaRelation VALUES (3,2,2);
INSERT INTO PlaylistMediaRelation VALUES (10,2,3);
INSERT INTO Playlist VALUES (3,'Z empty playlist', NULL, 0, NULL, 0, 0);
COMMIT;
