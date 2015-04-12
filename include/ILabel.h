#ifndef ILABEL_H
#define ILABEL_H

#include <memory>
#include <vector>

#include "IMediaLibrary.h"

class ILabel
{
    public:
        virtual ~ILabel() {}

        virtual unsigned int id() const = 0;
        virtual const std::string& name() = 0;
        virtual std::vector<FilePtr> files() = 0;
};

#endif // ILABEL_H
