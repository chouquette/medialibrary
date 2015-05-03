#include "File.h"
#include "Utils.h"

#include <stdexcept>
#include <sys/stat.h>

namespace fs
{

File::File( const std::string& filePath )
    : m_path( utils::file::directory( filePath ) )
    , m_name( utils::file::fileName( filePath ) )
    , m_fullPath( filePath )
    , m_extension( utils::file::extension( filePath ) )
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
