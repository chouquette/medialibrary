#pragma once

#include <string>

namespace utils
{

namespace file
{
    std::string extension( const std::string& fileName );
    std::string directory( const std::string& filePath );
    std::string fileName( const std::string& filePath );
}

}
