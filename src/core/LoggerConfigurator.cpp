/**
 * @file LoggerConfigurator.cpp
 * @brief Drives the full configuration pipeline: JSON → LoggerConfig → backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "LoggerConfigurator.h"
#include "JsonConfigParser.h"
#include "LoggerFactoryImpl.h"
#include "LoggerBase.h"

#include <logger/Logger.h>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Helper: map a level string to Logger::Level
// ---------------------------------------------------------------------------

static Logger::Level parseLevel(const std::string& s)
{
    if (s == "FATAL" || s == "fatal") return Logger::Level::Fatal;
    if (s == "ERROR" || s == "error") return Logger::Level::Error;
    if (s == "WARN"  || s == "warn")  return Logger::Level::Warn;
    if (s == "DEBUG" || s == "debug") return Logger::Level::Debug;
    if (s == "TRACE" || s == "trace") return Logger::Level::Trace;
    // Default: INFO
    return Logger::Level::Info;
}

// ---------------------------------------------------------------------------
// LoggerConfigurator::configure
// ---------------------------------------------------------------------------

void LoggerConfigurator::configure(const std::string& filePath)
{
    std::vector<LoggerConfig> configs = JsonConfigParser::parse(filePath);

    LoggerFactoryImpl& impl = LoggerFactoryImpl::getInstance();

    for (const LoggerConfig& lc : configs)
    {
        // "root" maps to the special root logger name used by the hierarchy
        const std::string& name = lc.name;

        LoggerPtr logger = impl.getLogger(name);
        if (!logger)
            continue;

        if (!lc.level.empty())
            logger->setLevel(parseLevel(lc.level));

        impl.configureLogger(logger, lc.sinks);
    }
}
