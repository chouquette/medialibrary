#pragma once

#include "filesystem/IFile.h"

#include <string>

namespace fs
{

class File : public IFile
{
public:
    File( const std::string& path, const std::string& fileName );
    virtual ~File() = default;

    virtual const std::string& name() const override;
    virtual const std::string& path() const override;
    virtual const std::string& fullPath() const override;
    virtual const std::string& extension() const override;
    virtual unsigned int lastModificationDate() const override;

private:
    const std::string m_path;
    const std::string m_name;
    const std::string m_fullPath;
    const std::string m_extension;
    unsigned int m_lastModificationDate;
};

}
