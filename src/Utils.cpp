#include "Utils.h"

namespace utils
{

namespace file
{

std::string extension( const std::string& fileName )
{
    auto pos = fileName.find_last_of( '.' );
    if ( pos == std::string::npos || pos == fileName.length() )
        return {};
    return fileName.substr( pos + 1 );
}

}

}
