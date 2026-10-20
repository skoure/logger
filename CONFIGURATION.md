# Logger Configuration Guide

This document describes how to configure the logger facade at runtime using a
JSON configuration file.

---

## Quick Start

```cpp
#include <logger/LoggerFactory.h>

// Apply configuration before retrieving any loggers
sk::logger::LoggerFactory::configureFromJsonFile("config/logger.json");

// Now retrieve a logger — its level and sinks are already set
auto log = sk::logger::LoggerFactory::getLogger("App");
log->info("Application started");
```

> Note: avoid calling logging or creating markers from static object destructors — logging during static teardown is unsafe.

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
            "max_size":  10485760,
            "max_files": 3
          }
        }
      ]
    }
  ]
}
```

---

## Logger Entry Fields

| Field        | Required | Description                                              |
|--------------|----------|----------------------------------------------------------|
| `name`       | yes      | Logger name, e.g. `"root"`, `"App"`, `"App.Database"`. |
| `level`      | yes      | Severity threshold (see table below).                   |
| `flush_on`   | no       | Minimum severity that triggers an immediate flush after each append (see below). |
| `additivity` | no       | Boolean flag controlling sink inheritance from parent (default: `true`). |
| `sinks`      | yes      | Array of sink objects (may be empty).                   |

### Log Levels

| JSON value | Logger::Level         |
|------------|-----------------------|
| `"FATAL"`  | `Logger::Level::Fatal` |
| `"ERROR"`  | `Logger::Level::Error` |
| `"WARN"`   | `Logger::Level::Warn`  |
| `"INFO"`   | `Logger::Level::Info`  |
| `"DEBUG"`  | `Logger::Level::Debug` |
| `"TRACE"`  | `Logger::Level::Trace` |

### flush_on

The optional `flush_on` field sets a minimum severity threshold that causes the
logger to flush its underlying output stream immediately after each log event at
or above that level.

| JSON value | Flushes on                                   |
|------------|----------------------------------------------|
| `"FATAL"`  | Fatal events only                            |
| `"ERROR"`  | Error and Fatal events                       |
| `"WARN"`   | Warn, Error, and Fatal events                |
| `"INFO"`   | Info, Warn, Error, and Fatal events          |
| `"DEBUG"`  | Debug, Info, Warn, Error, and Fatal events   |
| `"TRACE"`  | All events                                   |

**Default behaviour:** if `flush_on` is omitted, no automatic flushing is
performed after individual log calls.  The backend's default buffering policy
applies.

**Inheritance:** a logger without an explicit `flush_on` inherits the threshold
of its nearest ancestor that has one set.

**Backend notes:**

| Backend      | Flush mechanism                                                             |
|--------------|-----------------------------------------------------------------------------|
| SimpleLogger | `std::ostream::flush()` called on each configured sink stream after append. |
| spdlog       | `spdlog::logger::flush()` called after append — flushes all attached sinks. |
| log4cxx      | `immediateFlush` set to `true`/`false` on each `WriterAppender` before each append, overriding the default-`true` behaviour so only threshold-crossing writes flush. |

**Example:**

```json
{
  "name":     "App.Database",
  "level":    "WARN",
  "flush_on": "WARN",
  "sinks": [
    {
      "type":    "rotating_file",
      "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n",
      "properties": {
        "path":      "logs/database.log",
        "max_size":  5242880,
        "max_files": 5
      }
    }
  ]
}
```

With `"flush_on": "WARN"`, every WARN, ERROR, and FATAL event written to the
rotating file is flushed immediately.  INFO and DEBUG events remain buffered.

---

## Additivity

The optional `additivity` field controls whether a logger inherits sinks from its parent
logger during configuration. 

**Default behaviour:** if `additivity` is omitted or set to `true`, a logger that has no
explicitly configured sinks will inherit all sinks from its parent logger. This inheritance
walks up the hierarchy, so a deeply-nested logger can receive sinks from any ancestor.

**When `additivity` is `false`:** a logger uses only its explicitly configured sinks and
does **not** inherit parent sinks, even if the parent has them. Child loggers of this logger
still inherit from it according to their own additivity flags.

**Example:**

Consider this hierarchy and configuration:

```
root
├── App
│   ├── App.Database
│   └── App.Security
└── Framework
```

```json
{
  "loggers": [
    {
      "name":  "root",
      "level": "INFO",
      "sinks": [
        {"type": "console", "pattern": "[%p] %m%n"}
      ]
    },
    {
      "name":      "App.Database",
      "level":     "DEBUG",
      "additivity": false,
      "sinks": [
        {
          "type":    "file",
          "pattern": "[%d] [%p] %m%n",
          "properties": {"path": "logs/database.log"}
        }
      ]
    },
    {
      "name":  "App.Security",
      "level": "DEBUG"
    }
  ]
}
```

**Results:**

- `root` logs to console
- `App` logs to console (inherits from `root`, no explicit sinks)
- `App.Database` logs **only** to file (additivity=false, does NOT inherit console sink from root)
- `App.Security` logs to console (inherits from parent `App`, which inherited from `root`)
- `Framework` logs to console (inherits from `root`)

**Backend support:**

| Backend  | Implementation |
|----------|---|
| SimpleLogger | Child logger's sinks are copied from parent during configuration if additivity is true and child has no explicit sinks. |
| spdlog       | Child logger's sinks vector is assigned from parent during configuration if additivity is true. |
| log4cxx      | The logger's native `setAdditivity()` is called with the configured value, controlling whether events propagate to parent appenders. |

---

## Sink Types

> **Property value types:** All `properties` values accept JSON strings, booleans, or integers.
> For example, `"color": true` and `"color": "true"` are both valid; `"max_size": 10485760`
> and `"max_size": "10485760"` are both valid.

### Sink-Level Threshold

Each sink accepts an optional `level` field that acts as a per-sink minimum severity filter.
When set, only log events at that severity **or more severe** are written to that sink.
The sink threshold is applied **after** the logger-level gate, so it can only be equally or
more restrictive than the logger's own level — it cannot cause a sink to receive events the
logger has already suppressed.

| Field   | Type   | Required | Description |
|---------|--------|----------|-------------|
| `level` | string | no       | Minimum severity for this sink: `FATAL`, `ERROR`, `WARN`, `INFO`, `DEBUG`, `TRACE`. Omit to apply no additional filtering beyond the logger's level. |

**No hierarchy:** sink-level thresholds are flat — there is no parent/child inheritance
between sinks.

**Example:** console shows only WARN and above; file captures everything the logger allows (DEBUG+):

```json
{
  "name":  "root",
  "level": "DEBUG",
  "sinks": [
    {
      "type":    "console",
      "level":   "WARN",
      "pattern": "[%d{%H:%M:%S}] [%p] %m%n"
    },
    {
      "type":    "file",
      "level":   "DEBUG",
      "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] [%c] %m%n",
      "properties": { "path": "logs/app.log" }
    }
  ]
}
```

**Backend notes:**

| Backend      | Mechanism                                                       |
|--------------|-----------------------------------------------------------------|
| SimpleLogger | Level check applied in `append()` before writing to each sink stream. |
| spdlog       | `sink->set_level()` called on each spdlog sink during configuration. |
| log4cxx      | `AppenderSkeleton::setThreshold()` called on each appender during configuration. |

---

### Sink Filters

Each configured sink can have an ordered `filters` array. Every filter must
pass for the sink to write an event (AND composition). Evaluation follows the
array order and stops at the first rejection. Put inexpensive checks, such as
marker or logger-name filters, before message-regex filters to avoid unnecessary
regex searches. The sink's `level` threshold is a separate gate.

| Type | Fields | Matching behavior |
|------|--------|-------------------|
| `marker` | `names`, `mode` | Match when any event marker exactly and case-sensitively equals any configured name. |
| `logger_name` | `names`, `mode` | Match when the full logger name exactly and case-sensitively equals any configured name. Names do not imply hierarchy prefixes. |
| `message_regex` | `pattern`, `mode` | Search the raw message body using a C++ ECMAScript regular expression (`std::regex_search`). |

For `marker` and `logger_name`, `names` may be one string or an array of
strings. Whitespace is trimmed and empty entries are ignored. An omitted or
empty name list imposes no filtering. The `mode` field accepts `include` or
`exclude`, case-insensitively, and defaults to `include` for all filter types.
Include keeps matching events; exclude drops matching events. In exclude mode,
unmarked events pass a marker filter.

`message_regex` requires a string `pattern`. Search finds a matching substring;
use anchors such as `^timeout` to require a beginning-of-message match, or
`^timeout$` to require a full-message match. An empty pattern is valid and
matches every message. Invalid patterns raise `ParseException` while parsing
configuration.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `filters` | array | no | Ordered filters for this sink. Omit or use an empty array for no additional filtering. |
| `filters[].type` | string | yes | `marker`, `logger_name`, or `message_regex`. |
| `filters[].names` | array or string | no | Names for `marker` or `logger_name` filters. |
| `filters[].pattern` | string | yes for `message_regex` | ECMAScript regular expression searched against the message body. |
| `filters[].mode` | string (`include`\|`exclude`) | no | Include or exclude matching events; defaults to `include`. |

**Example:** write only security-tagged messages from the exact `App.Security`
logger when the raw message mentions a denied request; the console excludes
`NOISY_SQL` markers:

```json
{
  "name": "root",
  "level": "DEBUG",
  "sinks": [
    {
      "type": "file",
      "pattern": "%m%n",
      "filters": [
        { "type": "marker", "names": "SECURITY" },
        { "type": "logger_name", "names": ["App.Security"] },
        { "type": "message_regex", "pattern": "denied|unauthorized" }
      ],
      "properties": { "path": "logs/audit.log" }
    },
    {
      "type": "console",
      "pattern": "[%p] %m%n",
      "filters": [
        { "type": "marker", "names": ["NOISY_SQL"], "mode": "exclude" }
      ]
    }
  ]
}
```

**Backend notes:**

| Backend | Mechanism |
|---------|-----------|
| SimpleLogger | Evaluates the configured filters against the shared log record before writing to each sink. |
| spdlog | A sink wrapper evaluates logger name, payload, and marker metadata before delegating. |
| log4cxx | An appender filter evaluates logger name, message, and marker MDC metadata. |

---

### `console`

Writes to standard output with optional ANSI color coding.

```json
{
  "type":    "console",
  "pattern": "[%d{%Y-%m-%d %H:%M:%S}] [%p] %m%n",
  "properties": {
    "color": true
  }
}
```

| Property | Required | Default | Description |
|----------|----------|---------|-------------|
| `color`  | no       | backend-dependent (see below) | JSON boolean `true`/`false` (or string `"true"`/`"false"`) to enable/disable ANSI color coding. |

> **Backend support for `color`:**
> - **spdlog**: supported. Default is `false` (color off). Uses `stdout_color_sink_mt` when
>   enabled, `stdout_sink_mt` when disabled.
> - **log4cxx**: supported. Default is `false` (color off). When enabled, wraps the pattern
>   with `%Y`/`%y` (level-based ANSI colors via `PatternLayout`).
> - **SimpleLogger**: supported. Default is `false` (color off). When enabled, wraps each
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
    "max_size":  10485760,
    "max_files": 5
  }
}
```

