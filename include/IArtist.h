#pragma once

#include <string>
#include <vector>

#include "Types.h"

class IArtist
{
public:
    virtual ~IArtist() = default;
    virtual unsigned int id() const = 0;
    virtual const std::string& name() const = 0;
    virtual const std::string& shortBio() const = 0;
    virtual bool setShortBio( const std::string& shortBio ) = 0;
    virtual std::vector<AlbumPtr> albums() const = 0;
};
