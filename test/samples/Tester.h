#ifndef TESTER_H
#define TESTER_H

#include "MediaLibrary.h"

class Tester
{
public:
    Tester( MediaLibrary* ml, const std::string& caseName );
    void run();

private:
    MediaLibrary* m_ml;
};

#endif // TESTER_H
