/**
 * @file TestUtils.cpp
 * @brief Cross-platform helpers for the logger test suites.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: April 25, 2026
 */
#include "TestUtils.h"

#include <cstdio>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

namespace sk::logger::test {
namespace {

#ifdef _WIN32
inline int sk_pipe(int fds[2])  { return ::_pipe(fds, 4096, _O_BINARY); }
inline int sk_dup(int fd)       { return ::_dup(fd); }
inline int sk_dup2(int a, int b){ return ::_dup2(a, b); }
inline int sk_close(int fd)     { return ::_close(fd); }
inline int sk_read(int fd, void* buf, unsigned n) { return ::_read(fd, buf, n); }
inline int sk_stdout_fd()       { return ::_fileno(stdout); }
#else
inline int sk_pipe(int fds[2])  { return ::pipe(fds); }
inline int sk_dup(int fd)       { return ::dup(fd); }
inline int sk_dup2(int a, int b){ return ::dup2(a, b); }
inline int sk_close(int fd)     { return ::close(fd); }
inline int sk_read(int fd, void* buf, unsigned n)
                                { return static_cast<int>(::read(fd, buf, n)); }
inline int sk_stdout_fd()       { return STDOUT_FILENO; }
#endif

} // namespace

std::string captureStdout(std::function<void()> fn)
{
    int pipefd[2];
    if (sk_pipe(pipefd) != 0) return {};

    const int stdoutFd = sk_stdout_fd();
    int saved = sk_dup(stdoutFd);
    if (saved < 0) { sk_close(pipefd[0]); sk_close(pipefd[1]); return {}; }

    sk_dup2(pipefd[1], stdoutFd);
    sk_close(pipefd[1]);

    fn();
    std::fflush(stdout);

    sk_dup2(saved, stdoutFd);
    sk_close(saved);

    char buf[512] = {};
    int n = sk_read(pipefd[0], buf, sizeof(buf) - 1);
    sk_close(pipefd[0]);
    return n > 0 ? std::string(buf, static_cast<std::size_t>(n)) : std::string{};
}

} // namespace sk::logger::test
