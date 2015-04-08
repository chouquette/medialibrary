#pragma once

#include "Types.h"

#include <vector>

class IFolder
{
public:
    virtual ~IFolder() = default;
    virtual unsigned int id() const = 0;
    virtual const std::string& path() = 0;
    virtual std::vector<FilePtr> files() = 0;
};
