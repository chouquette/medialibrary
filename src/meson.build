medialib_src = [
	'Album.cpp',
	'Artist.cpp',
	'AudioTrack.cpp',
	'Device.cpp',
	'Factory.cpp',
	'File.cpp',
	'Chapter.cpp',
	'Folder.cpp',
	'Genre.cpp',
	'Label.cpp',
	'LockFile.cpp',
	'Media.cpp',
	'MediaLibrary.cpp',
	'Movie.cpp',
	'Playlist.cpp',
	'Settings.cpp',
	'Show.cpp',
	'Thumbnail.cpp',
	'ShowEpisode.cpp',
	'VideoTrack.cpp',
	'Metadata.cpp',
	'SubtitleTrack.cpp',
	'database/SqliteConnection.cpp',
	'database/SqliteTools.cpp',
	'database/SqliteTransaction.cpp',
	'discoverer/DiscovererWorker.cpp',
	'discoverer/FsDiscoverer.cpp',
	'factory/DeviceListerFactory.cpp',
	'filesystem/common/CommonDevice.cpp',
	'filesystem/common/CommonDeviceLister.cpp',
	'filesystem/common/CommonFile.cpp',
	'filesystem/common/CommonDirectory.cpp',
    'filesystem/FsHolder.cpp',
	'logging/IostreamLogger.cpp',
	'logging/Logger.cpp',
	'metadata_services/MetadataParser.cpp',
	'metadata_services/LinkService.cpp',
	'thumbnails/ThumbnailerWorker.cpp',
	'parser/Parser.cpp',
	'parser/ParserWorker.cpp',
	'parser/Task.cpp',
	'utils/Directory.cpp',
	'utils/Filename.cpp',
	'utils/ModificationsNotifier.cpp',
	'utils/Strings.cpp',
	'utils/Url.cpp',
	'utils/File.cpp',
	'utils/TitleAnalyzer.cpp',
    'utils/Xml.cpp',
	'utils/SWMRLock.cpp',
	'Bookmark.cpp',
	'MediaGroup.cpp',
	'utils/XxHasher.cpp',
	'utils/StringKey.cpp',
    'Deprecated.cpp',
]

use_xxhash_dispatch = host_machine.cpu_family() == 'x86' or host_machine.cpu_family() == 'x86_64'

if use_xxhash_dispatch
  add_project_arguments('-DXXHASH_DYNAMIC_DISPATCH=1', language: ['cpp'])
endif

if not libxxhash_dep.found()
  xxhash_lib = []
  if use_xxhash_dispatch
    xxhash_lib = static_library('xxhash',
      ['utils/xxhash/xxh_x86dispatch.c']
    )
    libxxhash_dep = declare_dependency(link_with: xxhash_lib,
      include_directories: ['utils/xxhash/']
    )
  endif
endif

if libvlc_dep.found()
  if libvlc_dep.version().version_compare('>=4.0')
    medialib_src += [
        'thumbnails/CoreThumbnailer.cpp',
        'thumbnails/CoreThumbnailer.h',
    ]
  else
    medialib_src += [
        'thumbnails/VmemThumbnailer.cpp',
        'thumbnails/VmemThumbnailer.h',
        'thumbnails/imagecompressors/JpegCompressor.cpp',
    ]
  endif
  medialib_src += [
    'metadata_services/vlc/VLCMetadataService.cpp',
    'metadata_services/vlc/Common.cpp',
    'utils/VLCInstance.cpp',
    'filesystem/libvlc/FileSystemFactory.cpp',
    'filesystem/libvlc/Directory.cpp',
    'filesystem/libvlc/File.cpp',
    'filesystem/libvlc/Device.cpp',
    'filesystem/libvlc/DeviceLister.cpp',
  ]
endif

if host_machine.system() == 'linux'
    medialib_src += 'filesystem/unix/DeviceLister.cpp'
elif host_machine.system() == 'darwin'
	medialib_src += 'filesystem/darwin/DeviceLister.mm'
elif host_machine.system() == 'windows'
	medialib_src += 'filesystem/win32/DeviceLister.cpp'
endif

if host_machine.system() == 'windows'
    medialib_soversion = ''
else
    medialib_soversion = medialib_major_version
endif

medialib = library('medialibrary', medialib_src,
    dependencies: [
        sqlite_dep, 
        libjpeg_dep, 
        libvlc_dep, 
        libvlcpp_dep,
        threads_dep,
        libxxhash_dep,
        macosx_dep,
    ],
    install: true,
    soversion: medialib_soversion,
    version: medialib_soname_version,
    include_directories: includes
)
