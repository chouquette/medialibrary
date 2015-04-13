#pragma once

#include "Types.h"

#include <vector>

class IFolder
{
public:
    virtual ~IFolder() = default;
    virtual unsigned int id() const = 0;
    virtual const std::string& path() = 0;
    // This will only returns the files in this immediate folder
    virtual std::vector<FilePtr> files() = 0;
};