| Property    | Required | Default    | Description                             |
|-------------|----------|------------|-----------------------------------------|
| `path`      | yes      | —          | Path to the output log file.            |
| `max_size`  | no       | `10485760` | Maximum file size in bytes (10 MiB). JSON integer or string. |
| `max_files` | no       | `3`        | Number of rotated files to keep. JSON integer or string.     |

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
| `%n`             | Platform newline (`\n` on Unix, `\r\n` on Windows) |
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

Markers are named tags that can be attached to individual log events. They are useful
for filtering and correlating log output.

```cpp
#include <logger/MarkerFactory.h>

auto sqlMarker = sk::logger::MarkerFactory::getMarker("SQL");
log->debug(*sqlMarker, "SELECT * FROM users WHERE id=%d", userId);
```

The `%M` pattern token expands to the marker name when a marker is present,
or to an empty string otherwise.

For per-sink marker-based filtering, see the
[### Sink Filters](#sink-filters)
section in CONFIGURATION.md which documents the typed `filters` array,
include/exclude semantics, and per-backend integration.

### Per-backend marker support

| Backend      | Mechanism                                      |
|--------------|------------------------------------------------|
| SimpleLogger | `%M` expanded in `LogRecordPattern::render` |
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
| `%n`      | platform EOL literal |

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
of a JSON file. A registered ostream sink receives events from every logger,
including loggers with `additivity: false`:

```cpp
#include <logger/LoggerFactory.h>
#include <logger/FilterFactory.h>
#include <logger/MarkerFactory.h>
#include <sstream>

std::ostringstream buffer;
auto security = sk::logger::MarkerFactory::getMarker("SECURITY");
auto markerFilter = sk::logger::FilterFactory::createMarkerFilter(
  {security});
auto nameFilter = sk::logger::FilterFactory::createLoggerNameFilter(
  {"App.Security"});
auto messageFilter = sk::logger::FilterFactory::createRegexFilter(
  "denied|unauthorized");

sk::logger::LoggerFactory::addOstreamSink(
  buffer, "[%p] [%c] %m%n", {markerFilter, nameFilter, messageFilter},
  sk::logger::Logger::Level::Warn);

auto log = sk::logger::LoggerFactory::getLogger("App.Security");
log->warn(*security, "Request denied");
std::string output = buffer.str();

// Remove all global ostream sinks before their streams go out of scope.
sk::logger::LoggerFactory::clearOstreamSinks();
```

The canonical pattern syntax is identical to the JSON configuration format (see
[Pattern Tokens](#pattern-tokens) above).

`addOstreamSink()` has no logger-name argument. Each call adds an independent
global sink. Filters are evaluated in the supplied vector order and stop at the
first rejection; put inexpensive filters before regex filters when possible.
The optional fourth argument is the sink's level threshold and defaults to
`Logger::Level::Trace`, which accepts every level. A `Warn` threshold accepts
Fatal, Error, and Warn events while suppressing Info, Debug, and Trace. The level
threshold is checked before filters, and all filters must pass for the sink to
write an event.

The ostream remains non-owning. Keep it alive until `clearOstreamSinks()` has
removed the registration, no logging calls are in flight, and no further logging
will use it. `clearOstreamSinks()` removes all global ostream sink registrations;
it does not change logger-specific sinks configured through JSON.

Filters are factory-created and can be combined in the same vector:

```cpp
auto excludeNoisySql = sk::logger::FilterFactory::createMarkerFilter(
  {sk::logger::MarkerFactory::getMarker("NOISY_SQL")},
  sk::logger::FilterMode::Exclude);
auto includeTimeout = sk::logger::FilterFactory::createRegexFilter(
  "timed.?out", sk::logger::FilterMode::Include);
sk::logger::LoggerFactory::addOstreamSink(
  buffer, "[%p] [%c] %m%n", {excludeNoisySql, includeTimeout});
```

`createRegexFilter()` uses ECMAScript syntax and substring search. Add `^` or `$`
anchors to constrain the match position. Invalid patterns throw `std::regex_error`.
`createLoggerNameFilter()` matches full logger names exactly and case-sensitively;
it does not match descendants implicitly. The `mode` defaults to include for all
filter factories. With no filters, the global sink writes every event allowed by
the logger's level.

---

## Calling Configuration Methods Multiple Times

`LoggerFactory::configureFromJsonFile()` and `LoggerFactory::configureFromJsonString()`
may be called more than once.  Each call re-applies the configuration: logger
levels are reset and sinks are replaced. This is safe but not recommended in
production; prefer a single configuration call at startup.

---

## Error Handling

`LoggerFactory::configureFromJsonFile()` and `LoggerFactory::configureFromJsonString()`
throw `ParseException` (which extends `std::runtime_error`) if:

- The specified file does not exist or cannot be opened (file method only).
- The JSON is invalid.

All other conditions (unknown logger names, unknown sink types, empty sink
arrays) are handled gracefully without throwing.
