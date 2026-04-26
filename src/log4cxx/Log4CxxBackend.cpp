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

#include <log4cxx/appenderskeleton.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/rolling/rollingfileappender.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/writerappender.h>
#include <log4cxx/helpers/outputstreamwriter.h>
#include <log4cxx/helpers/outputstream.h>
#include <log4cxx/helpers/bytebuffer.h>
#include <log4cxx/helpers/pool.h>

#include <ostream>
#include <stdexcept>

using namespace sk::logger;

// ---------------------------------------------------------------------------
// std::ostream adapter for log4cxx's OutputStream interface
//
// Must be at file scope — the LOG4CXX macros emit log4cxx-namespaced
// definitions that conflict when nested inside an anonymous namespace.
// Being in a .cpp file is sufficient to keep the class translation-unit local.
// ---------------------------------------------------------------------------

/**
 * @brief Wraps a std::ostream as a log4cxx OutputStream.
 *
 * log4cxx's I/O stack is: WriterAppender → OutputStreamWriter → OutputStream.
 * There is no built-in adapter that accepts a std::ostream directly, so we
 * implement the three-method interface ourselves.  The ostream is held by
 * non-owning reference — the caller must ensure it outlives this object.
 *
 * The LOG4CXX_OBJECT macros (DECLARE_/IMPLEMENT_) require the class to be
 * defined inside the log4cxx namespace (they use `helpers::Class` unqualified).
 * Since this adapter is only ever constructed by our own code — never by
 * log4cxx's class factory — we implement the three Object pure-virtuals
 * manually using fully-qualified names, avoiding the macros entirely.
 */
class StdOStreamOutputStream : public log4cxx::helpers::OutputStream
{
public:
    // Object protocol — manual implementation to avoid log4cxx macro constraints.
    // Advertise ourselves as a plain OutputStream; no factory registration needed.
    const log4cxx::helpers::Class& getClass() const override
    {
        return log4cxx::helpers::OutputStream::getStaticClass();
    }
    const void* cast(const log4cxx::helpers::Class& clazz) const override
    {
        if (&clazz == &log4cxx::helpers::Object::getStaticClass())
            return static_cast<const log4cxx::helpers::Object*>(this);
        if (&clazz == &log4cxx::helpers::OutputStream::getStaticClass())
            return static_cast<const log4cxx::helpers::OutputStream*>(this);
        return nullptr;
    }
    bool instanceof(const log4cxx::helpers::Class& clazz) const override
    {
        return cast(clazz) != nullptr;
    }

    explicit StdOStreamOutputStream(std::ostream& os) : os_(os) {}

    void close(log4cxx::helpers::Pool&) override {}

    void flush(log4cxx::helpers::Pool&) override { os_.flush(); }

    void write(log4cxx::helpers::ByteBuffer& buf, log4cxx::helpers::Pool&) override
    {
        os_.write(buf.current(), static_cast<std::streamsize>(buf.remaining()));
        buf.position(buf.limit());
    }

private:
    std::ostream& os_;
};

// ---------------------------------------------------------------------------

namespace {
    const bool s_registered = []() {
        LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
        return true;
    }();
}

LoggerBasePtr Log4CxxBackend::createLogger(const std::string& name)
{
    return std::make_shared<Log4CxxLogger>(name);
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
            auto colorIt = sc.properties.find("color");
            const bool useColor = (colorIt != sc.properties.end())
                                  && (colorIt->second == "true");

            if (useColor)
            {
                std::string coloredPattern = "%Y" + log4cxxPattern + "%y";
                layout = std::make_shared<log4cxx::PatternLayout>(
                    log4cxx::LogString(coloredPattern.begin(), coloredPattern.end()));
            }

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
        {
            if (sc.level.has_value())
            {
                auto skel = std::dynamic_pointer_cast<log4cxx::AppenderSkeleton>(appender);
                if (skel)
                    skel->setThreshold(Log4CxxLogger::toLog4CxxLevel(*sc.level));
            }
            internalLogger->addAppender(appender);
        }
    }
}

void Log4CxxBackend::configureLoggerWithOstream(LoggerPtr loggerPtr,
                                                std::ostream& os,
                                                const std::string& canonicalPattern)
{
    auto* l4Logger = dynamic_cast<Log4CxxLogger*>(loggerPtr.get());
    if (!l4Logger) return;

    log4cxx::LoggerPtr internalLogger = l4Logger->getInternalLogger();
    if (!internalLogger) return;

    const std::string l4Pattern = Log4CxxPatternTranslator::translate(canonicalPattern);
    auto layout = std::make_shared<log4cxx::PatternLayout>(
        log4cxx::LogString(l4Pattern.begin(), l4Pattern.end()));

    // OutputStreamWriter's ctor takes OutputStreamPtr& (non-const lvalue ref in
    // ABI <= 15), so we must upcast to the base shared_ptr type explicitly.
    log4cxx::helpers::OutputStreamPtr osStream =
        std::make_shared<StdOStreamOutputStream>(os);
    auto writer = std::make_shared<log4cxx::helpers::OutputStreamWriter>(osStream);

    auto appender = std::make_shared<log4cxx::WriterAppender>();
    appender->setLayout(layout);
    appender->setWriter(writer);
    log4cxx::helpers::Pool pool;
    appender->activateOptions(pool);

    internalLogger->addAppender(appender);
}

void sk::logger::useLog4CxxBackend()
{
    LoggerFactoryImpl::getInstance().setBackend(std::make_unique<Log4CxxBackend>());
}
