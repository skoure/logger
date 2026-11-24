
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
sk::logger::LoggerFactory::configureFromJsonFile("config/logger.json");
```

For programmatic output to an ostream across all loggers (including loggers with
additivity disabled), use `LoggerFactory::addOstreamSink()` — see
[CONFIGURATION.md](CONFIGURATION.md).

See [CONFIGURATION.md](CONFIGURATION.md) for the full JSON schema, pattern
token reference, and per-backend translation tables.

> Note: avoid calling logging or creating markers from static object destructors — logging during static teardown is unsafe.

---

## Markers

Markers are named tags that categorise individual log events.

```cpp
#include <logger/MarkerFactory.h>

auto marker = sk::logger::MarkerFactory::getMarker("SQL");

logger->info(*marker, "SELECT completed in %d ms", elapsed);
logger->error(*marker, "Query failed", ex);
```

### Attaching Multiple Markers

An event can carry more than one marker when it belongs to multiple categories,
such as both `SQL` and `SLOW_QUERY`:

```cpp
#include <logger/LoggerFactory.h>
#include <logger/MarkerFactory.h>

auto logger = sk::logger::LoggerFactory::getLogger("App.Database");
auto sql = sk::logger::MarkerFactory::getMarker("SQL");
auto slow = sk::logger::MarkerFactory::getMarker("SLOW_QUERY");

logger->info({*sql, *slow}, "Query completed in %d ms", 142);
logger->error({*sql, *slow}, "Query failed", ex);
```

### Marker Pattern Output

The `%M` pattern token expands to marker names in event order, or to an empty
string when no marker is attached. Bare `%M` includes every attached marker,
joined with `", "`. Add braces to select markers or change the separator:

```text
%M{-:ACTIVITY-LOG;sep=|}
%M{+:OP-LOG,SECURITY;sep= / }
%M{OP-LOG,SECURITY;sep=,}
%M{;sep=|}
```

`+:` includes the named markers, `-:` excludes them, and a bare list defaults
to include mode. Matching is by exact marker name, with output retaining event
order. Matching quotes around `sep=` are stripped; unquoted spaces are
preserved. Invalid `%M{...}` syntax in JSON configuration raises
`PatternLayoutParseException`. In programmatic SimpleLogger patterns, malformed
marker options fall back to bare `%M` behavior: all attached markers with the
default `", "` separator. Any width modifier on the token still applies.

Per-sink filters can select events by marker, exact logger name, or a regular
expression searched against the raw message body. JSON filter configuration and
programmatic `FilterFactory` examples are documented in
[CONFIGURATION.md](CONFIGURATION.md).

---

## Customising Level Names

By default the `%p` pattern token outputs uppercase level names consistent across
all three backends: `FATAL`, `ERROR`, `WARN`, `INFO`, `DEBUG`, `TRACE`.

To override any or all of the six names, call `LoggerFactory::setLevelNames()` once
at startup before any logging takes place:

```cpp
#include <logger/LoggerFactory.h>
#include <logger/LevelNames.h>

int main() {
  // Optional — override any or all level names before any logging
  sk::logger::LevelNames names;
  names.warn = "WARNING";   // only the fields you want to change
  sk::logger::LoggerFactory::setLevelNames(names);

  auto logger = sk::logger::LoggerFactory::getLogger("App");
  logger->warn("disk usage at %d%%", 95);  // prints "WARNING" in the level column
  // ...
}
```

The strings pointed to by the `LevelNames` struct must outlive all subsequent logging calls.

---

## Rate-Limited Logging

`RateLimitedLogger` provides backend-independent, thread-safe rate limiting at individual log statements. It wraps any configured `LoggerPtr` and preserves all configured sinks, patterns, markers, and exception stacktraces.

### Timed Quota

Admits up to `maxRate` calls per fixed/reset time window (defaults to 1 admitted call per window if `maxRate` is omitted):

```cpp
#include <logger/LoggerFactory.h>
#include <logger/RateLimitedLogger.h>

void retryRequest(const std::string& reqId) {
    auto logger = sk::logger::LoggerFactory::getLogger("Network");

    // Retained as a function-local static handle for statement-level state
    static auto throttled = sk::logger::RateLimitedLogger::forLogger(logger)
        .every(std::chrono::seconds{10})
        .maxRate(5)
        .build();

    // Admits first 5 calls every 10 seconds; suppresses remainder
    throttled->warn("Retrying request %s; {suppressed} repeats suppressed", reqId.c_str());
}
```

### Call Sampling

Admits every Nth call (calls 1, 1+N, 1+2N, ...):

```cpp
static auto sampled = sk::logger::RateLimitedLogger::forLogger(logger)
    .every(10)
    .build();

sampled->info("Processing batch item %d; {suppressed} skipped", itemId);
```

### `{suppressed}` Placeholder

The optional `{suppressed}` placeholder in format strings and exception context messages is replaced with the number of calls suppressed since the previous admitted message. The counter resets on every admitted message, even if `{suppressed}` is omitted.

### Lazy Lambda Evaluation (Zero Overhead When Suppressed)

To avoid eager argument evaluation for expensive computations, pass a callable returning a string:

```cpp
// Lambda is executed ONLY when admitted (never when suppressed or level disabled)
throttled->warn([&] {
    return "Expensive diagnostic: " + generateHeavyDiagnostics();
});

// Can also receive the suppressed count directly:
throttled->warn([&](int suppressed) {
    return "Dropped " + std::to_string(suppressed) + " requests: " + generateHeavyDiagnostics();
});
```

### Polymorphic Compatibility

`RateLimitedLogger` directly implements the `sk::logger::Logger` interface. The `RateLimitedLoggerPtr` returned by `build()` is a `std::shared_ptr<RateLimitedLogger>` and implicitly converts to `sk::logger::LoggerPtr`:

```cpp
void auditOperation(sk::logger::LoggerPtr log);

auditOperation(throttled); // Valid! Full polymorphic compatibility
```

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

**Linux (Release):**
```sh
cmake --preset linux
cmake --build --preset linux-release
ctest --preset linux-test
```

**Linux (Debug with `_GLIBCXX_DEBUG`):**

If Conan dependencies were installed with `_GLIBCXX_DEBUG` (see the Conan note above), pass the
matching define at configure time so the STL container ABIs agree:
```sh
cmake --preset linux -DCMAKE_CXX_FLAGS_DEBUG="-D_GLIBCXX_DEBUG"
cmake --build --preset linux-debug
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
