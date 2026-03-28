
# logger
C++ Logging Facade

## Purpose

This project provides a modern C++ logging facade that unifies multiple logging implementations (such as SimpleLogger, Log4cxx, and spdlog) under a single, consistent API. The facade enables you to:

- Switch logging implementations without changing application code
- Centralize logger configuration and management
- Write portable, testable, and maintainable logging code
- Integrate advanced features from different implementations as needed

It is ideal for projects that require flexibility, cross-platform support, and robust logging infrastructure.

## Example Usage

```cpp
#include <logger/LoggerFactory.h>

using namespace sk::logger;

int main() {
  // No backend initialization needed — the backend auto-registers
  // when you link to logger_simple_auto, logger_spdlog_auto, or logger_log4cxx_auto

  // Obtain a logger instance by name
  LoggerPtr logger = LoggerFactory::getInstance().getLogger("App.Database");

  // Set log level
  logger->setLevel(Logger::Level::Info);

  // Log messages at different levels
  logger->info("Hello %s!", "world");
  logger->error("Something went wrong: %d", 42);
  logger->debug("Debugging value: %f", 3.14);

  // Log an exception with context message and stacktrace
  try {
    // ... some operation ...
  } catch (const std::exception& ex) {
    logger->error("Failed to open config file", ex);
    logger->fatal("Unrecoverable error", ex);
  }

  return 0;
}
```

## Runtime Configuration

The facade supports JSON-driven configuration of logger levels and output sinks.

```cpp
#include <logger/LoggerFactory.h>

// Call once at startup, before retrieving any loggers
sk::logger::LoggerFactory::configure("config/logger.json");
```

See [CONFIGURATION.md](CONFIGURATION.md) for the full JSON schema, pattern
token reference, and per-backend translation tables.

---

## Markers

Markers are named tags that categorise individual log events.

```cpp
#include <logger/MarkerFactory.h>

auto marker = sk::logger::MarkerFactory::getMarker("SQL");

logger->info(*marker, "SELECT completed in %d ms", elapsed);
logger->error(*marker, "Query failed", ex);
```

The `%M` pattern token in a configured sink pattern expands to the marker name
(or an empty string when no marker is attached to the event).

---

## How to Link

The build produces an auto-registering wrapper target for each backend. Link exactly one:

| Backend      | CMake target           |
|--------------|------------------------|
| SimpleLogger | `logger_simple_auto`   |
| spdlog       | `logger_spdlog_auto`   |
| Log4cxx      | `logger_log4cxx_auto`  |

The `_auto` target carries `logger_core` as a transitive dependency — no need to list it separately.

CMake example:
```cmake
find_package(logger REQUIRED)
target_link_libraries(my_app PRIVATE logger_spdlog_auto)
```

## Build Instructions

### 1. Install Conan (v2)

```sh
pip install conan
```
Or see the official docs: https://docs.conan.io/2/installation.html

### 2. Install dependencies with Conan

A single Conan install fetches all backend dependencies (spdlog, log4cxx, gtest) at once.

**Available option:**
- `with_cpptrace` (default: True) — enables exception stacktrace capture via [cpptrace](https://github.com/jeremy-rifkin/cpptrace)

**Linux:**
```sh
conan install ./conan/conanfile.py --output-folder=build/conan --build=missing \
    --profile:all=./conan/profile/linux -s:a build_type=Release
```

**Windows:**
```sh
conan install ./conan/conanfile.py --output-folder=build/conan --build=missing \
    --profile:all=./conan/profile/windows -s:a build_type=Release
```

**Disable cpptrace** (omits stacktrace from exception logging):
```sh
conan install ./conan/conanfile.py --output-folder=build/conan --build=missing \
    --profile:all=./conan/profile/linux -s:a build_type=Release -o="&:with_cpptrace=False"
```

**Note for Linux debug builds:**
If your application or tests are built with `_GLIBCXX_DEBUG`, run Conan with:
```sh
conan install ./conan/conanfile.py --output-folder=build/conan --build=missing \
    --profile:all=./conan/profile/linux -s:a build_type=Debug \
    -c tools.build:defines=["_GLIBCXX_DEBUG"]
```

**Note for stacktrace support (cpptrace):**
Stacktrace frame names are only meaningful with debug symbols. Use `Debug` or `RelWithDebInfo`.

### 3. Configure, build, and test with CMake

All available backends are built automatically based on what Conan installed.

**Linux:**
```sh
cmake --preset linux
cmake --build --preset linux-release
ctest --preset linux-test
```

**Windows:**
```sh
cmake --preset windows
cmake --build --preset windows-release
ctest --preset windows-test
```

This produces up to three library artifacts:
- `build/linux/Release/liblogger_simple.a`
- `build/linux/Release/liblogger_spdlog.a`
- `build/linux/Release/liblogger_log4cxx.a`

And runs three separate test executables:
- `logger_tests_simple`
- `logger_tests_spdlog`
- `logger_tests_log4cxx`

### 4. Generate documentation

**Linux:**
```sh
cmake --build ./build/linux --target doc
```

**Windows:**
```sh
cmake --build ./build/windows --target doc
```

For more details, see the [CMakePresets.json](CMakePresets.json) and [conan/conanfile.py](conan/conanfile.py) files.
