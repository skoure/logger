/**
 * @file RateLimitedLogger.h
 * @brief Backend-independent, per-statement rate-limited logger wrapper.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 22, 2026
 */
#ifndef SK_LOGGER_RATE_LIMITED_LOGGER_H
#define SK_LOGGER_RATE_LIMITED_LOGGER_H

#include <chrono>
#include <cstdarg>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <logger/Logger.h>
#include <logger/Marker.h>
#include <logger/MarkerList.h>

namespace sk { namespace logger {

class RateLimitedLogger;

/**
 * @typedef RateLimitedLoggerPtr
 * @brief Shared pointer type for RateLimitedLogger instances.
 */
typedef std::shared_ptr<RateLimitedLogger> RateLimitedLoggerPtr;

/**
 * @class RateLimitedLogger
 * @brief Backend-independent, per-statement rate-limiting wrapper for Logger instances.
 *
 * RateLimitedLogger provides thread-safe, statement-level rate limiting using either
 * a timed quota (fixed/reset time window) or call sampling (admitting every Nth call).
 *
 * Implementation details (state, clocks, counters, and mutexes) are kept opaque behind
 * a Private Implementation (PImpl) pattern. Each RateLimitedLogger handle maintains its
 * own independent, synchronized state, ensuring that rate limits at different call sites
 * do not interfere with one another.
 *
 * The wrapper retains a target LoggerPtr and faithfully implements the complete Logger
 * interface, routing admitted messages through the normal backend pipeline so that all
 * configured patterns, filters, and sinks remain in effect. Suppressed calls avoid format
 * string processing.
 *
 * An optional `{suppressed}` placeholder in format strings, exception context messages, or
 * returned lambda strings is automatically replaced with the number of calls suppressed
 * since the previous admitted message. The suppressed counter is always reset on each admitted call.
 *
 * For expensive arguments, template overloads accepting callables (lambdas) are provided.
 * The callable is only executed if the log level is enabled AND the call is admitted, completely
 * eliminating eager argument evaluation overhead when suppressed.
 *
 * RateLimitedLogger implements the Logger interface; the shared pointer returned by build()
 * implicitly converts to LoggerPtr:
 * @code
 *   static auto throttled = RateLimitedLogger::forLogger(logger)
 *                               .every(std::chrono::seconds{10})
 *                               .maxRate(5)
 *                               .build();
 *   throttled->warn("Retrying request %s; {suppressed} repeats suppressed", reqId);
 * @endcode
 */
class RateLimitedLogger : public Logger
{
public:
    /**
     * @class Builder
     * @brief Fluent builder for configuring and constructing RateLimitedLogger handles.
     */
    class Builder
    {
    public:
        /**
         * @brief Constructs a Builder associated with the target logger.
         * @param logger Target logger to wrap. Must not be null.
         * @throws std::invalid_argument If @p logger is null.
         */
        explicit Builder(LoggerPtr logger);

        Builder(const Builder&) = delete;
        Builder& operator=(const Builder&) = delete;

        /**
         * @brief Move constructor.
         * @param other Builder instance to move from.
         */
        Builder(Builder&& other) noexcept = default;

        /**
         * @brief Move assignment operator.
         * @param other Builder instance to move from.
         * @return Reference to this Builder.
         */
        Builder& operator=(Builder&& other) noexcept = default;

        /**
         * @brief Sets the maximum number of log calls admitted per fixed/reset window.
         *
         * Only valid with timed quota rate limiting (configured via every(duration)).
         * Defaults to 1 if every(duration) is specified without calling maxRate().
         * If combined with count-based every(int count), build() will throw std::invalid_argument.
         *
         * @param count Maximum calls admitted per window (must be > 0).
         * @return Reference to this Builder for method chaining.
         * @throws std::invalid_argument If count <= 0.
         */
        Builder& maxRate(int count);

        /**
         * @brief Sets the fixed/reset window duration for timed quota rate limiting.
         *
         * Each handle's first call starts its initial window; the first call after the
         * window duration expires starts the next window and is admitted. Setting this
         * activates TimedQuota mode and overrides any prior call to every(int).
         *
         * @param duration Fixed/reset window duration (must be positive).
         * @return Reference to this Builder for method chaining.
         * @throws std::invalid_argument If duration <= 0.
         */
        Builder& every(std::chrono::nanoseconds duration);

