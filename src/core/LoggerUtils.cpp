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

std::string formatException(const char* msg, const std::exception& ex)
{
    std::ostringstream oss;
    if (msg && msg[0] != '\0') {
        oss << msg << ": ";
    }
    oss << demangle(typeid(ex).name()) << ": " << ex.what();
#ifdef USE_CPPTRACE
    oss << "\nStacktrace:\n";
    // skip=1 removes the formatException frame from the top of the trace
    oss << cpptrace::generate_trace(1).to_string();
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

}} // namespace sk::logger
