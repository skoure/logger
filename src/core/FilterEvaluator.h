/**
 * @file FilterEvaluator.h
 * @brief Internal short-circuit evaluation for sink filters.
 * 
 * Copyright (c) 2026 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: October 18, 2026
 */
#ifndef SK_FILTER_EVALUATOR_H
#define SK_FILTER_EVALUATOR_H

#include <LogRecord.h>
#include <logger/Filter.h>
#include <memory>
#include <vector>

namespace sk { namespace logger {

class FilterEvaluator {
public:
    static bool passesAll(const std::vector<std::shared_ptr<const Filter>>& filters,
                          const LogRecord& record)
    {
        for (const auto& filter : filters)
        {
            if (!filter || !filter->accepts(record))
                return false;
        }
        return true;
    }
};

}} // namespace sk::logger

#endif // SK_FILTER_EVALUATOR_H