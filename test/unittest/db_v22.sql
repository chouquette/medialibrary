BEGIN TRANSACTION;
CREATE TABLE Settings(db_model_version UNSIGNED INTEGER NOT NULL,video_groups_prefix_length UNSIGNED INTEGER NOT NULL,video_groups_minimum_media_count UNSIGNED INTEGER NOT NULL);
CREATE TABLE Device(id_device INTEGER PRIMARY KEY AUTOINCREMENT,uuid TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,scheme TEXT,is_removable BOOLEAN,is_present BOOLEAN,last_seen UNSIGNED INTEGER);
CREATE TABLE Folder(id_folder INTEGER PRIMARY KEY AUTOINCREMENT,path TEXT,name TEXT COLLATE NOCASE,parent_id UNSIGNED INTEGER,is_banned BOOLEAN NOT NULL DEFAULT 0,device_id UNSIGNED INTEGER,is_removable BOOLEAN NOT NULL,nb_audio UNSIGNED INTEGER NOT NULL DEFAULT 0,nb_video UNSIGNED INTEGER NOT NULL DEFAULT 0,FOREIGN KEY(parent_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,FOREIGN KEY(device_id) REFERENCES Device(id_device) ON DELETE CASCADE,UNIQUE(path,device_id) ON CONFLICT FAIL);
CREATE TABLE ExcludedEntryFolder(folder_id UNSIGNED INTEGER NOT NULL,FOREIGN KEY(folder_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,UNIQUE(folder_id) ON CONFLICT FAIL);
CREATE VIRTUAL TABLE FolderFts USING FTS3(name);
CREATE TABLE Thumbnail(id_thumbnail INTEGER PRIMARY KEY AUTOINCREMENT,mrl TEXT,is_generated BOOLEAN NOT NULL,shared_counter INTEGER NOT NULL DEFAULT 0);
CREATE TABLE ThumbnailLinking(entity_id UNSIGNED INTEGER NOT NULL,entity_type UNSIGNED INTEGER NOT NULL,size_type UNSIGNED INTEGER NOT NULL,thumbnail_id UNSIGNED INTEGER NOT NULL,origin UNSIGNED INT NOT NULL,PRIMARY KEY(entity_id,entity_type,size_type),FOREIGN KEY(thumbnail_id) REFERENCES Thumbnail(id_thumbnail) ON DELETE CASCADE);
CREATE TABLE Media(id_media INTEGER PRIMARY KEY AUTOINCREMENT,type INTEGER,subtype INTEGER NOT NULL DEFAULT 0,duration INTEGER DEFAULT -1,play_count UNSIGNED INTEGER,last_played_date UNSIGNED INTEGER,real_last_played_date UNSIGNED INTEGER,insertion_date UNSIGNED INTEGER,release_date UNSIGNED INTEGER,title TEXT COLLATE NOCASE,filename TEXT COLLATE NOCASE,is_favorite BOOLEAN NOT NULL DEFAULT 0,is_present BOOLEAN NOT NULL DEFAULT 1,device_id INTEGER,nb_playlists UNSIGNED INTEGER NOT NULL DEFAULT 0,folder_id UNSIGNED INTEGER,FOREIGN KEY(folder_id) REFERENCES Folder(id_folder));
CREATE VIRTUAL TABLE MediaFts USING FTS3(title,labels);
CREATE VIEW VideoGroup AS SELECT LOWER(SUBSTR(CASE WHEN title LIKE 'The %' THEN SUBSTR(title, 5) ELSE title END, 1, (SELECT video_groups_prefix_length FROM Settings))) as grp, COUNT() as cnt, VIDEO_GROUP_AGGREGATE(title) as display FROM Media  WHERE type = 1 AND is_present != 0 GROUP BY grp;
CREATE TABLE File(id_file INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INT DEFAULT NULL,playlist_id UNSIGNED INT DEFAULT NULL,mrl TEXT,type UNSIGNED INTEGER,last_modification_date UNSIGNED INT,size UNSIGNED INT,folder_id UNSIGNED INTEGER,is_removable BOOLEAN NOT NULL,is_external BOOLEAN NOT NULL,is_network BOOLEAN NOT NULL,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(playlist_id) REFERENCES Playlist(id_playlist) ON DELETE CASCADE,FOREIGN KEY(folder_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,UNIQUE(mrl,folder_id) ON CONFLICT FAIL);
CREATE TABLE Label(id_label INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT UNIQUE ON CONFLICT FAIL);
CREATE TABLE LabelFileRelation(label_id INTEGER,media_id INTEGER,PRIMARY KEY(label_id,media_id),FOREIGN KEY(label_id) REFERENCES Label(id_label) ON DELETE CASCADE,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Playlist(id_playlist INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE,file_id UNSIGNED INT DEFAULT NULL,creation_date UNSIGNED INT NOT NULL,artwork_mrl TEXT,FOREIGN KEY(file_id) REFERENCES File(id_file) ON DELETE CASCADE);
CREATE VIRTUAL TABLE PlaylistFts USING FTS3(name);
CREATE TABLE PlaylistMediaRelation(media_id INTEGER,mrl STRING,playlist_id INTEGER,position INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE SET NULL,FOREIGN KEY(playlist_id) REFERENCES Playlist(id_playlist) ON DELETE CASCADE);
CREATE TABLE Genre(id_genre INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,nb_tracks INTEGER NOT NULL DEFAULT 0);
CREATE VIRTUAL TABLE GenreFts USING FTS3(name);
CREATE TABLE Album(id_album INTEGER PRIMARY KEY AUTOINCREMENT,title TEXT COLLATE NOCASE,artist_id UNSIGNED INTEGER,release_year UNSIGNED INTEGER,short_summary TEXT,nb_tracks UNSIGNED INTEGER DEFAULT 0,duration UNSIGNED INTEGER NOT NULL DEFAULT 0,nb_discs UNSIGNED INTEGER NOT NULL DEFAULT 1,is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_tracks),FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE);
CREATE VIRTUAL TABLE AlbumFts USING FTS3(title,artist);
CREATE TABLE AlbumTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,media_id INTEGER UNIQUE,duration INTEGER NOT NULL,artist_id UNSIGNED INTEGER,genre_id INTEGER,track_number UNSIGNED INTEGER,album_id UNSIGNED INTEGER NOT NULL,disc_number UNSIGNED INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE,FOREIGN KEY(genre_id) REFERENCES Genre(id_genre),FOREIGN KEY(album_id) REFERENCES Album(id_album)  ON DELETE CASCADE);
CREATE TABLE Show(id_show INTEGER PRIMARY KEY AUTOINCREMENT,title TEXT,release_date UNSIGNED INTEGER,short_summary TEXT,artwork_mrl TEXT,tvdb_id TEXT);
CREATE VIRTUAL TABLE ShowFts USING FTS3(title);
CREATE TABLE ShowEpisode(id_episode INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INTEGER NOT NULL,episode_number UNSIGNED INT,season_number UNSIGNED INT,episode_summary TEXT,tvdb_id TEXT,show_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(show_id) REFERENCES Show(id_show) ON DELETE CASCADE);
CREATE TABLE Movie(id_movie INTEGER PRIMARY KEY AUTOINCREMENT,media_id UNSIGNED INTEGER NOT NULL,summary TEXT,imdb_id TEXT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE INDEX movie_media_idx ON Movie(media_id);
CREATE TABLE VideoTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,width UNSIGNED INTEGER,height UNSIGNED INTEGER,fps_num UNSIGNED INTEGER,fps_den UNSIGNED INTEGER,bitrate UNSIGNED INTEGER,sar_num UNSIGNED INTEGER,sar_den UNSIGNED INTEGER,media_id UNSIGNED INT,language TEXT,description TEXT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE AudioTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,bitrate UNSIGNED INTEGER,samplerate UNSIGNED INTEGER,nb_channels UNSIGNED INTEGER,language TEXT,description TEXT,media_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Artist(id_artist INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT COLLATE NOCASE UNIQUE ON CONFLICT FAIL,shortbio TEXT,nb_albums UNSIGNED INT DEFAULT 0,nb_tracks UNSIGNED INT DEFAULT 0,mb_id TEXT,is_present UNSIGNED INTEGER NOT NULL DEFAULT 0 CHECK(is_present <= nb_tracks));
CREATE VIRTUAL TABLE ArtistFts USING FTS3(name);
CREATE TABLE MediaArtistRelation(media_id INTEGER NOT NULL,artist_id INTEGER,PRIMARY KEY(media_id,artist_id),FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE,FOREIGN KEY(artist_id) REFERENCES Artist(id_artist) ON DELETE CASCADE);
CREATE TABLE Task(id_task INTEGER PRIMARY KEY AUTOINCREMENT,step INTEGER NOT NULL DEFAULT 0,retry_count INTEGER NOT NULL DEFAULT 0,type INTEGER NOT NULL,mrl TEXT,file_type INTEGER NOT NULL,file_id UNSIGNED INTEGER,parent_folder_id UNSIGNED INTEGER,link_to_id UNSIGNED INTEGER NOT NULL,link_to_type UNSIGNED INTEGER NOT NULL,link_extra UNSIGNED INTEGER NOT NULL,UNIQUE(mrl,type, link_to_id, link_to_type, link_extra) ON CONFLICT FAIL,FOREIGN KEY(parent_folder_id) REFERENCES Folder(id_folder) ON DELETE CASCADE,FOREIGN KEY(file_id) REFERENCES File(id_file) ON DELETE CASCADE);
CREATE TABLE Metadata(id_media INTEGER,entity_type INTEGER,type INTEGER,value TEXT,PRIMARY KEY(id_media,entity_type,type));
CREATE TABLE SubtitleTrack(id_track INTEGER PRIMARY KEY AUTOINCREMENT,codec TEXT,language TEXT,description TEXT,encoding TEXT,media_id UNSIGNED INT,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Chapter(id_chapter INTEGER PRIMARY KEY AUTOINCREMENT,offset INTEGER NOT NULL,duration INTEGER NOT NULL,name TEXT,media_id INTEGER,FOREIGN KEY(media_id) REFERENCES Media(id_media) ON DELETE CASCADE);
CREATE TABLE Bookmark(id_bookmark INTEGER PRIMARY KEY AUTOINCREMENT,time UNSIGNED INTEGER NOT NULL,name TEXT,description TEXT,media_id UNSIGNED INTEGER NOT NULL,FOREIGN KEY(media_id) REFERENCES Media(id_media),UNIQUE(time,media_id) ON CONFLICT FAIL);
CREATE INDEX folder_device_id_idx ON Folder (device_id);
CREATE INDEX parent_folder_id_idx ON Folder (parent_id);
CREATE TRIGGER insert_folder_fts AFTER INSERT ON Folder BEGIN INSERT INTO FolderFts(rowid,name) VALUES(new.id_folder,new.name);END;
CREATE TRIGGER delete_folder_fts BEFORE DELETE ON Folder BEGIN DELETE FROM FolderFts WHERE rowid = old.id_folder;END;
CREATE TRIGGER update_folder_nb_media_on_insert AFTER INSERT ON Media WHEN new.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE new.type WHEN 1 THEN 1 ELSE 0 END) WHERE id_folder = new.folder_id;END;
CREATE TRIGGER update_folder_nb_media_on_update AFTER UPDATE ON Media WHEN new.folder_id IS NOT NULL AND old.type != new.type BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE old.type WHEN 2 THEN -1 ELSE 0 END)+(CASE new.type WHEN 2 THEN 1 ELSE 0 END),nb_video = nb_video + (CASE old.type WHEN 1 THEN -1 ELSE 0 END)+(CASE new.type WHEN 1 THEN 1 ELSE 0 END)WHERE id_folder = new.folder_id;END;
CREATE TRIGGER update_folder_nb_media_on_delete AFTER DELETE ON Media WHEN old.folder_id IS NOT NULL BEGIN UPDATE Folder SET nb_audio = nb_audio + (CASE old.type WHEN 2 THEN -1 ELSE 0 END),nb_video = nb_video + (CASE old.type WHEN 1 THEN -1 ELSE 0 END) WHERE id_folder = old.folder_id;END;
CREATE INDEX album_artist_id_idx ON Album(artist_id);
CREATE TRIGGER is_album_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 BEGIN  UPDATE Album SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_album = (SELECT album_id FROM AlbumTrack WHERE media_id = new.id_media); END;
CREATE TRIGGER delete_album_track AFTER DELETE ON AlbumTrack BEGIN  UPDATE Album SET nb_tracks = nb_tracks - 1, is_present = is_present - 1, duration = duration - old.duration WHERE id_album = old.album_id; DELETE FROM Album WHERE id_album=old.album_id AND nb_tracks = 0; END;
CREATE TRIGGER add_album_track AFTER INSERT ON AlbumTrack BEGIN UPDATE Album SET duration = duration + new.duration, nb_tracks = nb_tracks + 1, is_present = is_present + 1 WHERE id_album = new.album_id; END;
CREATE TRIGGER insert_album_fts AFTER INSERT ON Album WHEN new.title IS NOT NULL BEGIN INSERT INTO AlbumFts(rowid, title) VALUES(new.id_album, new.title); END;
CREATE TRIGGER delete_album_fts BEFORE DELETE ON Album WHEN old.title IS NOT NULL BEGIN DELETE FROM AlbumFts WHERE rowid = old.id_album; END;
CREATE INDEX album_media_artist_genre_album_idx ON AlbumTrack(media_id, artist_id, genre_id, album_id);
CREATE INDEX album_track_album_genre_artist_ids ON AlbumTrack(album_id, genre_id, artist_id);
CREATE TRIGGER has_tracks_present AFTER UPDATE OF is_present ON Media WHEN new.subtype = 3 BEGIN  UPDATE Artist SET is_present=is_present + (CASE new.is_present WHEN 0 THEN -1 ELSE 1 END)WHERE id_artist = (SELECT artist_id FROM AlbumTrack  WHERE media_id = new.id_media ); END;
CREATE TRIGGER has_album_remaining AFTER DELETE ON Album WHEN old.artist_id != 1 AND  old.artist_id != 2 BEGIN UPDATE Artist SET nb_albums = nb_albums - 1 WHERE id_artist = old.artist_id; DELETE FROM Artist WHERE id_artist = old.artist_id  AND nb_albums = 0  AND nb_tracks = 0; END;
CREATE TRIGGER has_track_remaining AFTER DELETE ON AlbumTrack WHEN old.artist_id != 1 AND  old.artist_id != 2 BEGIN UPDATE Artist SET nb_tracks = nb_tracks - 1, is_present = is_present - 1 WHERE id_artist = old.artist_id; DELETE FROM Artist WHERE id_artist = old.artist_id  AND nb_albums = 0  AND nb_tracks = 0; END;
CREATE TRIGGER insert_artist_fts AFTER INSERT ON Artist WHEN new.name IS NOT NULL BEGIN INSERT INTO ArtistFts(rowid,name) VALUES(new.id_artist, new.name); END;
CREATE TRIGGER delete_artist_fts BEFORE DELETE ON Artist WHEN old.name IS NOT NULL BEGIN DELETE FROM ArtistFts WHERE rowid=old.id_artist; END;
CREATE INDEX index_last_played_date ON Media(last_played_date DESC);
CREATE INDEX index_media_presence ON Media(is_present);
CREATE TRIGGER is_media_device_present AFTER UPDATE OF is_present ON Device BEGIN UPDATE Media SET is_present=new.is_present WHERE device_id=new.id_device;END;
CREATE TRIGGER cascade_file_deletion AFTER DELETE ON File BEGIN  DELETE FROM Media WHERE (SELECT COUNT(id_file) FROM File WHERE media_id=old.media_id) = 0 AND id_media=old.media_id; END;
CREATE TRIGGER insert_media_fts AFTER INSERT ON Media BEGIN INSERT INTO MediaFts(rowid,title,labels) VALUES(new.id_media, new.title, ''); END;
CREATE TRIGGER delete_media_fts BEFORE DELETE ON Media BEGIN DELETE FROM MediaFts WHERE rowid = old.id_media; END;
CREATE TRIGGER update_media_title_fts AFTER UPDATE OF title ON Media BEGIN UPDATE MediaFts SET title = new.title WHERE rowid = new.id_media; END;
CREATE INDEX media_types_idx ON Media(type, subtype);
CREATE TRIGGER increment_media_nb_playlist AFTER INSERT ON PlaylistMediaRelation BEGIN  UPDATE Media SET nb_playlists = nb_playlists + 1  WHERE id_media = new.media_id; END;
CREATE TRIGGER decrement_media_nb_playlist AFTER DELETE ON PlaylistMediaRelation BEGIN  UPDATE Media SET nb_playlists = nb_playlists - 1  WHERE id_media = old.media_id; END;
CREATE INDEX media_last_usage_dates_idx ON Media(last_played_date, real_last_played_date, insertion_date);
CREATE INDEX media_folder_id_idx ON Media(folder_id);
CREATE INDEX file_media_id_index ON File(media_id);
CREATE INDEX file_folder_id_index ON File(folder_id);
CREATE TRIGGER insert_genre_fts AFTER INSERT ON Genre BEGIN INSERT INTO GenreFts(rowid,name) VALUES(new.id_genre, new.name); END;
CREATE TRIGGER delete_genre_fts BEFORE DELETE ON Genre BEGIN DELETE FROM GenreFts WHERE rowid = old.id_genre; END;
CREATE TRIGGER update_genre_on_new_track AFTER INSERT ON AlbumTrack WHEN new.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks + 1 WHERE id_genre = new.genre_id; END;
CREATE TRIGGER update_genre_on_track_deleted AFTER DELETE ON AlbumTrack WHEN old.genre_id IS NOT NULL BEGIN UPDATE Genre SET nb_tracks = nb_tracks - 1 WHERE id_genre = old.genre_id; DELETE FROM Genre WHERE nb_tracks = 0; END;
CREATE TRIGGER update_playlist_order_on_insert AFTER INSERT ON PlaylistMediaRelation WHEN new.position IS NOT NULL BEGIN UPDATE PlaylistMediaRelation SET position = position + 1 WHERE playlist_id = new.playlist_id AND position >= new.position AND rowid != new.rowid; END;
CREATE TRIGGER update_playlist_order_on_delete AFTER DELETE ON PlaylistMediaRelation BEGIN UPDATE PlaylistMediaRelation SET position = position - 1 WHERE playlist_id = old.playlist_id AND position > old.position; END;
CREATE TRIGGER insert_playlist_fts AFTER INSERT ON Playlist BEGIN INSERT INTO PlaylistFts(rowid, name) VALUES(new.id_playlist, new.name); END;
CREATE TRIGGER update_playlist_fts AFTER UPDATE OF name ON Playlist BEGIN UPDATE PlaylistFts SET name = new.name WHERE rowid = new.id_playlist; END;
CREATE TRIGGER delete_playlist_fts BEFORE DELETE ON Playlist BEGIN DELETE FROM PlaylistFts WHERE rowid = old.id_playlist; END;
CREATE INDEX playlist_position_pl_id_index ON PlaylistMediaRelation(playlist_id, position);
CREATE INDEX playlist_file_id ON Playlist(file_id);
CREATE TRIGGER delete_label_fts BEFORE DELETE ON Label BEGIN UPDATE MediaFts SET labels = TRIM(REPLACE(labels, old.name, '')) WHERE labels MATCH old.name; END;
CREATE TRIGGER insert_show_fts AFTER INSERT ON Show BEGIN INSERT INTO ShowFts(rowid,title) VALUES(new.id_show, new.title); END;
CREATE TRIGGER delete_show_fts BEFORE DELETE ON Show BEGIN DELETE FROM ShowFts WHERE rowid = old.id_show; END;
CREATE INDEX show_episode_media_show_idx ON ShowEpisode(media_id, show_id);
CREATE TRIGGER auto_delete_album_thumbnail AFTER DELETE ON Album BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_album AND entity_type = 1; END;
CREATE TRIGGER auto_delete_artist_thumbnail AFTER DELETE ON Artist BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_artist AND entity_type = 2; END;
CREATE TRIGGER auto_delete_media_thumbnail AFTER DELETE ON Media BEGIN DELETE FROM ThumbnailLinking WHERE entity_id = old.id_media AND entity_type = 0; END;
CREATE TRIGGER incr_thumbnail_refcount AFTER INSERT ON ThumbnailLinking BEGIN UPDATE Thumbnail SET shared_counter = shared_counter + 1 WHERE id_thumbnail = new.thumbnail_id;END;
CREATE TRIGGER decr_thumbnail_refcount AFTER DELETE ON ThumbnailLinking BEGIN UPDATE Thumbnail SET shared_counter = shared_counter - 1 WHERE id_thumbnail = old.thumbnail_id;END;
CREATE TRIGGER update_thumbnail_refcount AFTER UPDATE OF thumbnail_id ON ThumbnailLinking WHEN old.thumbnail_id != new.thumbnail_id BEGIN UPDATE Thumbnail SET shared_counter = shared_counter - 1 WHERE id_thumbnail = old.thumbnail_id;UPDATE Thumbnail SET shared_counter = shared_counter + 1 WHERE id_thumbnail = new.thumbnail_id;END;
CREATE TRIGGER delete_unused_thumbnail AFTER UPDATE OF shared_counter ON Thumbnail WHEN new.shared_counter = 0 BEGIN DELETE FROM Thumbnail WHERE id_thumbnail = new.id_thumbnail;END;
CREATE INDEX thumbnail_link_index ON Thumbnail(id_thumbnail);
CREATE TRIGGER delete_playlist_linking_tasks AFTER DELETE ON Playlist BEGIN DELETE FROM Task WHERE link_to_type = 1 AND link_to_id = old.id_playlist AND type = 1;END;
CREATE INDEX audio_track_media_idx ON AudioTrack(media_id);
CREATE INDEX subtitle_track_media_idx  ON SubtitleTrack(media_id);
CREATE INDEX video_track_media_idx ON VideoTrack(media_id);
INSERT INTO Settings VALUES(22,6,1);
INSERT INTO Device VALUES(1,'49665d40-73ae-4134-b2f6-b421411b9508','file://',0,1,0);
INSERT INTO Folder VALUES(1,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/','multi_cd',NULL,0,1,0,0,0);
INSERT INTO Folder VALUES(2,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder3/','folder3',1,0,1,0,1,0);
INSERT INTO Folder VALUES(3,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder2/','folder2',1,0,1,0,1,0);
INSERT INTO Folder VALUES(4,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder1/','folder1',1,0,1,0,1,0);
INSERT INTO Media VALUES(1,2,3,10057,NULL,NULL,NULL,1571660066,2000,'Track 3','track3.mp3',0,1,1,0,2);
INSERT INTO Media VALUES(2,2,3,10057,NULL,NULL,NULL,1571660066,2000,'Track 2','track2.mp3',0,1,1,0,3);
INSERT INTO Media VALUES(3,2,3,10057,NULL,NULL,NULL,1571660066,2000,'Track 1','track1.mp3',0,1,1,0,4);
INSERT INTO File VALUES(1,1,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder3/track3.mp3',1,1531495970,139264,2,0,0,0);
INSERT INTO File VALUES(2,2,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder2/track2.mp3',1,1531495970,139264,3,0,0,0);
INSERT INTO File VALUES(3,3,NULL,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder1/track1.mp3',1,1531495970,139264,4,0,0,0);
INSERT INTO Album VALUES(1,'Album',3,2000,NULL,3,30171,3,3);
INSERT INTO AlbumTrack VALUES(1,1,10057,3,NULL,0,1,3);
INSERT INTO AlbumTrack VALUES(2,2,10057,3,NULL,0,1,2);
INSERT INTO AlbumTrack VALUES(3,3,10057,3,NULL,0,1,1);
INSERT INTO AudioTrack VALUES(1,'mpga',128000,44100,2,'','',1);
INSERT INTO AudioTrack VALUES(2,'mpga',128000,44100,2,'','',2);
INSERT INTO AudioTrack VALUES(3,'mpga',128000,44100,2,'','',3);
INSERT INTO Artist VALUES(1,NULL,NULL,0,0,NULL,0);
INSERT INTO Artist VALUES(2,NULL,NULL,0,0,NULL,0);
INSERT INTO Artist VALUES(3,'Artist',NULL,1,3,NULL,3);
INSERT INTO MediaArtistRelation VALUES(1,3);
INSERT INTO MediaArtistRelation VALUES(2,3);
INSERT INTO MediaArtistRelation VALUES(3,3);
INSERT INTO Task VALUES(1,3,0,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder3/track3.mp3',1,1,2,0,0,0);
INSERT INTO Task VALUES(2,3,0,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder2/track2.mp3',1,2,3,0,0,0);
INSERT INTO Task VALUES(3,3,0,0,'file:///home/chouquette/dev/medialibrary/test/samples/samples/music/multi_cd/folder1/track1.mp3',1,3,4,0,0,0);
COMMIT;
