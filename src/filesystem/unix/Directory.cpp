#include "Directory.h"
#include "File.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs
{

const std::vector<std::string> Directory::supportedExtensions {
    // Videos
    "avi", "3gp", "amv", "asf", "divx", "dv", "flv", "gxf",
    "iso", "m1v", "m2v", "m2t", "m2ts", "m4v", "mkv", "mov",
    "mp2", "mp4", "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg",
    "mts", "mxf", "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps",
    "rec", "rm", "rmvb", "tod", "ts", "vob", "vro", "webm", "wmv",
    // Images
    "png", "jpg", "jpeg",
    // Audio
    "a52", "aac", "ac3", "aiff", "amr", "aob", "ape",
    "dts", "flac", "it", "m4a", "m4p", "mid", "mka", "mlp",
    "mod", "mp1", "mp2", "mp3", "mpc", "oga", "ogg", "oma",
    "rmi", "s3m", "spx", "tta", "voc", "vqf", "w64", "wav",
    "wma", "wv", "xa", "xm"
};

Directory::Directory( const std::string& path )
    : m_path( toAbsolute( path ) )
{
    read();
}

const std::string&Directory::path() const
{
    return m_path;
}

const std::vector<std::string>& Directory::files() const
{
    return m_files;
}

std::string Directory::toAbsolute(const std::string& path)
{
    auto abs = std::unique_ptr<char[]>( new char[PATH_MAX] );
    if ( realpath( path.c_str(), abs.get() ) == nullptr )
        throw std::runtime_error( "Failed to convert to absolute path" );
    return std::string{ abs.get() };
}

void Directory::read()
{
    auto dir = std::unique_ptr<DIR, int(*)(DIR*)>( opendir( m_path.c_str() ), closedir );
    if ( dir == nullptr )
        throw std::runtime_error("Failed to open directory");

    dirent* result = nullptr;

    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( strcmp( result->d_name, "." ) == 0 ||
             strcmp( result->d_name, ".." ) == 0 )
        {
            continue;
        }
#if defined(_DIRENT_HAVE_D_TYPE) && defined(_BSD_SOURCE)
        if ( result->d_type == DT_DIR )
        {
#else
        struct stat s;
        if ( lstat( result->d_name, &s ) != 0 )
            throw std::runtime_error("Failed to get file info" );
        if ( S_ISDIR( s.st_mode ) )
        {
#endif
            //FIXME
            continue;
        }
        else
        {
            m_files.push_back( toAbsolute( result->d_name ) );
        }
    }
}

}
