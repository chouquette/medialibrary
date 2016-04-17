ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wsign-compare -Wextra -Wstrict-aliasing -Wstrict-overflow \
			  -Wformat=2 -Wno-unused-parameter -Wcast-align -Wpointer-arith \
			  -Wwrite-strings -Wlogical-op \
			  -pipe

MEDIALIB_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src

libmedialibrary_ladir = $(includedir)/medialibrary

libmedialibrary_la_HEADERS = \
	include/medialibrary/IAlbum.h \
	include/medialibrary/IAlbumTrack.h \
	include/medialibrary/IArtist.h \
	include/medialibrary/IAudioTrack.h \
	include/medialibrary/IFile.h \
	include/medialibrary/IGenre.h \
	include/medialibrary/IHistoryEntry.h \
	include/medialibrary/ILabel.h \
	include/medialibrary/ILogger.h \
	include/medialibrary/IMedia.h \
	include/medialibrary/IMediaLibrary.h \
	include/medialibrary/IMovie.h \
	include/medialibrary/IPlaylist.h \
	include/medialibrary/IShowEpisode.h \
	include/medialibrary/IShow.h \
	include/medialibrary/IVideoTrack.h \
	include/medialibrary/Types.h \
	$(NULL)

libmedialibrary_la_SOURCES = \
	src/Album.cpp \
	src/AlbumTrack.cpp \
	src/Artist.cpp \
	src/AudioTrack.cpp \
	src/Device.cpp \
	src/Factory.cpp \
	src/File.cpp \
	src/Folder.cpp \
	src/Genre.cpp \
	src/History.cpp \
	src/Label.cpp \
	src/Media.cpp \
	src/MediaLibrary.cpp \
	src/Movie.cpp \
	src/Playlist.cpp \
	src/Settings.cpp \
	src/Show.cpp \
	src/ShowEpisode.cpp \
	src/VideoTrack.cpp \
	src/database/SqliteConnection.cpp \
	src/database/SqliteTools.cpp \
	src/database/SqliteTransaction.cpp \
	src/discoverer/DiscovererWorker.cpp \
	src/discoverer/FsDiscoverer.cpp \
	src/factory/FileSystem.cpp \
	src/filesystem/common/CommonFile.cpp \
	src/logging/IostreamLogger.cpp \
	src/logging/Logger.cpp \
	src/metadata_services/MetadataParser.cpp \
	src/metadata_services/vlc/VLCMetadataService.cpp \
	src/metadata_services/vlc/VLCThumbnailer.cpp \
	src/parser/Parser.cpp \
	src/parser/ParserService.cpp \
	src/utils/Filename.cpp \
	src/utils/ModificationsNotifier.cpp \
	src/utils/VLCInstance.cpp \
	$(NULL)

noinst_HEADERS = \
	include/discoverer/IDiscoverer.h \
	include/factory/IFileSystem.h \
	include/filesystem/IDevice.h \
	include/filesystem/IDirectory.h \
	include/filesystem/IFile.h \
	include/Fixup.h \
	include/medialibrary/IAlbum.h \
	include/medialibrary/IAlbumTrack.h \
	include/medialibrary/IArtist.h \
	include/medialibrary/IAudioTrack.h \
	include/medialibrary/IFile.h \
	include/medialibrary/IGenre.h \
	include/medialibrary/IHistoryEntry.h \
	include/medialibrary/ILabel.h \
	include/medialibrary/ILogger.h \
	include/medialibrary/IMedia.h \
	include/medialibrary/IMediaLibrary.h \
	include/medialibrary/IMovie.h \
	include/medialibrary/IPlaylist.h \
	include/medialibrary/IShowEpisode.h \
	include/medialibrary/IShow.h \
	include/medialibrary/IVideoTrack.h \
	include/medialibrary/Types.h \
	include/Types.h \
	src/Album.h \
	src/AlbumTrack.h \
	src/Artist.h \
	src/AudioTrack.h \
	src/database/DatabaseHelpers.h \
	src/database/SqliteConnection.h \
	src/database/SqliteErrors.h \
	src/database/SqliteTools.h \
	src/database/SqliteTraits.h \
	src/database/SqliteTransaction.h \
	src/Device.h \
	src/discoverer/DiscovererWorker.h \
	src/discoverer/FsDiscoverer.h \
	src/factory/FileSystem.h \
	src/File.h \
	src/filesystem/common/CommonFile.h \
	src/filesystem/unix/Device.h \
	src/filesystem/unix/Directory.h \
	src/filesystem/unix/File.h \
	src/filesystem/win32/Directory.h \
	src/filesystem/win32/File.h \
	src/Folder.h \
	src/Genre.h \
	src/History.h \
	src/Label.h \
	src/logging/IostreamLogger.h \
	src/logging/Logger.h \
	src/Media.h \
	src/MediaLibrary.h \
	src/metadata_services/MetadataParser.h \
	src/metadata_services/vlc/VLCMetadataService.h \
	src/metadata_services/vlc/VLCThumbnailer.h \
	src/Movie.h \
	src/parser/Parser.h \
	src/parser/ParserService.h \
	src/parser/Task.h \
	src/Playlist.h \
	src/Settings.h \
	src/ShowEpisode.h \
	src/Show.h \
	src/utils/Cache.h \
	src/utils/Filename.h \
	src/utils/ModificationsNotifier.h \
	src/utils/SWMRLock.h \
	src/utils/VLCInstance.h \
	src/VideoTrack.h \
	$(NULL)


