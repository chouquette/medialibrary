#ifndef IDESCRIPTION_H
#define IDESCRIPTION_H

#include <string>

class IDescription
{
    public:
        virtual ~IDescription() {}
        virtual const std::string& name() = 0;
        virtual unsigned int releaseYear() = 0;
        virtual const std::string& shortSummary() = 0;
        virtual const std::string& artworkUrl() = 0;
};

#endif // IDESCRIPTION_H
