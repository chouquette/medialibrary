#pragma once

#include <string>

class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void Error( const std::string& msg ) = 0;
    virtual void Warning( const std::string& msg ) = 0;
    virtual void Info( const std::string& msg ) = 0;
};
