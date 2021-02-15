samples_src = [
  'main.cpp',
  'Tester.cpp',
]

samples_exe = executable('samples', samples_src,
  link_with: [medialib],
  include_directories: [includes, test_include_dir],
  cpp_args: '-DSRC_DIR="' + meson.source_root() + '"',
  dependencies: [sqlite_dep, threads_dep, libvlc_dep, libvlcpp_dep],
  build_by_default: false
)

test_list = [
  'featuring',
  'parse_video',
  'parse_audio',
  'same_album_name_different_artist',
  'same_album_name_same_artist',
  'compilation',
  'compilation_no_albumartist',
  'release_year_same',
  'notags',
  'multi_cd',
  'no_album_artist',
  'utf8',
  'deduce_artwork_from_album',
  'deduce_artwork_from_track',
  'xiph_embedded_artwork',
  'playlist_external_media',
  'playlist_external_folder',
  'playlist_same_folder',
  'playlist_mixed_content',
  'playlist_extended_m3u',
  'playlist_encoding',
  'same_album_with_subfolder',
  'compilation_different_years',
  'tvshow',
  'unknown_tv_show',
  'media_groups',
  'attached_subs',
  'attached_audio',
]

if libvlc_dep.version().version_compare('>=4.0')
  test_list += [
    'thumbnail_sharing',
    'thumbnail_embedded_sharing'
  ]
endif

if host_machine.system() != 'windows'
  test_list += 'hidden'
endif

test_suites = [
  'Parse',
  'ParseTwice',
  'Resume',
  'Rescan',
  'Refresh'
]

foreach s : test_suites
  foreach t : test_list
      test(t, samples_exe,
          args: [s, t],
          suite: ['functional', s],
          is_parallel: false,
      )
  endforeach
endforeach

test('BackupRestorePlaylist', samples_exe,
  args: ['BackupRestorePlaylist', 'dummy'],
  is_parallel: false
)
