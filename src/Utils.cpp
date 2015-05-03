#include "Utils.h"

namespace utils
{

namespace file
{

std::string extension( const std::string& fileName )
{
    auto pos = fileName.find_last_of( '.' );
    if ( pos == std::string::npos )
        return {};
    return fileName.substr( pos + 1 );
}

std::string directory( const std::string& filePath )
{
    auto pos = filePath.find_last_of( '/' );
    if ( pos == std::string::npos )
        return {};
    return filePath.substr( 0, pos + 1 );
}

std::string fileName(const std::string& filePath)
{
    auto pos = filePath.find_last_of( '/' );
    if ( pos == std::string::npos )
        return filePath;
    return filePath.substr( pos + 1 );
}

}

}
