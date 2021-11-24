project('medialibrary', 'cpp',
        license: 'LGPLv2+',
        version: '0.9.5',
        default_options: [
            'cpp_std=c++14',
            'buildtype=debug',
            'warning_level=2',
            'b_ndebug=if-release'
        ]
)

macosx_dep = []
if host_machine.system() == 'darwin'
  add_languages('objcpp', required: true)
  # https://github.com/mesonbuild/meson/issues/5495
  add_global_arguments('-std=c++14', language: 'objcpp')
  macosx_dep = dependency('appleframeworks', modules: 'foundation')
endif

medialib_soname_version = '0.0.0'
ver_arr = medialib_soname_version.split('.')
medialib_major_version = ver_arr[0]
medialib_minor_version = ver_arr[1]
medialib_micro_version = ver_arr[2]

cxx = meson.get_compiler('cpp')
cdata = configuration_data()

cdata.set('_FILE_OFFSET_BITS', 64)

if host_machine.system() == 'windows'
    cdata.set('_WIN32_WINNT', '0x601')
    cdata.set('_UNICODE', 1)
    cdata.set('UNICODE', 1)
    cdata.set('_POSIX_C_SOURCE', '200809L')
    cdata.set('_BSD_SOURCE', 1)
    cdata.set('_GNU_SOURCE', 1)
endif

if cxx.has_function('link')
    cdata.set('HAVE_LINK', 1)
endif

message('Checking for C++11 thread support')
if cxx.compiles('''#include <thread>
    int main() { std::thread t; }''')
        cdata.set('CXX11_THREADS', 1)
endif

message('Checking for C++11 mutex support')
if cxx.compiles('''#include <mutex>
    int main() { std::mutex m; }''')
        cdata.set('CXX11_MUTEX', 1)
endif
    
message('Checking for C++11 thread_local support')
if cxx.compiles('thread_local int seaOtter = 0; int main() {}')
        cdata.set('CXX11_THREAD_LOCAL', 1)
endif

if host_machine.system() == 'windows'
    threads_dep = []
else
    pthreads = dependency('threads', required: true)
    threads_dep = declare_dependency(link_args: '-lpthread', dependencies: pthreads)
endif

sqlite_dep = dependency('sqlite3', version: '>= 3.33.0',
                        required: true)

libvlc_dep = dependency('libvlc', version: '>= 3.0', required: get_option('libvlc'))
if libvlc_dep.found()
    cdata.set('HAVE_LIBVLC', 1)
endif

# We only require libjpeg when using the 3.0 thumbnailer, using vmem
if libvlc_dep.found() and libvlc_dep.version().version_compare('< 4.0')
  libjpeg_dep = dependency('libjpeg', required: false)
  if not libjpeg_dep.found()
    jpegprefix = get_option('libjpeg_prefix')
    if not cxx.has_header('jpeglib.h', args: '-I' + jpegprefix / 'include')
      error('libjpeg was not found')
    endif
    libjpeg_dep = declare_dependency(
      include_directories: include_directories(jpegprefix / 'include'),
      link_args: ['-L' + jpegprefix / 'lib', '-ljpeg']
    )
  endif
  cdata.set('HAVE_JPEG', 1)
else
  libjpeg_dep = []
endif


if libvlc_dep.found()
    libvlcpp_dep = dependency('libvlcpp', required: false)
    if not libvlcpp_dep.found()
        if cxx.has_header('vlc.hpp', args: '-I' + join_paths(meson.source_root(), 'libvlcpp/vlcpp'))
            libvlcpp_dep = declare_dependency(
                include_directories: include_directories('libvlcpp/')
            )
        else
            error('libvlcpp not found. Please install it or initialize the submodule')
        endif
    endif
else
    libvlcpp_dep = []
endif

if get_option('force_attachment_api')
  if not libvlc_dep.found()
    warning('Invalid option \'force_attachment_api\' provided without libvlc enabled')
  elif libvlc_dep.version().version_compare('>=4.0')
    warning('Invalid option \'force_attachment_api\' provided with libvlc >=4.x')
  else
    cdata.set('FORCE_ATTACHMENTS_API', 1)
  endif
endif

config_h = configure_file(input: 'config.h.in', output: 'config.h', configuration: cdata)
add_project_arguments('-DHAVE_CONFIG_H=1', language: ['cpp'])

includes = include_directories('.', 'include', 'src')
subdir('include')
subdir('src')

# Ideally we shouldn't need an option to build tests, but we need
# https://github.com/mesonbuild/meson/issues/2518
# to be fixed first
if libvlc_dep.found() and not get_option('tests').disabled()
  subdir('test')
endif

benchmark_dep = dependency('benchmark', required: false, static: true)
if benchmark_dep.found()
  subdir('benchmark')
endif

pkg = import('pkgconfig')
pkg.generate(libraries: medialib,
    version: meson.project_version(),
    name: 'medialibrary',
    description: 'cross platform medialibrary'
)
