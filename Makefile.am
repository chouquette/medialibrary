ACLOCAL_AMFLAGS = -I m4

AM_CXXFLAGS = -Wall -Wsign-compare -Wextra -Wstrict-aliasing -Wstrict-overflow \
			  -Wformat=2 -Wno-unused-parameter -Wcast-align -Wpointer-arith \
			  -Wwrite-strings \
			  -pipe

MEDIALIB_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src

libmedialibrary_ladir = $(includedir)/medialibrary

libmedialibrary_la_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-fvisibility=default \
	$(NULL)

libmedialibrary_la_CPPFLAGS = \
	$(MEDIALIB_CPPFLAGS) \
	$(SQLITE_CFLAGS) \
	$(NULL)

libmedialibrary_la_HEADERS = \
	include/medialibrary/IAlbum.h \
	include/medialibrary/IAlbumTrack.h \
	include/medialibrary/IArtist.h \
	include/medialibrary/IAudioTrack.h \
	include/medialibrary/IBookmark.h \
	include/medialibrary/IChapter.h \
	include/medialibrary/IFile.h \
	include/medialibrary/IGenre.h \
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
	include/medialibrary/IDeviceLister.h \
	include/medialibrary/IFolder.h \
	include/medialibrary/IQuery.h \
	include/medialibrary/IMetadata.h \
	include/medialibrary/ISubtitleTrack.h \
	include/medialibrary/IThumbnailer.h \
	include/medialibrary/IInterruptProbe.h \
	include/medialibrary/IVideoGroup.h \
	$(NULL)

libmedialibraryfsdir = $(libmedialibrary_ladir)/filesystem
libmedialibraryfs_HEADERS = \
	include/medialibrary/filesystem/IDevice.h \
	include/medialibrary/filesystem/IDirectory.h \
	include/medialibrary/filesystem/IFile.h \
	include/medialibrary/filesystem/IFileSystemFactory.h \
	include/medialibrary/filesystem/Errors.h \
	$(NULL)

libmedialibraryparserdir = $(libmedialibrary_ladir)/parser
libmedialibraryparser_HEADERS = \
	include/medialibrary/parser/IItem.h \
	include/medialibrary/parser/Parser.h \
	include/medialibrary/parser/IParserService.h \
	$(NULL)

libmedialibrary_la_SOURCES = \
	src/Album.cpp \
	src/AlbumTrack.cpp \
	src/Artist.cpp \
	src/AudioTrack.cpp \
	src/Device.cpp \
	src/Factory.cpp \
	src/File.cpp \
	src/Chapter.cpp \
	src/Folder.cpp \
	src/Genre.cpp \
	src/Label.cpp \
	src/Media.cpp \
	src/MediaLibrary.cpp \
	src/Movie.cpp \
	src/Playlist.cpp \
	src/Settings.cpp \
	src/Show.cpp \
	src/Thumbnail.cpp \
	src/ShowEpisode.cpp \
	src/VideoTrack.cpp \
	src/Metadata.cpp \
	src/SubtitleTrack.cpp \
	src/database/SqliteConnection.cpp \
	src/database/SqliteTools.cpp \
	src/database/SqliteTransaction.cpp \
	src/discoverer/DiscovererWorker.cpp \
	src/discoverer/FsDiscoverer.cpp \
	src/discoverer/probe/PathProbe.cpp \
	src/factory/FileSystemFactory.cpp \
	src/factory/DeviceListerFactory.cpp \
	src/filesystem/common/CommonDevice.cpp \
	src/filesystem/common/CommonFile.cpp \
	src/filesystem/common/CommonDirectory.cpp \
	src/logging/IostreamLogger.cpp \
	src/logging/Logger.cpp \
	src/metadata_services/MetadataParser.cpp \
	src/metadata_services/LinkService.cpp \
	src/thumbnails/ThumbnailerWorker.cpp \
	src/parser/Parser.cpp \
	src/parser/ParserWorker.cpp \
	src/parser/Task.cpp \
	src/utils/Directory.cpp \
	src/utils/Filename.cpp \
	src/utils/ModificationsNotifier.cpp \
	src/utils/Strings.cpp \
	src/utils/Url.cpp \
	src/utils/File.cpp \
	src/utils/TitleAnalyzer.cpp \
	src/Bookmark.cpp \
	src/VideoGroup.cpp \
	$(NULL)


