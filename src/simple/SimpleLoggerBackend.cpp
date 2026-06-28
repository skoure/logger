/**
 * @file SimpleLoggerBackend.cpp
 * @brief ILoggerBackend implementation for the SimpleLogger backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <SimpleLoggerBackend.h>
#include <SimpleLogger.h>
#include <LoggerFactoryImpl.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace sk::logger;

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SimpleLoggerBackend>());
        return true;
    }();
}

LoggerBasePtr SimpleLoggerBackend::createLogger(const std::string& name)
{
    return std::make_shared<SimpleLogger>(name);
}
   
void SimpleLoggerBackend::applyParentSinks(LoggerPtr child, LoggerPtr parent)
{
    auto childSimple  = std::dynamic_pointer_cast<SimpleLogger>(child);
    auto parentSimple = std::dynamic_pointer_cast<SimpleLogger>(parent);

    // 1. Guard conditions
    if (!childSimple || !parentSimple || !childSimple->getAdditivity()) {
        return;
    }

    // 2. Fetch the read-only references
    const auto& childSinks  = childSimple->getSinks();
    const auto& parentSinks = parentSimple->getSinks();

    // 3. Create a combined buffer and allocate memory upfront
    std::vector<SimpleSink> combinedSinks;
    combinedSinks.reserve(childSinks.size() + parentSinks.size());

    // 4. Merge child sinks first, then parent sinks
    combinedSinks.insert(combinedSinks.end(), childSinks.begin(), childSinks.end());
    combinedSinks.insert(combinedSinks.end(), parentSinks.begin(), parentSinks.end());

    // 5. Replace the child's sinks entirely via the setter
    childSimple->setSinks(std::move(combinedSinks)); 
}
        


void SimpleLoggerBackend::configureLogger(LoggerPtr loggerPtr,
                                           const std::vector<SinkConfig>& sinks)
{
    if (sinks.empty()) return;

    auto* sl = dynamic_cast<SimpleLogger*>(loggerPtr.get());
    if (!sl) return;

    std::vector<SimpleSink> simpleSinks;
    simpleSinks.reserve(sinks.size());

    for (const SinkConfig& sc : sinks)
    {
        SimpleSink sink;
        sink.pattern = sc.pattern;

        sink.level = sc.level;

        if (sc.type == "console")
        {
            auto colorIt = sc.properties.find("color");
            sink.color  = (colorIt != sc.properties.end())
                          && (colorIt->second == "true");
            sink.stream = std::shared_ptr<std::ostream>(
                &std::clog, [](std::ostream*) {});
        }
        else if (sc.type == "file")
        {
            auto it = sc.properties.find("path");
            if (it == sc.properties.end())
                throw std::runtime_error(
                    "SimpleLoggerBackend::configureLogger: 'file' sink missing 'path'");

            auto fs = std::make_shared<std::ofstream>(it->second, std::ios::app);
            if (!fs->is_open())
                throw std::runtime_error(
                    std::string("SimpleLoggerBackend::configureLogger: cannot open file: ")
                    + it->second);
            sink.stream = fs;
        }
        else if (sc.type == "rotating_file")
        {
            // SimpleLogger does not support rotation — treat as a plain file.
            auto it = sc.properties.find("path");
            if (it == sc.properties.end())
                throw std::runtime_error(
                    "SimpleLoggerBackend::configureLogger: 'rotating_file' sink missing 'path'");

            auto fs = std::make_shared<std::ofstream>(it->second, std::ios::app);
            if (!fs->is_open())
                throw std::runtime_error(
                    std::string("SimpleLoggerBackend::configureLogger: cannot open file: ")
                    + it->second);
            sink.stream = fs;
        }
        else
        {
            // Unknown type — skip
            continue;
        }

        simpleSinks.push_back(std::move(sink));
    }

    sl->setSinks(std::move(simpleSinks));
}

void SimpleLoggerBackend::configureLoggerWithOstream(LoggerPtr loggerPtr,
                                                     std::ostream& os,
                                                     const std::string& canonicalPattern)
{
    auto* sl = dynamic_cast<SimpleLogger*>(loggerPtr.get());
    if (!sl) return;

    SimpleSink sink;
    sink.pattern = canonicalPattern;
    sink.stream  = std::shared_ptr<std::ostream>(&os, [](std::ostream*) {});  // non-owning
    auto sinks = sl->getSinks();
    sinks.push_back(std::move(sink));
    sl->setSinks(std::move(sinks));
}

void SimpleLoggerBackend::clearSinks(LoggerPtr logger)
{
    auto* sl = dynamic_cast<SimpleLogger*>(logger.get());
    if (sl) sl->setSinks({});
}

void sk::logger::useSimpleLoggerBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SimpleLoggerBackend>());
}
