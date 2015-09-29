#pragma once

#include <ILogger.h>
#include <iostream>

class IostreamLogger : public ILogger
{
public:
    virtual void Error(const std::string& msg)
    {
        std::cout << "Error: " << msg;
    }

    virtual void Warning(const std::string& msg)
    {
        std::cout << "Warning: " << msg;
    }

    virtual void Info(const std::string& msg)
    {
        std::cout << "Info: " << msg;
    }
};
