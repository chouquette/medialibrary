bench_requests_srcs = [
  'BenchAlbumRequests.cpp',
  'BenchMediaRequests.cpp',
  'main.cpp',
  'BenchRequestsCommon.cpp'
]

executable('bench_requests',
  bench_requests_srcs,
  dependencies: benchmark_dep,
  link_with: medialib,
  include_directories: includes
)

