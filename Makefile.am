ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -Wall -Wsign-compare -Wextra -Wstrict-aliasing -Wstrict-overflow \
			  -Wformat=2 -Wno-unused-parameter -Wcast-align -Wpointer-arith \
			  -Wwrite-strings \
			  -pipe

MEDIALIB_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src

libmedialibrary_ladir = $(includedir)/medialibrary

libmedialibrary_la_CPPFLAGS = \
	$(MEDIALIB_CPPFLAGS) \
	$(SQLITE_CFLAGS) \
	$(VLC_CFLAGS) \
	$(VLCPP_CFLAGS) \
	-fvisibility=default \
	$(NULL)

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
	include/medialibrary/IDeviceLister.h \
	include/medialibrary/IFolder.h \
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
	src/factory/FileSystemFactory.cpp \
	src/factory/NetworkFileSystemFactory.cpp \
	src/factory/DeviceListerFactory.cpp \
	src/filesystem/common/CommonDevice.cpp \
	src/filesystem/common/CommonFile.cpp \
	src/filesystem/common/CommonDirectory.cpp \
	src/filesystem/network/Directory.cpp \
	src/filesystem/network/File.cpp \
	src/filesystem/network/Device.cpp \
	src/logging/IostreamLogger.cpp \
	src/logging/Logger.cpp \
	src/metadata_services/MetadataParser.cpp \
	src/metadata_services/vlc/VLCMetadataService.cpp \
	src/metadata_services/vlc/VLCThumbnailer.cpp \
	src/parser/Parser.cpp \
	src/parser/ParserService.cpp \
	src/utils/Filename.cpp \
	src/utils/ModificationsNotifier.cpp \
	src/utils/Url.cpp \
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
	src/factory/FileSystemFactory.h \
	src/factory/NetworkFileSystemFactory.h \
	src/factory/DeviceListerFactory.h \
	src/File.h \
	src/filesystem/common/CommonFile.h \
	src/filesystem/common/CommonDirectory.h \
	src/filesystem/common/CommonDevice.h \
	src/filesystem/unix/Device.h \
	src/filesystem/unix/Directory.h \
	src/filesystem/unix/File.h \
	src/filesystem/network/Device.h \
	src/filesystem/network/Directory.h \
	src/filesystem/network/File.h \
	src/filesystem/unix/DeviceLister.h \
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
	src/metadata_services/vlc/imagecompressors/IImageCompressor.h \
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
	src/utils/Url.h \
	src/utils/VLCInstance.h \
	src/VideoTrack.h \
	src/compat/Thread.h \
	src/compat/Mutex.h \
	src/compat/ConditionVariable.h \
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
	src/filesystem/darwin/DeviceLister.mm
endif
endif

libmedialibrary_la_LIBADD = \
	$(SQLITE_LIBS) 			\
	$(VLC_LIBS) 			\
	$(NULL)

libmedialibrary_la_LDFLAGS = -no-undefined -export-symbols-regex 'NewMediaLibrary'
if HAVE_DARWIN
libmedialibrary_la_LIBADD += libmedialibrary_macos.la
libmedialibrary_la_LDFLAGS += -Wl,-undefined,dynamic_lookup,-framework,Foundation
endif


if HAVE_EVAS
libmedialibrary_la_SOURCES += src/metadata_services/vlc/imagecompressors/EvasCompressor.cpp
noinst_HEADERS += src/metadata_services/vlc/imagecompressors/EvasCompressor.h
libmedialibrary_la_CPPFLAGS += $(EVAS_CFLAGS)
else
if HAVE_LIBJPEG
libmedialibrary_la_SOURCES += src/metadata_services/vlc/imagecompressors/JpegCompressor.cpp
noinst_HEADERS += src/metadata_services/vlc/imagecompressors/JpegCompressor.h
libmedialibrary_la_CPPFLAGS += $(LIBJPEG_CFLAGS)
libmedialibrary_la_LIBADD += $(LIBJPEG_LIBS)
endif
endif

lib_LTLIBRARIES = libmedialibrary.la
if HAVE_DARWIN
lib_LTLIBRARIES += libmedialibrary_macos.la
endif

if HAVE_TESTS

check_PROGRAMS = unittest samples

lib_LTLIBRARIES += libgtest.la libgtestmain.la

libgtest_la_SOURCES = \
	googletest/src/gtest-all.cc

libgtestmain_la_SOURCES = \
	googletest/src/gtest_main.cc

libgtest_la_CPPFLAGS = \
	-I$(top_srcdir)/googletest \
	-I$(top_srcdir)/googletest/include

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
	test/unittest/UrlTests.cpp \
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
	test/unittest/MiscTests.cpp \
	$(NULL)

unittest_CPPFLAGS = 		\
	$(MEDIALIB_CPPFLAGS) 	\
	-I$(top_srcdir)/test	\
	-I$(top_srcdir)/googletest/include \
	$(SQLITE_CFLAGS) 		\
	$(VLCPP_CFLAGS) \
	$(VLC_CFLAGS) \
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

samples_CPPFLAGS = 			\
	$(MEDIALIB_CPPFLAGS)	\
	-DSRC_DIR=\"$(abs_srcdir)\"	\
	-I$(top_srcdir)/test	\
	-I$(top_srcdir)/googletest/include \
	$(SQLITE_CFLAGS)		\
	$(RAPIDJSON_CFLAGS)		\
	$(VLCPP_CFLAGS) \
	$(VLC_CFLAGS) \
	$(NULL)

samples_LDADD = 		\
	libmedialibrary.la	\
	libgtest.la			\
	libgtestmain.la		\
	$(PTHREAD_LIBS) 	\
	$(SQLITE_LIBS)		\
	$(NULL)

endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = medialibrary.pc
EXTRA_DIST = medialibrary.pc
