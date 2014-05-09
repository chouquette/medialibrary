#ifndef ILABEL_H
#define ILABEL_H

#include <vector>

class IFile;

class ILabel
{
    public:
        virtual ~ILabel() {}

        virtual const std::string& name() = 0;
        virtual std::vector<IFile*> files() = 0;
};

#endif // ILABEL_H
