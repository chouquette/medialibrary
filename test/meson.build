test_include_dir = include_directories('.')

if get_option('fuzz')
  subdir('fuzz')
else
  subdir('unittest')

  rapidjson_dep = dependency('RapidJSON', required: false)
  if rapidjson_dep.found()
    subdir('samples')
  else
    warning('RapidJSON not found, disabling functional tests.')
  endif

  subdir('discoverer')
  subdir('sqlite_load')
  subdir('fast_teardown')
  subdir('fast_discover_cancel')
  subdir('subscriptions_importer')
endif
