#pragma once

#include "filesystem/IDirectory.h"
#include <string>

namespace fs
{

class Directory : public IDirectory
{
public:
    explicit Directory( const std::string& path );
    virtual const std::string& path() const override;
    virtual const std::vector<std::string>& files() const override;

private:
    static std::string toAbsolute( const std::string& path );

private:
    void read();

private:
    const std::string m_path;
    std::vector<std::string> m_files;
};

}