        /**
         * @brief Sets the fixed/reset window duration for timed quota rate limiting.
         *
         * Overload accepting any std::chrono::duration (e.g. std::chrono::seconds, milliseconds).
         *
         * @tparam Rep Arithmetic representation type for duration ticks.
         * @tparam Period std::ratio representing tick period.
         * @param duration Fixed/reset window duration (must be positive).
         * @return Reference to this Builder for method chaining.
         * @throws std::invalid_argument If duration <= 0.
         */
        template <typename Rep, typename Period>
        Builder& every(const std::chrono::duration<Rep, Period>& duration)
        {
            return every(std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
        }

        /**
         * @brief Configures call sampling rate limiting, admitting every Nth call.
         *
         * For interval N, calls 1, 1+N, 1+2N, etc. are admitted, and intermediate calls
         * are suppressed. Setting this activates CountSampling mode and overrides any prior
         * call to every(duration).
         *
         * @param count Sample interval (must be > 0). E.g. 10 admits calls 1, 11, 21...
         * @return Reference to this Builder for method chaining.
         * @throws std::invalid_argument If count <= 0.
         */
        Builder& every(int count);

        /**
         * @brief Injects a custom monotonic time source for deterministic testing of timed windows.
         *
         * When omitted, std::chrono::steady_clock::now is used.
         *
         * @param clockFn Callable returning the current std::chrono::steady_clock::time_point.
         * @return Reference to this Builder for method chaining.
         */
        Builder& clock(std::function<std::chrono::steady_clock::time_point()> clockFn);

        /**
         * @brief Builds and returns the finalized RateLimitedLogger handle.
         *
         * Creates an independent handle owning its synchronized rate-limiting state.
         * Typically stored in a function-local static variable at the log statement.
         *
         * @return Shared pointer to the finalized RateLimitedLogger handle.
         * @throws std::invalid_argument If configuration is invalid (e.g. null logger,
         *         every() not specified, or maxRate combined with count sampling).
         */
        RateLimitedLoggerPtr build();

    private:
        LoggerPtr m_logger;
        int m_maxRate{0};
        std::chrono::nanoseconds m_everyDuration{0};
        int m_everyCount{0};
        std::function<std::chrono::steady_clock::time_point()> m_clock;
    };

    /**
     * @brief Entry point for creating a RateLimitedLogger fluent builder.
     * @param logger Target logger to wrap. Must not be null.
     * @return Builder instance configured with the target logger.
     */
    static Builder forLogger(LoggerPtr logger);

    /**
     * @brief Destructor. Destroys the private implementation.
     */
    ~RateLimitedLogger() override;

    RateLimitedLogger(const RateLimitedLogger&) = delete;
    RateLimitedLogger& operator=(const RateLimitedLogger&) = delete;

    /**
     * @brief Move constructor.
     * @param other Handle to move from.
     */
    RateLimitedLogger(RateLimitedLogger&& other) noexcept;

    /**
     * @brief Move assignment operator.
     * @param other Handle to move from.
     * @return Reference to this RateLimitedLogger.
     */
    RateLimitedLogger& operator=(RateLimitedLogger&& other) noexcept;

    /**
     * @brief Returns the underlying wrapped logger.
     * @return LoggerPtr to the wrapped logger instance.
     */
    LoggerPtr getLogger() const;

    // -----------------------------------------------------------------------
    // Level management (Logger overrides - transparent forwarding)
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the name of the wrapped logger.
     * @return Logger name as a std::string.
     */
    std::string getName() const override;

    /**
     * @brief Returns the effective logging level of the wrapped logger.
     * @return Current effective log level.
     */
    Level getLevel() const override;

    /**
     * @brief Sets the logging level on the wrapped logger.
     * @param level New log level to set.
     */
    void setLevel(Level level) override;

    /**
     * @brief Reverts the wrapped logger to inherited level behaviour.
     */
    void clearLevel() override;

