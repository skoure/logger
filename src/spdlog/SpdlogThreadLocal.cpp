/**
 * @file SpdlogThreadLocal.cpp
 * @brief Thread-local storage definitions for the spdlog formatter bridge.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: March 28, 2026
 */
#include "SpdlogThreadLocal.h"

namespace sk { namespace logger { namespace spdlog_tls {

thread_local const char* markerName = nullptr;
thread_local std::string threadName;

}}} // namespace sk::logger::spdlog_tls
