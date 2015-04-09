#include "File.h"

namespace fs
{

File::File( const std::string& path, const std::string& fileName )
    : m_path( path )
    , m_name( fileName )
    , m_fullPath( path + ( *path.rbegin() != '/' ? "/" : "" ) + fileName )
    , m_extension( extension( fileName ) )
{
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

std::string File::extension( const std::string& fileName )
{
    auto pos = fileName.find_last_of( '.' );
    if ( pos == std::string::npos || pos == fileName.length() )
        return {};
    return fileName.substr( pos + 1 );
}

}
