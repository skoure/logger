# Logger Configuration Guide

This document describes how to configure the logger facade at runtime using a
JSON configuration file.

---

## Quick Start

```cpp
#include <logger/LoggerFactory.h>

// Apply configuration before retrieving any loggers
sk::logger::LoggerFactory::configure("config/logger.json");

// Now retrieve a logger — its level and sinks are already set
auto log = sk::logger::LoggerFactory::getLogger("App");
log->info("Application started");
```

---

## JSON Configuration Format

```json
{
  "loggers": [
    {
      "name":  "root",
      "level": "INFO",
      "sinks": [
        {
          "type":    "console",
          "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n"
        }
      ]
    },
    {
      "name":  "App.Database",
      "level": "DEBUG",
      "sinks": [
        {
          "type":    "rotating_file",
          "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] [%M] %m%n",
          "properties": {
            "path":      "logs/database.log",
            "max_size":  "10485760",
            "max_files": "3"
          }
        }
      ]
    }
  ]
}
```

---

## Logger Entry Fields

| Field   | Required | Description                                              |
|---------|----------|----------------------------------------------------------|
| `name`  | yes      | Logger name, e.g. `"root"`, `"App"`, `"App.Database"`. |
| `level` | yes      | Severity threshold (see table below).                   |
| `sinks` | yes      | Array of sink objects (may be empty).                   |

### Log Levels

| JSON value | Logger::Level         |
|------------|-----------------------|
| `"FATAL"`  | `Logger::Level::Fatal` |
| `"ERROR"`  | `Logger::Level::Error` |
| `"WARN"`   | `Logger::Level::Warn`  |
| `"INFO"`   | `Logger::Level::Info`  |
| `"DEBUG"`  | `Logger::Level::Debug` |
| `"TRACE"`  | `Logger::Level::Trace` |

---

## Sink Types

### `console`

Writes to standard output with optional ANSI color coding.

```json
{
  "type":    "console",
  "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n"
}
```

| Property | Required | Default | Description |
|----------|----------|---------|-------------|
| `color`  | no       | backend-dependent (see below) | Set to `"true"` to enable ANSI color coding, `"false"` to disable. |

> **Backend support for `color`:**
> - **spdlog**: supported. Default is `"false"` (color off). Uses `stdout_color_sink_mt` when
>   enabled, `stdout_sink_mt` when disabled.
> - **log4cxx**: supported. Default is `"false"` (color off). When enabled, wraps the pattern
>   with `%Y`/`%y` (level-based ANSI colors via `PatternLayout`).
> - **SimpleLogger**: supported. Default is `"false"` (color off). When enabled, wraps each
>   log line with level-based ANSI escape codes.

### `file`

Appends to a plain text file.

```json
{
  "type":    "file",
  "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n",
  "properties": {
    "path": "logs/app.log"
  }
}
```

| Property | Required | Description                  |
|----------|----------|------------------------------|
| `path`   | yes      | Path to the output log file. |

### `rotating_file`

Appends to a file that rotates when it reaches `max_size` bytes.

```json
{
  "type":    "rotating_file",
  "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n",
  "properties": {
    "path":      "logs/app.log",
    "max_size":  "10485760",
    "max_files": "5"
  }
}
```

| Property    | Required | Default    | Description                             |
|-------------|----------|------------|-----------------------------------------|
| `path`      | yes      | —          | Path to the output log file.            |
| `max_size`  | no       | `10485760` | Maximum file size in bytes (10 MiB).    |
| `max_files` | no       | `3`        | Number of rotated files to keep.        |

> **Note (SimpleLogger backend):** The SimpleLogger backend does not support
> log rotation.  A `rotating_file` sink is treated as a plain `file` sink when
> the SimpleLogger backend is active.

---

## Pattern Tokens

Patterns use a canonical log4j-style syntax.  Each backend translates these
tokens into its own native format.

| Token            | Meaning                                     |
|------------------|---------------------------------------------|
| `%m`             | Log message                                 |
| `%p`             | Log level name (`INFO`, `WARN`, …)          |
| `%c`             | Logger name                                 |
| `%t`             | Thread identifier                           |
| `%T`             | Thread name (OS-level name)                 |
| `%M`             | Marker name (empty if no marker set)        |
| `%d{strftime}`   | Timestamp formatted with a strftime string  |
| `%n`             | Newline                                     |
| `%%`             | Literal `%` (SimpleLogger only)             |

### Date format examples

| Pattern                       | Example output              |
|-------------------------------|-----------------------------|
| `%d{%Y-%m-%d %H:%M:%S}`      | `2026-03-23 14:05:30`       |
| `%d{%H:%M:%S}`               | `14:05:30`                  |
| `%d{%Y/%m/%d}`               | `2026/03/23`                |

---

## Width and Alignment Modifiers

Any token (except `%n` and `%%`) can be prefixed with an optional modifier
that controls field width and alignment.  The full syntax is:

```
%[-][min-width][.max-width]token
```

| Part         | Description                                                         |
|--------------|---------------------------------------------------------------------|
| `-`          | Left-align the field (default is right-align).                      |
| `min-width`  | Pad the value with spaces to at least this many characters.         |
| `.max-width` | Truncate the value to at most this many characters (from the left). |

