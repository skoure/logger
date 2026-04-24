/**
 * @file LoggerHierarchy.cpp
 * @brief Implementation of hierarchical logger management.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 */
#include "LoggerHierarchy.h"

#include <stdexcept>

using namespace sk::logger;

LoggerHierarchy::LoggerHierarchy()
{
    root_ = std::make_shared<LoggerNode>();
    loggerMap_["root"] = root_;
}

std::string LoggerHierarchy::getParentPath(const std::string& name)
{
    if (name == "root")
        return "root";

    size_t lastDot = name.find_last_of('.');
    if (lastDot == std::string::npos)
        return "root";

    return name.substr(0, lastDot);
}

void LoggerHierarchy::createParents(const std::string& path)
{
    if (path == "root" || loggerMap_.count(path) > 0) {
        return;  // Already exists
    }

    std::string parentPath = getParentPath(path);
    if (parentPath != path) {
        createParents(parentPath); // Recursively create parents
    }
    
    // Create intermediate node if it doesn't have a logger yet
    if (loggerMap_.count(path) == 0) {
        auto node = std::make_shared<LoggerNode>();
        // data is nullptr — intermediate placeholder, no logger assigned yet
        loggerMap_[parentPath]->addChild(node);
        loggerMap_[path] = node;
    }
}

void LoggerHierarchy::addLogger(const std::string& name, LoggerPtr logger)
{
    if (name.empty()) {
        throw std::runtime_error("Logger name cannot be empty");
    }
    
    if (!logger) {
        throw std::runtime_error("Logger instance cannot be nullptr");
    }        
    if (name == "root") {
        root_->data = logger;
        return;
    }

    std::string parentPath = getParentPath(name);
    createParents(parentPath);

    if (loggerMap_.count(name) > 0) {
        loggerMap_[name]->data = logger;
    } else {
        auto node = std::make_shared<LoggerNode>();
        node->data = logger;
        loggerMap_[parentPath]->addChild(node);
        loggerMap_[name] = node;
    }
}

LoggerPtr LoggerHierarchy::getLogger(const std::string& name) const
{
    auto it = loggerMap_.find(name);
    return (it != loggerMap_.end()) ? it->second->data : nullptr;
}

LoggerHierarchy::LoggerNodePtr LoggerHierarchy::getLoggerNode(const std::string& name) const
{
    auto it = loggerMap_.find(name);
    return (it != loggerMap_.end()) ? it->second : nullptr;
}

LoggerPtr LoggerHierarchy::getParent(const std::string& name) const
{
    if (name == "root") {
        return nullptr; // root has no parent
    }
    std::string parentPath = getParentPath(name);
    auto it = loggerMap_.find(parentPath);
    return (it != loggerMap_.end()) ? it->second->data : nullptr;
}

std::vector<LoggerPtr> LoggerHierarchy::getChildren(const std::string& name) const
{
    std::vector<LoggerPtr> children;
    auto it = loggerMap_.find(name);
    if (it == loggerMap_.end()) {
        return children;
    }

    for (const auto& childNode : it->second->getChildren()) {
        if (childNode->data)
            children.push_back(childNode->data);
    }
    return children;
}

LoggerPtr LoggerHierarchy::getRoot() const
{
    return root_->data;
}

void LoggerHierarchy::clear()
{
    loggerMap_.clear();
    root_ = std::make_shared<LoggerNode>();
    loggerMap_["root"] = root_;
}

size_t LoggerHierarchy::getLoggerCount() const
{
    return loggerMap_.size();
}

std::vector<std::pair<std::string, LoggerPtr>> LoggerHierarchy::getAllLoggersTopDown() const
{
    std::vector<std::pair<std::string, LoggerPtr>> result;
    for (const auto& entry : loggerMap_) {
        if (entry.second->data)
            result.emplace_back(entry.first, entry.second->data);
    }
    return result;
}