if HAVE_WIN32
libmedialibrary_la_SOURCES += \
	src/filesystem/win32/Directory.cpp \
	src/filesystem/win32/File.cpp
else
libmedialibrary_la_SOURCES += \
	src/filesystem/unix/Device.cpp \
	src/filesystem/unix/Directory.cpp \
	src/filesystem/unix/File.cpp
endif

libmedialibrary_la_LDFLAGS = -no-undefined

libmedialibrary_la_CPPFLAGS = \
	$(MEDIALIB_CPPFLAGS) \
	$(SQLITE_CFLAGS) \
	$(VLC_CFLAGS) \
	$(VLCPP_CFLAGS) \
	$(NULL)

libmedialibrary_la_LIBADD = \
	$(SQLITE_LIBS) 			\
	$(VLC_LIBS) 			\
	$(LIBJPEG_LIBS)			\
	$(NULL)

lib_LTLIBRARIES = libmedialibrary.la

if HAVE_TESTS

check_PROGRAMS = unittest samples

lib_LTLIBRARIES += libgtest.la libgtestmain.la

libgtest_la_SOURCES = \
	googletest/src/gtest-all.cc

libgtestmain_la_SOURCES = \
	googletest/src/gtest_main.cc

libgtest_la_CPPFLAGS = \
	-I$(top_srcdir)/googletest

if HAVE_PTHREAD
	libgtest_la_CPPFLAGS += -DGTEST_HAS_PTHREAD=1
	libgtest_la_CXXFLAGS = $(PTHREAD_CFLAGS)
	libgtest_la_LIDADD = $(PTHREAD_LIBS)
else
	libgtest_la_CPPFLAGS += -DGTEST_HAS_PTHREAD=0
endif

libgtestmain_la_CPPFLAGS = $(libgtest_la_CPPFLAGS)
libgtestmain_la_CXXFLAGS = $(libgtest_la_CXXFLAGS)
libgtestmain_la_LIBADD   = $(libgtest_la_LIBADD)

unittest_SOURCES = \
	test/common/MediaLibraryTester.cpp \
	test/mocks/FileSystem.cpp \
	test/mocks/filesystem/MockDevice.cpp \
	test/mocks/filesystem/MockDirectory.cpp \
	test/mocks/filesystem/MockFile.cpp \
	test/unittest/AlbumTests.cpp \
	test/unittest/AlbumTrackTests.cpp \
	test/unittest/ArtistTests.cpp \
	test/unittest/AudioTrackTests.cpp \
	test/unittest/DeviceTests.cpp \
	test/unittest/FileTests.cpp \
	test/unittest/FolderTests.cpp \
	test/unittest/FsUtilsTests.cpp \
	test/unittest/GenreTests.cpp \
	test/unittest/HistoryTests.cpp \
	test/unittest/LabelTests.cpp \
	test/unittest/MediaTests.cpp \
	test/unittest/MovieTests.cpp \
	test/unittest/PlaylistTests.cpp \
	test/unittest/RemovalNotifierTests.cpp \
	test/unittest/ShowTests.cpp \
	test/unittest/Tests.cpp \
	test/unittest/VideoTrackTests.cpp \
	$(NULL)

unittest_CXXFLAGS = 		\
	$(MEDIALIB_CPPFLAGS) 	\
	-I$(top_srcdir)/test	\
	$(NULL)

unittest_LDADD = 		\
	libmedialibrary.la 	\
	libgtest.la			\
	libgtestmain.la		\
	$(PTHREAD_LIBS) 	\
	$(SQLITE_LIBS)		\
	$(NULL)

samples_SOURCES = 						\
	test/common/MediaLibraryTester.cpp 	\
	test/samples/main.cpp 				\
	test/samples/Tester.cpp 			\
	$(NULL)

samples_CXXFLAGS = 			\
	$(MEDIALIB_CPPFLAGS)	\
	-DSRC_DIR=\"$(top_srcdir)\"	\
	-I$(top_srcdir)/test	\
	$(NULL)

samples_LDADD = 		\
	libmedialibrary.la	\
	libgtest.la			\
	libgtestmain.la		\
	$(PTHREAD_LIBS) 	\
	$(SQLITE_LIBS)		\
	$(NULL)

endif