Padding always uses spaces.  Truncation removes leading characters.

### Modifier examples

| Pattern    | Value        | Output         | Notes                            |
|------------|--------------|----------------|----------------------------------|
| `%-5p`     | `INFO`       | `INFO `        | Left-align, pad to 5 chars       |
| `%-5p`     | `WARN`       | `WARN `        | Left-align, pad to 5 chars       |
| `%-5p`     | `FATAL`      | `FATAL`        | Exact fit — no padding           |
| `%10m`     | `hello`      | `     hello`   | Right-align, pad to 10 chars     |
| `%-10M`    | `SQL`        | `SQL       `   | Left-align marker to 10 chars    |
| `%-10M`    | *(none)*     | `          `   | Empty marker — 10 spaces         |
| `%.20c`    | `App.Database.SQL` | `atabase.SQL` | Truncate from left to 20 chars |
| `%-20.20c` | `App`        | `App                 ` | Left-align, min+max 20 chars |

### Typical use: fixed-width level field

All built-in level names are at most 5 characters (`FATAL`, `ERROR`, `TRACE`,
`DEBUG`).  Use `%-5p` to ensure every level field is exactly 5 characters wide,
keeping subsequent fields column-aligned:

```
[2026-03-29 14:05:30] [INFO ] [SQL       ] msg...
[2026-03-29 14:05:30] [FATAL] [          ] msg...
[2026-03-29 14:05:30] [DEBUG] [SQL       ] msg...
```

### Backend notes

| Backend      | Modifier support                                                    |
|--------------|---------------------------------------------------------------------|
| SimpleLogger | Full support — padding and truncation applied in C++.               |
| spdlog       | Full support for all tokens except `%d` (see note below).           |
| log4cxx      | Full support — modifiers are passed through in native log4j syntax. |

> **spdlog `%d` limitation:** spdlog inlines strftime tokens directly into its
> pattern string, so a width modifier on `%d{...}` cannot be applied to the
> resulting multi-token sequence.  The modifier is silently dropped for spdlog.
> Use SimpleLogger or log4cxx if you need a fixed-width timestamp field.

---

## Markers

Markers are named tags that can be attached to individual log events.
They are useful for filtering and correlating log output.

```cpp
#include <logger/MarkerFactory.h>

auto sqlMarker = sk::logger::MarkerFactory::getMarker("SQL");
log->debug(*sqlMarker, "SELECT * FROM users WHERE id=%d", userId);
```

The `%M` pattern token expands to the marker name when a marker is present,
or to an empty string otherwise.

### Per-backend marker support

| Backend      | Mechanism                                      |
|--------------|------------------------------------------------|
| SimpleLogger | `%M` expanded in `SimpleLoggerPattern::render` |
| spdlog       | Thread-local bridge → custom `%&` flag         |
| Log4cxx      | MDC key `"marker"` → `%X{marker}` in pattern  |

---

## Backend Token Translation

Each backend translates the canonical pattern into its own format:

### spdlog

| Canonical | spdlog    |
|-----------|-----------|
| `%m`      | `%v`      |
| `%p`      | `%l`      |
| `%c`      | `%n`      |
| `%t`      | `%t`      |
| `%T`      | `%*` (custom) |
| `%M`      | `%&` (custom) |
| `%d{fmt}` | `fmt` inlined |
| `%n`      | `\n`      |

### log4cxx

| Canonical   | log4cxx          |
|-------------|------------------|
| `%m`        | `%m`             |
| `%p`        | `%p`             |
| `%c`        | `%c`             |
| `%t`        | `%t`             |
| `%T`        | `%t`             |
| `%M`        | `%X{marker}`     |
| `%d{strfmt}`| `%d{java-fmt}`   |
| `%n`        | `%n`             |

---

## Programmatic Sink Configuration

For situations where you need log output captured in memory — such as unit tests,
in-process diagnostics, or custom stream wrappers — use the programmatic API instead
of a JSON file:

```cpp
#include <logger/LoggerFactory.h>
#include <sstream>

std::ostringstream buffer;
sk::logger::LoggerFactory::configureLoggerWithOstream(
    "App.Database", buffer, "[%p] [%c] %m%n");

auto log = sk::logger::LoggerFactory::getLogger("App.Database");
log->info("Connected");

std::string output = buffer.str();  // e.g. "[INFO] [App.Database] Connected\n"
```

The canonical pattern syntax is identical to the JSON configuration format (see
[Pattern Tokens](#pattern-tokens) above).

> **Lifetime:** The `std::ostream` passed to `configureLoggerWithOstream` is held
> by non-owning reference.  Ensure it outlives every logger configured against it.

This API is supported by all three backends (simple, spdlog, log4cxx).

---

## Calling `configure()` Multiple Times

`LoggerFactory::configure()` may be called more than once.  Each call
re-applies the configuration: logger levels are reset and sinks are replaced.
This is safe but not recommended in production; prefer a single configuration
call at startup.

---

## Error Handling

`LoggerFactory::configure()` throws `std::runtime_error` if:

- The specified file does not exist or cannot be opened.
- The file contains invalid JSON.

All other conditions (unknown logger names, unknown sink types, empty sink
arrays) are handled gracefully without throwing.
