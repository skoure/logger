/**
 * @file TestUtils.h
 * @brief Shared cross-platform helpers for the logger test suites.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 25, 2026
 */
#ifndef SK_TEST_UTILS_H
#define SK_TEST_UTILS_H

#include <functional>
#include <string>

namespace sk::logger::test {

/**
 * @brief Captures the output written to stdout during the execution of a function.
 * 
 * Redirect stdout to a pipe, run fn(), restore stdout, and return the bytes
 * written to stdout while fn() ran. Returns an empty string on capture failure
 * or zero-byte output. Works on Windows and POSIX.
 * 
 * @param fn The function to execute while capturing stdout.
 * @return The captured output as a string.
 */
std::string captureStdout(std::function<void()> fn);

} // namespace sk::logger::test

#endif // SK_TEST_UTILS_H
