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

#include <fstream>
#include <set>
#include <sstream>
#include <string>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// LoggerConfigurator::configureFromJsonString — all apply logic lives here
// ---------------------------------------------------------------------------

void LoggerConfigurator::configureFromJsonString(const std::string& json)
{
    std::istringstream iss(json);
    std::vector<LoggerConfig> configs = JsonConfigParser::parse(iss);

    LoggerFactoryImpl& impl = LoggerFactoryImpl::getInstance();

    // Step 1: clean slate — clear all explicit levels, flush thresholds, and sinks
    // so that loggers not listed in this config revert to inherited behaviour.
    impl.clearAllLevels();
    impl.clearAllSinks();
    impl.clearAllFlushOn();

    // Step 2: apply explicit levels and sinks from the config.
    std::set<std::string> configured;
    for (const LoggerConfig& lc : configs)
    {
        const std::string& name = lc.name;

        LoggerPtr logger = impl.getLogger(name);
        if (!logger)
            continue;

        if (lc.level.has_value())
            logger->setLevel(*lc.level);

        if (lc.flushOn.has_value())
            logger->setFlushOn(*lc.flushOn);

        // Additivity is internal — set it on the LoggerBase implementation
        if (auto base = std::dynamic_pointer_cast<LoggerBase>(logger))
            base->setAdditivity(lc.additivity);

        impl.configureLogger(logger, lc.sinks);

        // The configuration contains an empty list of sinks and additivity is disabled,
        // so we clear the sinks for this logger.
        if (!lc.additivity && lc.sinks.empty())
            impl.clearSinks(logger);
            
        configured.insert(name);
    }

    // Step 3: propagate parent sinks to all loggers not listed in the config,
    // walking in parent-before-child order so intermediate unconfigured loggers
    // receive correct sinks before their descendants inherit from them.
    impl.propagateInheritedSinks(configured);
}

// ---------------------------------------------------------------------------
// LoggerConfigurator::configureFromJsonFile — reads file, delegates
// ---------------------------------------------------------------------------

void LoggerConfigurator::configureFromJsonFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error(
            "LoggerConfigurator: cannot open '" + filePath + "'");
    configureFromJsonString(std::string(std::istreambuf_iterator<char>(file),
                                        std::istreambuf_iterator<char>()));
}
