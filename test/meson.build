test_include_dir = include_directories('.')

subdir('unittest')

if get_option('libvlc').enabled()
  subdir('samples')
  subdir('discoverer')
  subdir('sqlite_load')
  subdir('fast_teardown')
  subdir('fast_discover_cancel')
endif
