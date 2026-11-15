 /**
 * @file MarkerPatternConverter.h
 * @brief log4cxx converter for configured marker pattern tokens.
 *
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 14, 2026
 */
#ifndef SK_LOG4CXX_MARKER_PATTERN_CONVERTER_H
#define SK_LOG4CXX_MARKER_PATTERN_CONVERTER_H

#include <MarkerPattern.h>
#include <log4cxx/pattern/loggingeventpatternconverter.h>

namespace sk { namespace logger {

/** @brief Formats marker names selected by one configured `%M{...}` token. */
class MarkerPatternConverter final
    : public log4cxx::pattern::LoggingEventPatternConverter
{
public:
    /** @brief Constructs a converter for a parsed marker selection rule. */
    explicit MarkerPatternConverter(MarkerPatternOptions options);

    /** @brief Returns the log4cxx converter class identity. */
    const log4cxx::helpers::Class& getClass() const override;

    /** @brief Supports log4cxx object casts through the converter base class. */
    const void* cast(const log4cxx::helpers::Class& clazz) const override;

    /** @brief Reports whether this converter supports the requested class. */
    bool instanceof(const log4cxx::helpers::Class& clazz) const override;

    /** @brief Formats the event's selected markers into the layout output. */
    void format(const log4cxx::spi::LoggingEventPtr& event,
                log4cxx::LogString& output,
                log4cxx::helpers::Pool& pool) const override;

private:
    MarkerPatternOptions options_;
};

}} // namespace sk::logger

#endif // SK_LOG4CXX_MARKER_PATTERN_CONVERTER_H