noinst_HEADERS = \
	include/discoverer/IDiscoverer.h \
	include/Fixup.h \
	include/Types.h \
	include/Common.h \
	src/Album.h \
	src/AlbumTrack.h \
	src/Artist.h \
	src/AudioTrack.h \
	src/database/DatabaseHelpers.h \
	src/database/SqliteConnection.h \
	src/database/SqliteErrors.h \
	src/database/SqliteQuery.h \
	src/database/SqliteTools.h \
	src/database/SqliteTraits.h \
	src/database/SqliteTransaction.h \
	src/Device.h \
	src/discoverer/DiscovererWorker.h \
	src/discoverer/FsDiscoverer.h \
	src/discoverer/probe/CrawlerProbe.h \
	src/discoverer/probe/IProbe.h \
	src/discoverer/probe/PathProbe.h \
	src/factory/FileSystemFactory.h \
	src/factory/DeviceListerFactory.h \
	src/File.h \
	src/filesystem/common/CommonFile.h \
	src/filesystem/common/CommonDirectory.h \
	src/filesystem/common/CommonDevice.h \
	src/filesystem/darwin/DeviceLister.h \
	src/filesystem/unix/Device.h \
	src/filesystem/unix/Directory.h \
	src/filesystem/unix/File.h \
	src/filesystem/unix/DeviceLister.h \
	src/filesystem/win32/Directory.h \
	src/filesystem/win32/File.h \
	src/Chapter.h \
	src/Folder.h \
	src/Genre.h \
	src/Label.h \
	src/logging/IostreamLogger.h \
	src/logging/Logger.h \
	src/Media.h \
	src/MediaLibrary.h \
	src/metadata_services/MetadataParser.h \
	src/metadata_services/LinkService.h \
	src/thumbnails/ThumbnailerWorker.h \
	src/Movie.h \
	src/parser/Parser.h \
	src/parser/ParserWorker.h \
	src/parser/Task.h \
	src/Playlist.h \
	src/Settings.h \
	src/ShowEpisode.h \
	src/Show.h \
	src/Thumbnail.h \
	src/utils/Directory.h \
	src/utils/Filename.h \
	src/utils/ModificationsNotifier.h \
	src/utils/Strings.h \
	src/utils/SWMRLock.h \
	src/utils/Url.h \
	src/utils/File.h \
	src/utils/TitleAnalyzer.h \
	src/VideoTrack.h \
	src/Metadata.h \
	src/compat/Thread.h \
	src/compat/Mutex.h \
	src/compat/ConditionVariable.h \
	src/SubtitleTrack.h \
	src/Bookmark.h \
	src/VideoGroup.h \
	$(NULL)


if HAVE_WIN32
libmedialibrary_la_SOURCES += \
	src/filesystem/win32/Directory.cpp \
	src/filesystem/win32/File.cpp \
	src/filesystem/win32/DeviceLister.cpp \
	$(NULL)
noinst_HEADERS += \
	src/utils/Charsets.h \
	$(NULL)
else
libmedialibrary_la_SOURCES += \
	src/filesystem/unix/Directory.cpp \
	src/filesystem/unix/File.cpp \
	$(NULL)
if HAVE_LINUX
if !HAVE_ANDROID
libmedialibrary_la_SOURCES += \
	src/filesystem/unix/DeviceLister.cpp \
	$(NULL)
