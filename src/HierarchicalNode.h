/**
 * @file HierarchicalNode.h
 * @brief Generic template for hierarchical node relationships with contained data.
 *
 * Copyright (c) 2025 Stephen Kouretas. All Rights Reserved.
 *
 * @author Stephen Kouretas <stephen.kouretas@gmail.com>
 * @date Created: November 16, 2025
 * @date Last modified: November 16, 2025
 */
#ifndef SK_HIERARCHICAL_NODE_H
#define SK_HIERARCHICAL_NODE_H

#include <algorithm>
#include <memory>
#include <vector>

namespace sk { namespace common {

/**
 * @class HierarchicalNode
 * @brief Generic container template for hierarchical data with parent-child relationships.
 *
 * A node that contains data of type T and maintains parent-child relationships.
 * Uses composition instead of inheritance - you don't inherit from this class,
 * you create instances of HierarchicalNode<YourDataType>.
 *
 * @tparam T The type of data stored in the node
 *
 * Example usage:
 * @code
 * struct FileInfo {
 *     std::string name;
 *     std::string type;  // "file" or "directory"
 *     size_t size;       // in bytes
 * };
 *
 * using FileNode = HierarchicalNode<FileInfo>;
 * 
 * auto root = std::make_shared<FileNode>();
 * root->data.name = "home";
 * root->data.type = "directory";
 * root->data.size = 0;
 * 
 * auto documents = std::make_shared<FileNode>();
 * documents->data.name = "documents";
 * documents->data.type = "directory";
 * documents->data.size = 0;
 * 
 * auto file1 = std::make_shared<FileNode>();
 * file1->data.name = "readme.txt";
 * file1->data.type = "file";
 * file1->data.size = 1024;
 * 
 * root->addChild(documents);
 * documents->addChild(file1);
 * 
 * // Query relationships
 * std::cout << documents->data.name << " has " 
 *           << documents->getChildCount() << " file(s)" << std::endl;
 * std::cout << file1->data.name << "'s parent directory is " 
 *           << file1->getParent()->data.name << std::endl;
 * @endcode
 */
template <typename T>
class HierarchicalNode : public std::enable_shared_from_this<HierarchicalNode<T>> {
public:
    using NodePtr = std::shared_ptr<HierarchicalNode<T>>;
    using NodeWeakPtr = std::weak_ptr<HierarchicalNode<T>>;

    /**
     * @brief The data stored in this node.
     */
    T data;

    HierarchicalNode() = default;
    explicit HierarchicalNode(const T& data) : data(data) {}
    explicit HierarchicalNode(T&& data) : data(std::move(data)) {}
    virtual ~HierarchicalNode() = default;

    /**
     * @brief Get the parent node.
     * @return Shared pointer to parent node, or nullptr if this is root
     */
    NodePtr getParent() const {
        return parent_.lock();
    }

    /**
     * @brief Set the parent node.
     * @param parent Shared pointer to the parent node
     */
    void setParent(NodePtr parent) {
        parent_ = parent;
    }

    /**
     * @brief Get all child nodes.
     * @return Vector of shared pointers to child nodes
     */
    const std::vector<NodePtr>& getChildren() const {
        return children_;
    }

    /**
     * @brief Add a child node and set this node as its parent.
     * @param child Shared pointer to the child node
     * 
     * Note: This method uses shared_from_this(), so this object must be
     * managed by at least one shared_ptr before calling addChild().
     */
    void addChild(NodePtr child) {
        if (child) {
            children_.push_back(child);
            child->setParent(this->shared_from_this());
        }
    }

    /**
     * @brief Remove a child node.
     * @param child Shared pointer to the child node to remove
     * @return True if child was found and removed, false otherwise
     */
    bool removeChild(NodePtr child) {
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            (*it)->setParent(nullptr);
            children_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Remove all children.
     */
    void clearChildren() {
        for (auto& child : children_) {
            if (child) {
                child->setParent(nullptr);
            }
        }
        children_.clear();
    }

    /**
     * @brief Check if this node has any children.
     * @return True if there are children, false otherwise
     */
    bool hasChildren() const {
        return !children_.empty();
    }

    /**
     * @brief Get the number of direct children.
     * @return Number of children
     */
    size_t getChildCount() const {
        return children_.size();
    }

    /**
     * @brief Check if this node is a root node (has no parent).
     * @return True if this is a root node, false otherwise
     */
    bool isRoot() const {
        return parent_.expired();
    }

    /**
     * @brief Check if this node is a leaf node (has no children).
     * @return True if this is a leaf node, false otherwise
     */
    bool isLeaf() const {
        return children_.empty();
    }

private:
    // Use weak_ptr for parent to avoid circular references
    NodeWeakPtr parent_;
    
    // Children are owned by the parent
    std::vector<NodePtr> children_;
};

} } // namespace sk::common

#endif  // SK_HIERARCHICAL_NODE_H
