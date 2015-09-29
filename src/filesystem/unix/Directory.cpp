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
    struct stat s;
    lstat( path.c_str(), &s );
    m_lastModificationDate = s.st_mtim.tv_sec;
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

const std::vector<std::string>&Directory::dirs() const
{
    return m_dirs;
}

unsigned int Directory::lastModificationDate() const
{
    return m_lastModificationDate;
}

bool Directory::isRemovable() const
{
    //FIXME
    return false;
}

std::string Directory::toAbsolute(const std::string& path)
{
    auto abs = std::unique_ptr<char[]>( new char[PATH_MAX] );
    if ( realpath( path.c_str(), abs.get() ) == nullptr )
    {
        std::string err( "Failed to convert to absolute path" );
        err += "(" + path + "): ";
        err += strerror(errno);
        throw std::runtime_error( err );
    }
    return std::string{ abs.get() };
}

void Directory::read()
{
    auto dir = std::unique_ptr<DIR, int(*)(DIR*)>( opendir( m_path.c_str() ), closedir );
    if ( dir == nullptr )
    {
        std::string err( "Failed to open directory " );
        err += m_path;
        err += strerror(errno);
        throw std::runtime_error( err );
    }

    dirent* result = nullptr;

    while ( ( result = readdir( dir.get() ) ) != nullptr )
    {
        if ( strcmp( result->d_name, "." ) == 0 ||
             strcmp( result->d_name, ".." ) == 0 )
        {
            continue;
        }
        std::string path = m_path + "/" + result->d_name;

#if defined(_DIRENT_HAVE_D_TYPE) && defined(_BSD_SOURCE)
        if ( result->d_type == DT_DIR )
        {
#else
        struct stat s;
        if ( lstat( result->d_name, &s ) != 0 )
        {
            std::string err( "Failed to get file info " );
            err += m_path;
            err += strerror(errno);
            throw std::runtime_error( err );
        }
        if ( S_ISDIR( s.st_mode ) )
        {
#endif
            m_dirs.emplace_back( toAbsolute( path ) );
        }
        else
        {
            m_files.emplace_back( toAbsolute( path ) );
        }
    }
}

}
