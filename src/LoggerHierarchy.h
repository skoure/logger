/**
 * @file LoggerHierarchy.h
 * @brief Hierarchical logger management for parent-child logger relationships.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 * @date Last modified: November 16, 2025
 */
#ifndef SK_LOGGER_HIERARCHY_H
#define SK_LOGGER_HIERARCHY_H

#include <logger/Logger.h>

#include <map>
#include <string>
#include <vector>

namespace sk { namespace logger {

/**
 * @class LoggerHierarchy
 * @brief Manages hierarchical relationships between loggers.
 *
 * Provides parent-child logger relationships where:
 * - Child loggers inherit configuration from parent loggers
 * - Logger names are dot-separated (e.g., "App.Database.SQL")
 * - "root" is the top-level logger
 * - Additivity flag controls whether messages propagate to parents
 *
 * Example hierarchy:
 *   root
 *   ├── App
 *   │   ├── App.Database
 *   │   │   └── App.Database.SQL
 *   │   └── App.UI
 *   └── System
 */
class LoggerHierarchy {
public:
    /**
     * @struct LoggerNode
     * @brief Represents a node in the logger hierarchy.
     */
    struct LoggerNode {
        std::string name;                                   // Full logger name
        LoggerPtr logger;                                   // The logger instance
        std::shared_ptr<LoggerNode> parent;                 // Parent node (nullptr if root)
        std::vector<std::shared_ptr<LoggerNode>> children;  // Child nodes
        bool additivity = true;                             // Whether to propagate to parent
    };

    typedef std::shared_ptr<LoggerNode> LoggerNodePtr;

    LoggerHierarchy();
    virtual ~LoggerHierarchy() = default;

    /**
     * @brief Add a logger to the hierarchy.
     * @param name Logger name (e.g., "App.Database")
     * @param logger The logger instance
     * @param additivity Whether this logger propagates to parent (default: true)
     * @return Pointer to the created or updated node
     */
    LoggerNodePtr addLogger(const std::string& name, LoggerPtr logger, bool additivity = true);

    /**
     * @brief Get a logger by name from the hierarchy.
     * @param name Logger name (e.g., "App.Database")
     * @return Logger instance or nullptr if not found
     */
    LoggerPtr getLogger(const std::string& name) const;

    /**
     * @brief Get a logger node by name from the hierarchy.
     * @param name Logger name (e.g., "App.Database")
     * @return LoggerNode pointer or nullptr if not found
     */
    LoggerNodePtr getLoggerNode(const std::string& name) const;

    /**
     * @brief Get the parent of a logger.
     * @param name Logger name (e.g., "App.Database")
     * @return Parent logger instance or nullptr if not found or is root
     */
    LoggerPtr getParent(const std::string& name) const;

    /**
     * @brief Get the parent logger node.
     * @param name Logger name (e.g., "App.Database")
     * @return Parent node pointer or nullptr if not found or is root
     */
    LoggerNodePtr getParentNode(const std::string& name) const;

    /**
     * @brief Get children of a logger.
     * @param name Logger name (e.g., "App.Database")
     * @return Vector of child loggers
     */
    std::vector<LoggerPtr> getChildren(const std::string& name) const;

    /**
     * @brief Set the additivity flag for a logger.
     * @param name Logger name
     * @param additivity Whether to propagate to parent
     */
    void setAdditivity(const std::string& name, bool additivity);

    /**
     * @brief Check if a logger has additivity enabled.
     * @param name Logger name
     * @return True if additivity is enabled, false otherwise
     */
    bool hasAdditivity(const std::string& name) const;


    /**
     * @brief Get root logger.
     * @return Root logger instance or nullptr if not set
     */
    LoggerPtr getRoot() const;

    /**
     * @brief Clear the entire hierarchy.
     */
    void clear();

    /**
     * @brief Get number of loggers in hierarchy.
     * @return Total logger count
     */
    size_t getLoggerCount() const;

private:
    /**
     * @brief Split a logger name into parts.
     * Examples: "App.Database.SQL" -> ["App", "Database", "SQL"]
     *           "root" -> ["root"]
     */
    static std::vector<std::string> splitLoggerName(const std::string& name);

    /**
     * @brief Get the parent path of a logger.
     * Examples: "App.Database.SQL" -> "App.Database"
     *           "App" -> "root"
     */
    static std::string getParentPath(const std::string& name);

    /**
     * @brief Create missing parent nodes in the hierarchy.
     * @param path Logger path (e.g., "App.Database.SQL")
     */
    void createParents(const std::string& path);

    // Root node of the hierarchy
    LoggerNodePtr root_;

    // Map of all loggers by name for quick lookup
    std::map<std::string, LoggerNodePtr> loggerMap_;
};

}}  // namespace sk::logger

#endif  // SK_LOGGER_HIERARCHY_H
