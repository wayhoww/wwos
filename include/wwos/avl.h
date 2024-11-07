#ifndef _WWOS_AVL_H
#define _WWOS_AVL_H


#include "wwos/algorithm.h"
#include "wwos/assert.h"


namespace wwos {

template <typename T>
struct avl_node {
    T data;
    avl_node* left = nullptr;
    avl_node* right = nullptr;
    avl_node* parent = nullptr;
    int height = 0;     // height of leaves is 0
};


template<typename T>
void connect_lc(avl_node<T>* parent, avl_node<T>* left) {
    wwassert(parent != nullptr, "parent is null");
    parent->left = left;
    if(left != nullptr) {
        left->parent = parent;
    }
}

template <typename T>
void connect_rc(avl_node<T>* parent, avl_node<T>* right) {
    wwassert(parent != nullptr, "parent is null");
    parent->right = right;
    if(right != nullptr) {
        right->parent = parent;
    }
}

template <typename T>
class avl_tree {
public:
    avl_tree() {}
    ~avl_tree() {}

    void insert(const T& data) {
        if(root == nullptr) {
            root = new avl_node<T>{data};
        } else {
            avl_node<T> *parent = find(data);
            if(data < parent->data) {
                connect_lc(parent, new avl_node<T>{data});
            } else {
                connect_rc(parent, new avl_node<T>{data});
            }
            update_height_and_rebalance(parent);
        }
    }

    void remove(avl_node<T>* node) {
        if(node == nullptr) {
            return;
        }

        wwassert(root != nullptr, "empty tree");

        avl_node<T>* parent = node->parent;

        if(node->left == nullptr && node->right == nullptr) {
            if(parent == nullptr) {
                root = nullptr;
            } else {
                if(parent->left == node) {
                    parent->left = nullptr;
                } else {
                    parent->right = nullptr;
                }
                update_height_and_rebalance(parent);
            }
            delete node;
        } else if(node->left == nullptr) {
            // take the right child and replace the node
            if(parent == nullptr) {
                root = node->right;
                node->right->parent = nullptr;
            } else {
                if(parent->left == node) {
                    connect_lc(parent, node->right);
                } else {
                    connect_rc(parent, node->right);
                }
                update_height_and_rebalance(parent);
            }
            delete node;
        } else if(node->right == nullptr) {
            // take the left child and replace the node
            if(parent == nullptr) {
                root = node->left;
                node->left->parent = nullptr;
            } else {
                if(parent->left == node) {
                    connect_lc(parent, node->left);
                } else {
                    connect_rc(parent, node->left);
                }
                update_height_and_rebalance(parent);
            }
            delete node;
        } else {
            // find the largest node in the left subtree
            avl_node<T>* largest_parent = node;
            avl_node<T>* largest = node->left;
            while(largest->right != nullptr) {
                largest_parent = largest;
                largest = largest->right;
            }
            node->data = largest->data;
            if(largest_parent->left == largest) {
                connect_lc(largest_parent, largest->left);
            } else {
                connect_rc(largest_parent, largest->left);
            }
            update_height_and_rebalance(largest_parent);
            delete largest;
        }
    }

    avl_node<T>* smallest() {
        avl_node<T>* current = root;
        if(current == nullptr) {
            wwassert(false, "empty tree");
        }
        while(current->left != nullptr) {
            current = current->left;
        }
        return current;
    }

    bool empty() const {
        return root == nullptr;
    }

    int height() const {
        return root == nullptr ? -1 : root->height;
    }

    // return the last node that are less than or equal to data. return it's parent if no such node exists
    avl_node<T>* find(const T& data) const {
        avl_node<T>* current = root;
        while(current != nullptr) {
            if(data < current->data) {
                if(current->left == nullptr) {
                    return current;
                } else {
                    current = current->left;
                }
            } else {
                if(current->right == nullptr) {
                    return current;
                } else {
                    current = current->right;
                }
            }
        }
        wwassert(false, "should not reach here");
    }

protected:
    avl_node<T>* root = nullptr;

    void update_height_and_rebalance(avl_node<T>* node) {
        while(true) {
            int left_height = node->left == nullptr ? -1 : node->left->height;
            int right_height = node->right == nullptr ? -1 : node->right->height;

            if(abs(left_height - right_height) > 1) {
                if(left_height > right_height && (node->left->right == nullptr || (node->left->left != nullptr && node->left->left->height > node->left->right->height))) {
                    node = rotate_clockwise(node);
                } else if (left_height < right_height && (node->right->left == nullptr || (node->right->right != nullptr && node->right->left->height <= node->right->right->height))) {
                    node = rotate_counterclockwise(node);
                } else if (left_height > right_height && (node->left->left == nullptr || (node->left->right != nullptr && node->left->left->height <= node->left->right->height))) {
                    node->left = rotate_counterclockwise(node->left);
                    node = rotate_clockwise(node);
                } else if (left_height < right_height && (node->right->right == nullptr || (node->right->left != nullptr && node->right->left->height > node->right->right->height))) {
                    node->right = rotate_clockwise(node->right);
                    node = rotate_counterclockwise(node);
                } else {
                    wwassert(false, "should not reach here");
                }
            } else {
                update_height(node);
            }
            if(node->parent == nullptr) {
                break;
            }
            node = node->parent;
        }
        root = node;
        root->parent = nullptr;
    }

    void update_height(avl_node<T>* node) {
        int left_height = node->left == nullptr ? -1 : node->left->height;
        int right_height = node->right == nullptr ? -1 : node->right->height;
        node->height = left_height > right_height ? left_height + 1 : right_height + 1;
    }

    avl_node<T>* rotate_clockwise(avl_node<T>* node) {
        auto p1 = node->left->left;
        auto p2 = node->left;
        auto p3 = node->left->right;
        auto p4 = node;
        auto p5 = node->right;

        auto parent = node->parent;

        connect_lc(p4, p3);
        connect_rc(p4, p5);

        connect_lc(p2, p1);
        connect_rc(p2, p4);

        update_height(p4);
        update_height(p2);
        
        p2->parent = parent;

        if(parent != nullptr) {
            if(parent->left == node) {
                connect_lc(parent, p2);
            } else {
                connect_rc(parent, p2);
            }
        }

        return p2;
    }

    avl_node<T>* rotate_counterclockwise(avl_node<T>* node) {
        auto p1 = node->left;
        auto p2 = node;
        auto p3 = node->right->left;
        auto p4 = node->right;
        auto p5 = node->right->right;

        auto parent = node->parent;

        connect_lc(p2, p1);
        connect_rc(p2, p3);

        connect_lc(p4, p2);
        connect_rc(p4, p5);

        update_height(p2);
        update_height(p4);

        p4->parent = parent;

        if(parent != nullptr) {
            if(parent->left == node) {
                connect_lc(parent, p4);
            } else {
                connect_rc(parent, p4);
            }
        }

        return p4;
    }
};

};


#endif