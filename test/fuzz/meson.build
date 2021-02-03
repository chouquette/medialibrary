# This tests url encoding & spliting
executable('fuzz_url_encode', 'url_encode.cpp',
  link_with: [medialib],
  link_args: '-fsanitize=fuzzer',
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: true
)

executable('utf8_nbchars', 'utf8_nbchars.cpp',
  link_with: [medialib],
  link_args: '-fsanitize=fuzzer',
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: true
)

executable('xxhash', 'xxhash.cpp',
  link_with: [medialib],
  link_args: '-fsanitize=fuzzer',
  include_directories: [includes, test_include_dir],
  dependencies: [sqlite_dep, threads_dep],
  build_by_default: true
)
