#include "Directory.h"
#include "File.h"

#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs
{

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
