#include "File.h"
#include "Utils.h"

#include <stdexcept>
#include <sys/stat.h>

namespace fs
{

File::File( const std::string& path, const std::string& fileName )
    : m_path( path )
    , m_name( fileName )
    , m_fullPath( path + ( *path.rbegin() != '/' ? "/" : "" ) + fileName )
    , m_extension( utils::file::extension( fileName ) )
{
    struct stat s;
    if ( lstat( m_fullPath.c_str(), &s ) )
        throw std::runtime_error( "Failed to get file stats" );
    m_lastModificationDate = s.st_mtim.tv_sec;
}

const std::string& File::name() const
{
    return m_name;
}

const std::string& File::path() const
{
    return m_path;
}

const std::string& File::fullPath() const
{
    return m_fullPath;
}

const std::string& File::extension() const
{
    return m_extension;
}

unsigned int File::lastModificationDate() const
{
    return m_lastModificationDate;
}

}