    /**
     * @brief Returns true if setLevel() has been called on the wrapped logger.
     * @return True if the level was explicitly set; false if inherited.
     */
    bool isLevelExplicitlySet() const override;

    // -----------------------------------------------------------------------
    // Flush threshold (Logger overrides - transparent forwarding)
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the flush threshold on the wrapped logger.
     * @param level Flush threshold level.
     */
    void setFlushOn(Level level) override;

    /**
     * @brief Removes the explicit flush threshold from the wrapped logger.
     */
    void clearFlushOn() override;

    /**
     * @brief Returns the effective flush threshold for the wrapped logger.
     * @return Flush threshold level, or nullopt if none is configured.
     */
    std::optional<Level> getFlushOn() const override;

    // -----------------------------------------------------------------------
    // Level checks (Logger overrides - transparent forwarding)
    // -----------------------------------------------------------------------

    /**
     * @brief Checks if FATAL level logging is enabled on the wrapped logger.
     * @return True if FATAL is enabled, false otherwise.
     */
    bool isFatalEnabled() const override;

    /**
     * @brief Checks if ERROR level logging is enabled on the wrapped logger.
     * @return True if ERROR is enabled, false otherwise.
     */
    bool isErrorEnabled() const override;

    /**
     * @brief Checks if WARN level logging is enabled on the wrapped logger.
     * @return True if WARN is enabled, false otherwise.
     */
    bool isWarnEnabled() const override;

    /**
     * @brief Checks if INFO level logging is enabled on the wrapped logger.
     * @return True if INFO is enabled, false otherwise.
     */
    bool isInfoEnabled() const override;

    /**
     * @brief Checks if DEBUG level logging is enabled on the wrapped logger.
     * @return True if DEBUG is enabled, false otherwise.
     */
    bool isDebugEnabled() const override;

    /**
     * @brief Checks if TRACE level logging is enabled on the wrapped logger.
     * @return True if TRACE is enabled, false otherwise.
     */
    bool isTraceEnabled() const override;

    // -----------------------------------------------------------------------
    // Standard printf-style logging methods
    // -----------------------------------------------------------------------

    /**
     * @brief Logs a formatted fatal message if FATAL is enabled and admitted by the rate limit.
     *
     * If FATAL level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload fatal(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void fatal(const char* fmt, ...) override;

    /**
     * @brief Logs a formatted error message if ERROR is enabled and admitted by the rate limit.
     *
     * If ERROR level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload error(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void error(const char* fmt, ...) override;

    /**
     * @brief Logs a formatted warning message if WARN is enabled and admitted by the rate limit.
     *
     * If WARN level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload warn(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void warn(const char* fmt, ...) override;

    /**
     * @brief Logs a formatted info message if INFO is enabled and admitted by the rate limit.
     *
     * If INFO level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload info(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void info(const char* fmt, ...) override;

    /**
     * @brief Logs a formatted debug message if DEBUG is enabled and admitted by the rate limit.
     *
     * If DEBUG level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload debug(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void debug(const char* fmt, ...) override;

    /**
     * @brief Logs a formatted trace message if TRACE is enabled and admitted by the rate limit.
     *
     * If TRACE level is disabled, returns immediately without modifying counters. If enabled,
     * checks rate-limiting admission. Admitted messages replace `{suppressed}` in @p fmt with
     * the count of calls suppressed since the last admitted message (resetting the count) and
     * forward the completed message to the wrapped logger. Suppressed calls skip formatting.
     *
     * @note Eager argument evaluation caveat: C++ evaluates function arguments before entry,
     *       so expressions in varargs are evaluated even when suppressed. For expensive
     *       arguments, use the lambda overload trace(Callable&&) instead.
     *
     * @param fmt printf-style format string, optionally containing `{suppressed}`.
     * @param ... Arguments formatted according to @p fmt.
     */
    void trace(const char* fmt, ...) override;

    /**
     * @brief Logs an exception at ERROR level with context if enabled and admitted.
     *
     * If ERROR is enabled and admitted, replaces `{suppressed}` in @p msg with the suppressed count,
     * resets the count, and delegates to the underlying logger's exception overload to preserve
     * stacktrace capture behavior. If suppressed, no logging occurs.
     *
     * @param msg Context message describing the operation that failed, optionally containing `{suppressed}`.
     * @param ex The exception to log.
     */
    void error(const char* msg, const std::exception& ex) override;

