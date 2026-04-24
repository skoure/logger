/**
 * @file LoggerHierarchy.h
 * @brief Hierarchical logger management for parent-child logger relationships.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 */
#ifndef SK_LOGGER_HIERARCHY_H
#define SK_LOGGER_HIERARCHY_H

#include <logger/Logger.h>
#include <containers/HierarchicalNode.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace sk { namespace logger {

/**
 * @class LoggerHierarchy
 * @brief Manages hierarchical relationships between loggers.
 *
 * Provides parent-child logger relationships where:
 * - Logger names are dot-separated (e.g., "App.Database.SQL")
 * - "root" is the top-level logger
 * - Level inheritance is handled dynamically by the loggers themselves
 *   (see LoggerBase::getLevel() which walks the parent chain)
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
    typedef sk::common::containers::HierarchicalNode<LoggerPtr> LoggerNode;
    typedef std::shared_ptr<LoggerNode>                         LoggerNodePtr;

    LoggerHierarchy();
    virtual ~LoggerHierarchy() = default;

    /**
     * @brief Add a logger to the hierarchy.
     *
     * Missing intermediate ancestor nodes are created automatically.
     *
     * @param name   Logger name (e.g., "App.Database")
     * @param logger The logger instance
     */
    void addLogger(const std::string& name, LoggerPtr logger);

    /**
     * @brief Get a logger by name from the hierarchy.
     * @param name Logger name (e.g., "App.Database")
     * @return Logger instance or nullptr if not found or is an intermediate node
     */
    LoggerPtr getLogger(const std::string& name) const;

    /**
     * @brief Get the hierarchy node for a given name.
     *
     * Unlike getLogger(), this returns a non-null pointer for intermediate nodes
     * (nodes that exist in the tree structure but have no logger assigned yet).
     *
     * @param name Logger name (e.g., "App.Database")
     * @return Node pointer or nullptr if no such node exists
     */
    LoggerNodePtr getLoggerNode(const std::string& name) const;

    /**
     * @brief Get the parent of a logger.
     * @param name Logger name (e.g., "App.Database")
     * @return Parent logger instance or nullptr if not found or is root
     */
    LoggerPtr getParent(const std::string& name) const;

    /**
     * @brief Get children of a logger.
     * @param name Logger name (e.g., "App.Database")
     * @return Vector of child loggers (excludes intermediate nodes with no logger)
     */
    std::vector<LoggerPtr> getChildren(const std::string& name) const;

    /**
     * @brief Get root logger.
     * @return Root logger instance or nullptr if not yet set
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

    /**
     * @brief Return all (name, logger) pairs that have a logger assigned,
     * in alphabetical order.
     *
     * For dot-separated logger names, alphabetical order is parent-before-child,
     * making this suitable for top-down traversal during sink propagation.
     * Placeholder nodes (data == nullptr) are excluded.
     */
    std::vector<std::pair<std::string, LoggerPtr>> getAllLoggersTopDown() const;

private:
    /**
     * @brief Get the parent path of a logger name.
     * Examples: "App.Database.SQL" -> "App.Database"
     *           "App"              -> "root"
     *           "root"             -> "root"
     */
    static std::string getParentPath(const std::string& name);

    /**
     * @brief Ensure all ancestor nodes exist in the hierarchy.
     * @param path Logger path (e.g., "App.Database.SQL")
     */
    void createParents(const std::string& path);

    // Root node of the hierarchy
    LoggerNodePtr root_;

    // Map of all nodes by logger name for O(1) lookup
    std::map<std::string, LoggerNodePtr> loggerMap_;
};

}}  // namespace sk::logger

#endif  // SK_LOGGER_HIERARCHY_H