endif
endif
if HAVE_DARWIN
libmedialibrary_macos_la_CPPFLAGS = $(libmedialibrary_la_CPPFLAGS)
libmedialibrary_macos_la_SOURCES = \
	src/filesystem/darwin/DeviceLister.mm \
	$(NULL)
endif
endif

libmedialibrary_la_LIBADD = \
	$(SQLITE_LIBS) 			\
	$(THREAD_LIBS)			\
	$(NULL)

libmedialibrary_la_LDFLAGS = -no-undefined -export-symbols-regex 'NewMediaLibrary'
if HAVE_DARWIN
libmedialibrary_la_LIBADD += libmedialibrary_macos.la
libmedialibrary_la_LDFLAGS += -Wl,-undefined,dynamic_lookup,-framework,Foundation
endif

lib_LTLIBRARIES = libmedialibrary.la
if HAVE_DARWIN
lib_LTLIBRARIES += libmedialibrary_macos.la
endif

if HAVE_LIBVLC

if HAVE_LIBVLC4
libmedialibrary_la_SOURCES += \
	src/thumbnails/CoreThumbnailer.cpp \
	src/thumbnails/CoreThumbnailer.h

else
libmedialibrary_la_SOURCES += \
	src/thumbnails/VmemThumbnailer.cpp \
	src/thumbnails/VmemThumbnailer.h

if HAVE_LIBJPEG
libmedialibrary_la_SOURCES += src/thumbnails/imagecompressors/JpegCompressor.cpp
noinst_HEADERS += src/thumbnails/imagecompressors/JpegCompressor.h
libmedialibrary_la_CPPFLAGS += $(LIBJPEG_CFLAGS)
libmedialibrary_la_LIBADD += $(LIBJPEG_LIBS)
endif

endif

libmedialibrary_la_SOURCES += \
	src/metadata_services/vlc/VLCMetadataService.cpp \
	src/metadata_services/vlc/Common.cpp \
	src/utils/VLCInstance.cpp \
	src/factory/NetworkFileSystemFactory.cpp \
	src/filesystem/network/Directory.cpp \
	src/filesystem/network/File.cpp \
	src/filesystem/network/Device.cpp \
	$(NULL)

noinst_HEADERS += \
	src/utils/VLCInstance.h \
    src/metadata_services/vlc/Common.hpp \
	src/metadata_services/vlc/VLCMetadataService.h \
	src/thumbnails/imagecompressors/IImageCompressor.h \
	src/factory/NetworkFileSystemFactory.h \
	src/filesystem/network/Device.h \
	src/filesystem/network/Directory.h \
	src/filesystem/network/File.h \
	$(NULL)

libmedialibrary_la_CPPFLAGS += \
	$(VLC_CFLAGS) \
	$(VLCPP_CFLAGS) \
	$(NULL)

libmedialibrary_la_LIBADD += $(VLC_LIBS)

endif


EXTRA_DIST=

if HAVE_TESTS

check_PROGRAMS = unittest samples

lib_LTLIBRARIES += libgtest.la libgtestmain.la

libgtest_la_SOURCES = \
	googletest/googletest/src/gtest-all.cc

libgtestmain_la_SOURCES = \
	googletest/googletest/src/gtest_main.cc

libgtest_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/googletest/googletest \
	-I$(top_srcdir)/googletest/googletest/include