    /**
     * @brief Logs an exception at FATAL level with context if enabled and admitted.
     *
     * If FATAL is enabled and admitted, replaces `{suppressed}` in @p msg with the suppressed count,
     * resets the count, and delegates to the underlying logger's exception overload to preserve
     * stacktrace capture behavior. If suppressed, no logging occurs.
     *
     * @param msg Context message describing the operation that failed, optionally containing `{suppressed}`.
     * @param ex The exception to log.
     */
    void fatal(const char* msg, const std::exception& ex) override;

    // -----------------------------------------------------------------------
    // Marker overloads
    // -----------------------------------------------------------------------

    /** @brief Logs a formatted fatal message tagged with @p marker if enabled and admitted. */
    void fatal(const Marker& marker, const char* fmt, ...) override;

    /** @brief Logs a formatted error message tagged with @p marker if enabled and admitted. */
    void error(const Marker& marker, const char* fmt, ...) override;

    /** @brief Logs a formatted warn message tagged with @p marker if enabled and admitted. */
    void warn(const Marker& marker, const char* fmt, ...) override;

    /** @brief Logs a formatted info message tagged with @p marker if enabled and admitted. */
    void info(const Marker& marker, const char* fmt, ...) override;

    /** @brief Logs a formatted debug message tagged with @p marker if enabled and admitted. */
    void debug(const Marker& marker, const char* fmt, ...) override;

    /** @brief Logs a formatted trace message tagged with @p marker if enabled and admitted. */
    void trace(const Marker& marker, const char* fmt, ...) override;

    /**
     * @brief Logs an exception at ERROR level with a marker if enabled and admitted.
     *
     * Replaces `{suppressed}` in @p msg with the suppressed count and delegates to the
     * underlying logger's exception overload with the marker.
     *
     * @param marker Marker to attach.
     * @param msg Context message.
     * @param ex Exception to log.
     */
    void error(const Marker& marker, const char* msg, const std::exception& ex) override;

    /**
     * @brief Logs an exception at FATAL level with a marker if enabled and admitted.
     *
     * Replaces `{suppressed}` in @p msg with the suppressed count and delegates to the
     * underlying logger's exception overload with the marker.
     *
     * @param marker Marker to attach.
     * @param msg Context message.
     * @param ex Exception to log.
     */
    void fatal(const Marker& marker, const char* msg, const std::exception& ex) override;

    // -----------------------------------------------------------------------
    // Multi-marker overloads
    // -----------------------------------------------------------------------

    /** @brief Logs a formatted fatal message tagged with multiple markers if enabled and admitted. */
    void fatal(const MarkerList& markers, const char* fmt, ...) override;

    /** @brief Logs a formatted error message tagged with multiple markers if enabled and admitted. */
    void error(const MarkerList& markers, const char* fmt, ...) override;

    /** @brief Logs a formatted warn message tagged with multiple markers if enabled and admitted. */
    void warn(const MarkerList& markers, const char* fmt, ...) override;

    /** @brief Logs a formatted info message tagged with multiple markers if enabled and admitted. */
    void info(const MarkerList& markers, const char* fmt, ...) override;

    /** @brief Logs a formatted debug message tagged with multiple markers if enabled and admitted. */
    void debug(const MarkerList& markers, const char* fmt, ...) override;

    /** @brief Logs a formatted trace message tagged with multiple markers if enabled and admitted. */
    void trace(const MarkerList& markers, const char* fmt, ...) override;

    /**
     * @brief Logs an exception at ERROR level with multiple markers if enabled and admitted.
     *
     * Replaces `{suppressed}` in @p msg with the suppressed count and delegates to the
     * underlying logger's exception overload with markers.
     *
     * @param markers MarkerList to attach.
     * @param msg Context message.
     * @param ex Exception to log.
     */
    void error(const MarkerList& markers, const char* msg, const std::exception& ex) override;

