fast_teardown_exec = executable('fast_teardown', 'main.cpp',
    link_with: [medialib],
    include_directories: [includes, test_include_dir],
    dependencies: [sqlite_dep, threads_dep],
    build_by_default: true
)

if get_option('long_running_tests')
  test('fast_teardown', fast_teardown_exec, suite: 'long_running_tests',
    priority: -1, timeout: 300)
endif
