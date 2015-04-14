#include "File.h"
#include "Utils.h"

namespace fs
{

File::File( const std::string& path, const std::string& fileName )
    : m_path( path )
    , m_name( fileName )
    , m_fullPath( path + ( *path.rbegin() != '/' ? "/" : "" ) + fileName )
    , m_extension( utils::file::extension( fileName ) )
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

}