    /**
     * @brief Logs an exception at FATAL level with multiple markers if enabled and admitted.
     *
     * Replaces `{suppressed}` in @p msg with the suppressed count and delegates to the
     * underlying logger's exception overload with markers.
     *
     * @param markers MarkerList to attach.
     * @param msg Context message.
     * @param ex Exception to log.
     */
    void fatal(const MarkerList& markers, const char* msg, const std::exception& ex) override;

    // -----------------------------------------------------------------------
    // Lazy evaluation overloads (Callables)
    // -----------------------------------------------------------------------

    /**
     * @brief Logs a lazily-evaluated fatal message if FATAL is enabled and admitted.
     *
     * The callable @p fn is only executed if FATAL level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void fatal(Callable&& fn)
    {
        logLazy(Level::Fatal, std::forward<Callable>(fn));
    }

    /**
     * @brief Logs a lazily-evaluated error message if ERROR is enabled and admitted.
     *
     * The callable @p fn is only executed if ERROR level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void error(Callable&& fn)
    {
        logLazy(Level::Error, std::forward<Callable>(fn));
    }

    /**
     * @brief Logs a lazily-evaluated warning message if WARN is enabled and admitted.
     *
     * The callable @p fn is only executed if WARN level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void warn(Callable&& fn)
    {
        logLazy(Level::Warn, std::forward<Callable>(fn));
    }

    /**
     * @brief Logs a lazily-evaluated info message if INFO is enabled and admitted.
     *
     * The callable @p fn is only executed if INFO level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void info(Callable&& fn)
    {
        logLazy(Level::Info, std::forward<Callable>(fn));
    }

    /**
     * @brief Logs a lazily-evaluated debug message if DEBUG is enabled and admitted.
     *
     * The callable @p fn is only executed if DEBUG level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void debug(Callable&& fn)
    {
        logLazy(Level::Debug, std::forward<Callable>(fn));
    }

    /**
     * @brief Logs a lazily-evaluated trace message if TRACE is enabled and admitted.
     *
     * The callable @p fn is only executed if TRACE level is enabled AND this call is
     * admitted by the rate-limiting policy. When suppressed, @p fn is never invoked.
     *
     * @tparam Callable Function object returning std::string (or convertible) with signature
     *         `() -> std::string` or `(int suppressed) -> std::string`.
     * @param fn Lazy message generator callable.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void trace(Callable&& fn)
    {
        logLazy(Level::Trace, std::forward<Callable>(fn));
    }

    // Lazy evaluation with Marker
    /** @brief Logs a lazily-evaluated fatal message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void fatal(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Fatal, marker, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated error message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void error(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Error, marker, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated warn message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void warn(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Warn, marker, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated info message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void info(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Info, marker, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated debug message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void debug(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Debug, marker, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated trace message tagged with @p marker if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void trace(const Marker& marker, Callable&& fn)
    {
        logLazyMarker(Level::Trace, marker, std::forward<Callable>(fn));
    }

    // Lazy evaluation with MarkerList
    /** @brief Logs a lazily-evaluated fatal message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void fatal(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Fatal, markers, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated error message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void error(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Error, markers, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated warn message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void warn(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Warn, markers, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated info message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void info(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Info, markers, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated debug message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void debug(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Debug, markers, std::forward<Callable>(fn));
    }

    /** @brief Logs a lazily-evaluated trace message tagged with multiple markers if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void trace(const MarkerList& markers, Callable&& fn)
    {
        logLazyMarkerList(Level::Trace, markers, std::forward<Callable>(fn));
    }

    // Lazy evaluation with Exceptions
    /**
     * @brief Logs an exception at ERROR level with a lazily-generated context message if enabled and admitted.
     *
     * When admitted, @p fn is invoked to produce the context message, and the call is delegated
     * to the wrapped logger's exception overload to capture stacktraces. When suppressed, @p fn is skipped.
     *
     * @tparam Callable Function object returning std::string (or convertible).
     * @param fn Lazy context message generator.
     * @param ex Exception to log.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void error(Callable&& fn, const std::exception& ex)
    {
        logLazyException(Level::Error, std::forward<Callable>(fn), ex);
    }

    /**
     * @brief Logs an exception at FATAL level with a lazily-generated context message if enabled and admitted.
     *
     * When admitted, @p fn is invoked to produce the context message, and the call is delegated
     * to the wrapped logger's exception overload to capture stacktraces. When suppressed, @p fn is skipped.
     *
     * @tparam Callable Function object returning std::string (or convertible).
     * @param fn Lazy context message generator.
     * @param ex Exception to log.
     */
    template <typename Callable, typename = std::enable_if_t<
        (std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>) &&
        !std::is_convertible_v<Callable, const char*>>>
    void fatal(Callable&& fn, const std::exception& ex)
    {
        logLazyException(Level::Fatal, std::forward<Callable>(fn), ex);
    }

