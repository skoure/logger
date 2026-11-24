/**
 * @file LoggerUtils.cpp
 * @brief Internal utility functions for logger implementations.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 20, 2026
 */
#include "LoggerUtils.h"
#include <cstdio>

#ifdef USE_CPPTRACE
#  include <cpptrace/cpptrace.hpp>
#endif

#ifdef __GNUG__
#  include <cxxabi.h>
#  include <cstdlib>
#endif

#ifdef __linux__
#  include <pthread.h>
#endif

#include <sstream>
#include <typeinfo>

namespace sk { namespace logger {

static std::string demangle(const char* name)
{
#ifdef __GNUG__
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled) ? demangled : name;
    std::free(demangled);
    return result;
#else
    return name;
#endif
}

std::string formatException(const char* msg, const std::exception& ex, int skip)
{
    std::ostringstream oss;
    if (msg && msg[0] != '\0') {
        oss << msg << ": ";
    }
    oss << demangle(typeid(ex).name()) << ": " << ex.what() << eol;
#ifdef USE_CPPTRACE
    // cpptrace generate_trace begins with 'Stack trace (most recent call first):'
    oss << cpptrace::generate_trace(skip).to_string();
#endif
    return oss.str();
}

std::string getCurrentThreadName()
{
#ifdef __linux__
    char name[64] = {};
    if (pthread_getname_np(pthread_self(), name, sizeof(name)) == 0)
        return std::string(name);
#endif
    return {};
}

std::string joinMarkerNames(const std::vector<const Marker*>& markers, const char* separator)
{
    std::string result;
    for (const Marker* marker : markers)
    {
        if (!marker)
            continue;
        if (!result.empty())
            result += separator;
        result += marker->getName();
    }
    return result;
}

std::string formatMessage(const char* fmt, va_list args)
{
    if (!fmt)
        return {};

    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (size <= 0)
        return {};

    std::string result(static_cast<std::size_t>(size) + 1, '\0');
    std::vsnprintf(&result[0], result.size(), fmt, args);
    result.resize(static_cast<std::size_t>(size));
    return result;
}

std::string replaceAll(const std::string& input, const std::string& placeholder, const std::string& replacement)
{
    if (placeholder.empty() || input.empty())
        return input;

    std::string result;
    result.reserve(input.size());
    std::size_t pos = 0;
    while (true)
    {
        std::size_t next = input.find(placeholder, pos);
        if (next == std::string::npos)
        {
            result.append(input, pos, input.size() - pos);
            break;
        }
        result.append(input, pos, next - pos);
        result.append(replacement);
        pos = next + placeholder.size();
    }
    return result;
}

}} // namespace sk::logger
