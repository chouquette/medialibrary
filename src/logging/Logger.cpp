#include "Logger.h"
#include "logging/IostreamLogger.h"

std::unique_ptr<ILogger> Log::s_defaultLogger = std::unique_ptr<ILogger>( new IostreamLogger );
std::atomic<ILogger*> Log::s_logger;
