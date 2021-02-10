project('medialibrary', 'cpp',
        license: 'LGPLv2+',
        version: '0.9.1-dev',
        default_options: [
            'cpp_std=c++14',
            'buildtype=debug',
            'warning_level=2',
            'b_ndebug=if-release'
        ]
)

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
    threads_dep = dependency('threads')
endif

sqlite_dep = dependency('sqlite3', version: '>= 3.25.0',
                        required: true)

libvlc_dep = dependency('libvlc', version: '>= 3.0', required: get_option('libvlc'))
if libvlc_dep.found()
    cdata.set('HAVE_LIBVLC', 1)
endif

# We only require libjpeg when using the 3.0 thumbnailer, using vmem
libjpeg_dep = dependency('libjpeg', 
    required: libvlc_dep.found() and libvlc_dep.version().version_compare('< 4.0'))
if libjpeg_dep.found()
    cdata.set('HAVE_JPEG', 1)
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

config_h = configure_file(output: 'config.h', configuration: cdata)
add_project_arguments('-DHAVE_CONFIG_H=1', language: ['cpp'])

includes = include_directories('.', 'include', 'src')
subdir('include')
subdir('src')

if libvlc_dep.found()
  subdir('test')
endif

pkg = import('pkgconfig')
pkg.generate(libraries: medialib,
    version: meson.project_version(),
    name: 'medialibrary',
    description: 'cross platform medialibrary'
)