    /** @brief Logs an exception with a marker and lazy context message at ERROR level if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void error(const Marker& marker, Callable&& fn, const std::exception& ex)
    {
        logLazyMarkerException(Level::Error, marker, std::forward<Callable>(fn), ex);
    }

    /** @brief Logs an exception with a marker and lazy context message at FATAL level if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void fatal(const Marker& marker, Callable&& fn, const std::exception& ex)
    {
        logLazyMarkerException(Level::Fatal, marker, std::forward<Callable>(fn), ex);
    }

    /** @brief Logs an exception with multiple markers and lazy context message at ERROR level if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void error(const MarkerList& markers, Callable&& fn, const std::exception& ex)
    {
        logLazyMarkerListException(Level::Error, markers, std::forward<Callable>(fn), ex);
    }

    /** @brief Logs an exception with multiple markers and lazy context message at FATAL level if enabled and admitted. */
    template <typename Callable, typename = std::enable_if_t<
        std::is_invocable_v<Callable> || std::is_invocable_v<Callable, int>>>
    void fatal(const MarkerList& markers, Callable&& fn, const std::exception& ex)
    {
        logLazyMarkerListException(Level::Fatal, markers, std::forward<Callable>(fn), ex);
    }

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    explicit RateLimitedLogger(std::unique_ptr<Impl> impl);

    bool isLevelActive(Level level) const;

    void logFormattedCore(Level level, const Marker* marker, const MarkerList* markers,
                          const char* fmt, va_list args);

    void logLazyImpl(Level level, const Marker* marker, const MarkerList* markerList,
                     const std::function<std::string(int)>& generator);

    void logLazyExceptionImpl(Level level, const Marker* marker, const MarkerList* markerList,
                              const std::function<std::string(int)>& generator,
                              const std::exception& ex);

    template <typename Callable>
    void logLazy(Level level, Callable&& fn)
    {
        if (!isLevelActive(level))
            return;

        logLazyImpl(level, nullptr, nullptr, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        });
    }

    template <typename Callable>
    void logLazyMarker(Level level, const Marker& marker, Callable&& fn)
    {
        if (!isLevelActive(level))
            return;

        logLazyImpl(level, &marker, nullptr, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        });
    }

    template <typename Callable>
    void logLazyMarkerList(Level level, const MarkerList& markers, Callable&& fn)
    {
        if (!isLevelActive(level))
            return;

        logLazyImpl(level, nullptr, &markers, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        });
    }

    template <typename Callable>
    void logLazyException(Level level, Callable&& fn, const std::exception& ex)
    {
        if (!isLevelActive(level))
            return;

        logLazyExceptionImpl(level, nullptr, nullptr, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        }, ex);
    }

    template <typename Callable>
    void logLazyMarkerException(Level level, const Marker& marker, Callable&& fn, const std::exception& ex)
    {
        if (!isLevelActive(level))
            return;

        logLazyExceptionImpl(level, &marker, nullptr, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        }, ex);
    }

    template <typename Callable>
    void logLazyMarkerListException(Level level, const MarkerList& markers, Callable&& fn, const std::exception& ex)
    {
        if (!isLevelActive(level))
            return;

        logLazyExceptionImpl(level, nullptr, &markers, [fn = std::forward<Callable>(fn)](int suppressed) mutable -> std::string {
            if constexpr (std::is_invocable_v<Callable, int>) {
                return std::string(fn(suppressed));
            } else {
                return std::string(fn());
            }
        }, ex);
    }
};

}} // namespace sk::logger

#endif // SK_LOGGER_RATE_LIMITED_LOGGER_H
