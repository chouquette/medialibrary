#pragma once

#include "filesystem/IDirectory.h"
#include <string>

namespace fs
{

class Directory : public IDirectory
{
public:
    explicit Directory( const std::string& path );
    virtual std::vector<std::unique_ptr<IFile>> files() const override;

public:
    static const std::vector<std::string> supportedExtensions;

private:
    static std::string toAbsolute( const std::string& path );

private:
    const std::string m_path;
};

}
