/**
 * @file SpdlogBackend.cpp
 * @brief ILoggerBackend implementation for the spdlog backend.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 21, 2026
 */
#include <SpdlogBackend.h>
#include <SpdlogLogger.h>
#include <SpdlogPatternTranslator.h>
#include <SpdlogThreadLocal.h>
#include <LoggerFactoryImpl.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>

#include <memory>
#include <stdexcept>
#include <string>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// Custom spdlog flag formatters
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Custom spdlog flag '%*' — emits the current thread name.
 *
 * Reads from spdlog_tls::threadName which is set by SpdlogLogger::append().
 */
class ThreadNameFormatter final : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg& /*msg*/,
                const std::tm& /*tm_time*/,
                spdlog::memory_buf_t& dest) override
    {
        const std::string& name = spdlog_tls::threadName;
        dest.append(name.data(), name.data() + name.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<ThreadNameFormatter>();
    }
};

/**
 * @brief Custom spdlog flag '%&' — emits the current marker name.
 *
 * Reads from spdlog_tls::markerName which is set by SpdlogLogger::append().
 */
class MarkerFormatter final : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg& /*msg*/,
                const std::tm& /*tm_time*/,
                spdlog::memory_buf_t& dest) override
    {
        const char* name = spdlog_tls::markerName;
        if (name && name[0] != '\0')
            dest.append(name, name + std::char_traits<char>::length(name));
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<MarkerFormatter>();
    }
};

// ---------------------------------------------------------------------------
// Helper: create a spdlog sink with a custom pattern formatter applied
// ---------------------------------------------------------------------------

void applyPattern(const std::shared_ptr<spdlog::sinks::sink>& sink,
                  const std::string& spdlogPattern)
{
    auto formatter = spdlog::details::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<ThreadNameFormatter>('*');
    formatter->add_flag<MarkerFormatter>('&');
    formatter->set_pattern(spdlogPattern);
    sink->set_formatter(std::move(formatter));
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SpdlogBackend>());
        return true;
    }();
}

// ---------------------------------------------------------------------------
// ILoggerBackend interface
// ---------------------------------------------------------------------------

LoggerPtr SpdlogBackend::createLogger(const std::string& name)
{
    return std::make_shared<SpdlogLogger>(name);
}

void SpdlogBackend::applyParentSinks(LoggerPtr child, LoggerPtr parent)
{
    auto childSpdlog  = std::dynamic_pointer_cast<SpdlogLogger>(child);
    auto parentSpdlog = std::dynamic_pointer_cast<SpdlogLogger>(parent);

    if (!childSpdlog || !parentSpdlog) return;

    auto parentInternal = parentSpdlog->getInternalLogger();
    auto childInternal  = childSpdlog->getInternalLogger();

    if (!parentInternal || !childInternal) return;

    childInternal->sinks() = parentInternal->sinks();
}

void SpdlogBackend::configureLogger(LoggerPtr loggerPtr,
                                    const std::vector<SinkConfig>& sinks)
{
    if (sinks.empty()) return;

    auto* spLogger = dynamic_cast<SpdlogLogger*>(loggerPtr.get());
    if (!spLogger) return;

    auto internalLogger = spLogger->getInternalLogger();
    if (!internalLogger) return;

    std::vector<std::shared_ptr<spdlog::sinks::sink>> newSinks;

    for (const SinkConfig& sc : sinks)
    {
        const std::string spdlogPattern =
            SpdlogPatternTranslator::translate(sc.pattern);

        std::shared_ptr<spdlog::sinks::sink> sink;

        if (sc.type == "console")
        {
            auto consoleSink =
                std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            applyPattern(consoleSink, spdlogPattern);
            sink = consoleSink;
        }
        else if (sc.type == "file")
        {
            auto it = sc.properties.find("path");
            if (it == sc.properties.end())
                throw std::runtime_error(
                    "SpdlogBackend::configureLogger: 'file' sink missing 'path'");

            auto fileSink =
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                    it->second, /*truncate=*/false);
            applyPattern(fileSink, spdlogPattern);
            sink = fileSink;
        }
        else if (sc.type == "rotating_file")
        {
            auto pathIt     = sc.properties.find("path");
            auto sizeIt     = sc.properties.find("max_size");
            auto filesIt    = sc.properties.find("max_files");

            if (pathIt == sc.properties.end())
                throw std::runtime_error(
                    "SpdlogBackend::configureLogger: 'rotating_file' sink missing 'path'");

            std::size_t maxSize  = sizeIt  != sc.properties.end()
                                   ? static_cast<std::size_t>(std::stoul(sizeIt->second))
                                   : 10 * 1024 * 1024; // 10 MiB default
            std::size_t maxFiles = filesIt != sc.properties.end()
                                   ? static_cast<std::size_t>(std::stoul(filesIt->second))
                                   : 3;

            auto rotatingSink =
                std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    pathIt->second, maxSize, maxFiles);
            applyPattern(rotatingSink, spdlogPattern);
            sink = rotatingSink;
        }
        else
        {
            // Unknown type — skip silently
            continue;
        }

        newSinks.push_back(sink);
    }

    if (!newSinks.empty())
        internalLogger->sinks() = newSinks;
}

void sk::logger::useSpdlogBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<SpdlogBackend>());
}
