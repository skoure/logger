/**
 * @file LoggerHierarchy.cpp
 * @brief Implementation of hierarchical logger management.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 * @date Last modified: November 16, 2025
 */
#include "LoggerHierarchy.h"

#include <algorithm>
#include <functional>
#include <sstream>

#include <SimpleLogger.h>

using namespace sk::logger;

LoggerHierarchy::LoggerHierarchy() {
    // Create root node
    root_ = std::make_shared<LoggerNode>();
    root_->data.name = "root";
    root_->data.additivity = true;
    loggerMap_["root"] = root_;
}

std::vector<std::string> LoggerHierarchy::splitLoggerName(const std::string& name) {
    std::vector<std::string> parts;
    std::stringstream ss(name);
    std::string part;

    while (std::getline(ss, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    return parts;
}

std::string LoggerHierarchy::getParentPath(const std::string& name) {
    if (name == "root") {
        return "root";  // root has no parent
    }

    size_t lastDot = name.find_last_of('.');
    if (lastDot == std::string::npos) {
        return "root";  // Direct child of root
    }

    return name.substr(0, lastDot);
}

void LoggerHierarchy::createParents(const std::string& path) {
    if (path == "root" || loggerMap_.count(path) > 0) {
        return;  // Already exists
    }

    std::string parentPath = getParentPath(path);
    if (parentPath != path) {
        createParents(parentPath);  // Recursively create parents
    }

    // Create intermediate node if it doesn't have a logger yet
    if (loggerMap_.count(path) == 0) {
        auto node = std::make_shared<LoggerNode>();
        node->data.name = path;
        loggerMap_[parentPath]->addChild(node);
        loggerMap_[path] = node;
    }
}

LoggerHierarchy::LoggerNodePtr LoggerHierarchy::addLogger(const std::string& name, LoggerPtr logger, bool additivity) {
    if (name.empty()) {
        throw std::runtime_error("Logger name cannot be empty");
    }

    if (!logger) {
        throw std::runtime_error("Logger instance cannot be nullptr");
    }

    // Handle root logger specially
    if (name == "root") {
        root_->data.logger = logger;
        root_->data.additivity = additivity;
        return root_;
    }

    // Ensure all parent nodes exist
    std::string parentPath = getParentPath(name);
    createParents(parentPath);

    // Create or update the logger node
    if (loggerMap_.count(name) > 0) {
        // Update existing node
        auto node = loggerMap_[name];
        node->data.logger = logger;
        node->data.additivity = additivity;
        return node;
    } else {
        // Create new node
        auto node = std::make_shared<LoggerNode>();
        node->data.name = name;
        node->data.logger = logger;
        node->data.additivity = additivity;
        loggerMap_[parentPath]->addChild(node);
        loggerMap_[name] = node;
        return node;
    }
}

LoggerPtr LoggerHierarchy::getLogger(const std::string& name) const {
    auto node = getLoggerNode(name);
    return node ? node->data.logger : nullptr;
}

LoggerHierarchy::LoggerNodePtr LoggerHierarchy::getLoggerNode(const std::string& name) const {
    auto it = loggerMap_.find(name);
    if (it != loggerMap_.end()) {
        return it->second;
    }
    return nullptr;
}

LoggerPtr LoggerHierarchy::getParent(const std::string& name) const {
    auto parentNode = getParentNode(name);
    return parentNode ? parentNode->data.logger : nullptr;
}

LoggerHierarchy::LoggerNodePtr LoggerHierarchy::getParentNode(const std::string& name) const {
    if (name == "root") {
        return nullptr;  // root has no parent
    }

    std::string parentPath = getParentPath(name);
    return getLoggerNode(parentPath);
}

std::vector<LoggerPtr> LoggerHierarchy::getChildren(const std::string& name) const {
    std::vector<LoggerPtr> children;
    auto node = getLoggerNode(name);

    if (!node) {
        return children;
    }

    for (const auto& childNode : node->getChildren()) {
        if (childNode->data.logger) {
            children.push_back(childNode->data.logger);
        }
    }

    return children;
}

void LoggerHierarchy::setAdditivity(const std::string& name, bool additivity) {
    auto node = getLoggerNode(name);
    if (node) {
        node->data.additivity = additivity;
    }
}

bool LoggerHierarchy::hasAdditivity(const std::string& name) const {
    auto node = getLoggerNode(name);
    return node ? node->data.additivity : true;  // default is true
}


LoggerPtr LoggerHierarchy::getRoot() const { return root_->data.logger; }

void LoggerHierarchy::clear() {
    loggerMap_.clear();
    root_ = std::make_shared<LoggerNode>();
    root_->data.name = "root";
    root_->data.additivity = true;
    loggerMap_["root"] = root_;
}

size_t LoggerHierarchy::getLoggerCount() const { return loggerMap_.size(); }
