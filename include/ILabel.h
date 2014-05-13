#ifndef ILABEL_H
#define ILABEL_H

#include <vector>

class IFile;

class ILabel
{
    public:
        virtual ~ILabel() {}

        virtual unsigned int id() const = 0;
        virtual const std::string& name() = 0;
        virtual std::vector<IFile*> files() = 0;
        virtual bool link( IFile* file ) = 0;
        virtual bool unlink( IFile* file ) const = 0;
};

#endif // ILABEL_H
