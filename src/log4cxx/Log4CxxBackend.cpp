/**
 * @file Log4CxxBackend.cpp
 * @brief ILoggerBackend implementation for the Log4cxx backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <Log4CxxBackend.h>
#include <Log4CxxLogger.h>
#include <Log4CxxPatternTranslator.h>
#include <LoggerFactoryImpl.h>

#include <log4cxx/consoleappender.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/rolling/rollingfileappender.h>
#include <log4cxx/patternlayout.h>

#include <stdexcept>

using namespace sk::logger;

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
        return true;
    }();
}

LoggerPtr Log4CxxBackend::createLogger(const std::string& name)
{
    return std::make_shared<Log4CxxLogger>(name);
}

void Log4CxxBackend::applyParentSinks(LoggerPtr /*child*/, LoggerPtr /*parent*/)
{
    // Log4cxx manages its own appender inheritance — nothing to do here.
}

void Log4CxxBackend::configureLogger(LoggerPtr loggerPtr,
                                      const std::vector<SinkConfig>& sinks)
{
    if (sinks.empty()) return;

    auto* l4Logger = dynamic_cast<Log4CxxLogger*>(loggerPtr.get());
    if (!l4Logger) return;

    log4cxx::LoggerPtr internalLogger = l4Logger->getInternalLogger();
    if (!internalLogger) return;

    // Clear existing appenders and disable additivity so that only our
    // explicitly configured sinks receive log events.
    internalLogger->removeAllAppenders();
    internalLogger->setAdditivity(false);

    for (const SinkConfig& sc : sinks)
    {
        const std::string log4cxxPattern =
            Log4CxxPatternTranslator::translate(sc.pattern);

        auto layout = std::make_shared<log4cxx::PatternLayout>(
            log4cxx::LogString(log4cxxPattern.begin(), log4cxxPattern.end()));

        log4cxx::AppenderPtr appender;

        if (sc.type == "console")
        {
            auto consoleAppender = std::make_shared<log4cxx::ConsoleAppender>();
            consoleAppender->setLayout(layout);
            log4cxx::helpers::Pool pool;
            consoleAppender->activateOptions(pool);
            appender = consoleAppender;
        }
        else if (sc.type == "file")
        {
            auto it = sc.properties.find("path");
            if (it == sc.properties.end())
                throw std::runtime_error(
                    "Log4CxxBackend::configureLogger: 'file' sink missing 'path'");

            auto fileAppender = std::make_shared<log4cxx::FileAppender>();
            fileAppender->setLayout(layout);
            fileAppender->setFile(
                log4cxx::LogString(it->second.begin(), it->second.end()));
            fileAppender->setAppend(true);
            log4cxx::helpers::Pool pool;
            fileAppender->activateOptions(pool);
            appender = fileAppender;
        }
        else if (sc.type == "rotating_file")
        {
            auto pathIt  = sc.properties.find("path");
            if (pathIt == sc.properties.end())
                throw std::runtime_error(
                    "Log4CxxBackend::configureLogger: 'rotating_file' sink missing 'path'");

            auto rollingAppender =
                std::make_shared<log4cxx::rolling::RollingFileAppender>();
            rollingAppender->setLayout(layout);
            rollingAppender->setFile(
                log4cxx::LogString(pathIt->second.begin(), pathIt->second.end()));
            rollingAppender->setAppend(true);
            log4cxx::helpers::Pool pool;
            rollingAppender->activateOptions(pool);
            appender = rollingAppender;
        }
        else
        {
            // Unknown type — skip silently
            continue;
        }

        if (appender)
            internalLogger->addAppender(appender);
    }
}

void sk::logger::useLog4CxxBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
}