libgtestmain_la_CPPFLAGS = $(AM_CPPFLAGS) $(libgtest_la_CPPFLAGS)

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
	test/unittest/ChapterTests.cpp \
	test/unittest/DeviceTests.cpp \
	test/unittest/FileTests.cpp \
	test/unittest/FolderTests.cpp \
	test/unittest/FsUtilsTests.cpp \
	test/unittest/UrlTests.cpp \
	test/unittest/GenreTests.cpp \
	test/unittest/LabelTests.cpp \
	test/unittest/MediaTests.cpp \
	test/unittest/MovieTests.cpp \
	test/unittest/PlaylistTests.cpp \
	test/unittest/RemovalNotifierTests.cpp \
	test/unittest/ShowTests.cpp \
	test/unittest/Tests.cpp \
	test/unittest/VideoTrackTests.cpp \
	test/unittest/MiscTests.cpp \
	test/unittest/ThumbnailTests.cpp \
	test/unittest/SubtitleTrackTests.cpp \
	test/unittest/BookmarkTests.cpp \
	test/unittest/VideoGroupTests.cpp \
	test/unittest/TitleAnalyzerTests.cpp \
	$(NULL)

EXTRA_DIST += test/unittest/db_v3.sql

unittest_CPPFLAGS = 		\
	$(MEDIALIB_CPPFLAGS) 	\
	-DSRC_DIR=\"$(abs_srcdir)\"	\
	-I$(top_srcdir)/test	\
	-I$(top_srcdir)/googletest/googletest/include \
	$(libmedialibrary_la_CPPFLAGS) \
	$(NULL)

unittest_LDADD = 		\
	libmedialibrary.la 	\
	libgtest.la			\
	libgtestmain.la		\
	$(SQLITE_LIBS)		\
	$(NULL)

samples_SOURCES = 						\
	test/common/MediaLibraryTester.cpp 	\
	test/samples/main.cpp 				\
	test/samples/Tester.cpp 			\
	test/samples/BackupRestorePlaylist.cpp \
	$(NULL)

samples_CPPFLAGS = 			\
	$(MEDIALIB_CPPFLAGS)	\
	-DSRC_DIR=\"$(abs_srcdir)\"	\
	-I$(top_srcdir)/test	\
	-I$(top_srcdir)/googletest/googletest/include \
	$(RAPIDJSON_CFLAGS)		\
	$(libmedialibrary_la_CPPFLAGS) \
	$(NULL)

samples_LDADD = 		\
	libmedialibrary.la	\
	libgtest.la			\
	libgtestmain.la		\
	$(SQLITE_LIBS)		\
	$(NULL)

EXTRA_PROGRAMS = test_discoverer sqlite_load

test_discoverer_SOURCES = test/discoverer/main.cpp
test_discoverer_CXXFLAGS = $(MEDIALIB_CPPFLAGS)
test_discoverer_LDADD = libmedialibrary.la $(SQLITE_LIBS)

sqlite_load_SOURCES = test/sqlite_load/SqliteLoadTest.cpp
sqlite_load_CPPFLAGS = -I$(top_srcdir)/test
sqlite_load_CXXFLAGS = $(MEDIALIB_CPPFLAGS)
sqlite_load_LDADD = libmedialibrary.la $(SQLITE_LIBS)

endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = medialibrary.pc
EXTRA_DIST += medialibrary.pc \
	src/database/migrations/migration3-5.sql \
	src/database/migrations/migration7-8.sql \
	src/database/migrations/migration13-14.sql \
	src/database/migrations/migration14-15.sql \
	src/database/migrations/migration15-16.sql \
	src/database/migrations/migration16-17.sql \
	src/database/migrations/migration17-18.sql \
	src/database/migrations/migration18-19.sql \
	src/database/migrations/migration19-20.sql \
	src/database/migrations/migration20-21.sql \
	src/database/migrations/migration21-22.sql \
	src/database/tables/File_triggers_v14.sql \
	src/database/tables/Media_triggers_v14.sql \
	src/database/tables/Folder_triggers_v14.sql \
	src/database/tables/Folder_triggers_v15.sql \
	src/database/tables/Playlist_triggers_v14.sql \
	src/database/tables/Playlist_triggers_v16.sql \
	src/database/tables/Task_triggers_v18.sql \
	src/database/tables/Thumbnail_triggers_v17.sql \
	src/database/tables/Thumbnail_triggers_v18.sql \
	$(NULL)
	

	
