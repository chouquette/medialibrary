test_include_dir = include_directories('.')

if get_option('fuzz')
  subdir('fuzz')
else
  subdir('unittest')

  subdir('samples')
  subdir('discoverer')
  subdir('sqlite_load')
  subdir('fast_teardown')
  subdir('fast_discover_cancel')
endif
