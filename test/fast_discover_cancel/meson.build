fast_disc_exec = executable('fast_discover_cancel', 'main.cpp',
    link_with: [medialib],
    include_directories: [includes, test_include_dir],
    dependencies: [sqlite_dep, threads_dep],
    build_by_default: true
)

if get_option('long_running_tests')
  test('fast_discoverer_cancel', fast_disc_exec, suite: 'long_running_tests',
    priority: -1, timeout: 300)
endif
