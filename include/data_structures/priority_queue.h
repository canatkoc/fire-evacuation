#pragma once

#include <cstddef>
#include <stdexcept>

template <typename T, typename Compare>
class PriorityQueue {
 private:
  struct HeapNode {
    T value;
    HeapNode* parent;
    HeapNode* left;
    HeapNode* right;
    HeapNode* next_insert;

    explicit HeapNode(const T& v)
        : value(v), parent(nullptr), left(nullptr), right(nullptr), next_insert(nullptr) {}
  };

  HeapNode* root_;
  HeapNode* insertion_queue_head_;
  HeapNode* insertion_queue_tail_;
  std::size_t size_;
  Compare compare_;

  void enqueueInsertionNode(HeapNode* node) {
    node->next_insert = nullptr;
    if (insertion_queue_tail_ == nullptr) {
      insertion_queue_head_ = node;
      insertion_queue_tail_ = node;
      return;
    }
    insertion_queue_tail_->next_insert = node;
    insertion_queue_tail_ = node;
  }

  HeapNode* dequeueInsertionNode() {
    if (insertion_queue_head_ == nullptr) {
      return nullptr;
    }
    HeapNode* result = insertion_queue_head_;
    insertion_queue_head_ = insertion_queue_head_->next_insert;
    if (insertion_queue_head_ == nullptr) {
      insertion_queue_tail_ = nullptr;
    }
    result->next_insert = nullptr;
    return result;
  }

  void bubbleUp(HeapNode* node) {
    while (node->parent != nullptr && compare_(node->value, node->parent->value)) {
      T temp = node->value;
      node->value = node->parent->value;
      node->parent->value = temp;
      node = node->parent;
    }
  }

  void bubbleDown(HeapNode* node) {
    while (node != nullptr) {
      HeapNode* best = node;
      if (node->left != nullptr && compare_(node->left->value, best->value)) {
        best = node->left;
      }
      if (node->right != nullptr && compare_(node->right->value, best->value)) {
        best = node->right;
      }
      if (best == node) {
        return;
      }
      T temp = node->value;
      node->value = best->value;
      best->value = temp;
      node = best;
    }
  }

  void clearSubtree(HeapNode* node) {
    if (node == nullptr) {
      return;
    }
    clearSubtree(node->left);
    clearSubtree(node->right);
    delete node;
  }

  HeapNode* findLastNodeLevelOrder(HeapNode* node) const {
    if (node == nullptr) {
      return nullptr;
    }

    HeapNode* bfs_head = node;
    HeapNode* bfs_tail = node;
    node->next_insert = nullptr;
    HeapNode* last = nullptr;

    while (bfs_head != nullptr) {
      HeapNode* current = bfs_head;
      bfs_head = bfs_head->next_insert;
      if (bfs_head == nullptr) {
        bfs_tail = nullptr;
      }
      current->next_insert = nullptr;
      last = current;

      if (current->left != nullptr) {
        current->left->next_insert = nullptr;
        if (bfs_tail == nullptr) {
          bfs_head = current->left;
          bfs_tail = current->left;
        } else {
          bfs_tail->next_insert = current->left;
          bfs_tail = current->left;
        }
      }
      if (current->right != nullptr) {
        current->right->next_insert = nullptr;
        if (bfs_tail == nullptr) {
          bfs_head = current->right;
          bfs_tail = current->right;
        } else {
          bfs_tail->next_insert = current->right;
          bfs_tail = current->right;
        }
      }
    }
    return last;
  }

 public:
  explicit PriorityQueue(const Compare& compare = Compare())
      : root_(nullptr),
        insertion_queue_head_(nullptr),
        insertion_queue_tail_(nullptr),
        size_(0),
        compare_(compare) {}

  ~PriorityQueue() { clear(); }

  PriorityQueue(const PriorityQueue&) = delete;
  PriorityQueue& operator=(const PriorityQueue&) = delete;

  PriorityQueue(PriorityQueue&& other) noexcept
      : root_(other.root_),
        insertion_queue_head_(other.insertion_queue_head_),
        insertion_queue_tail_(other.insertion_queue_tail_),
        size_(other.size_),
        compare_(other.compare_) {
    other.root_ = nullptr;
    other.insertion_queue_head_ = nullptr;
    other.insertion_queue_tail_ = nullptr;
    other.size_ = 0;
  }

  PriorityQueue& operator=(PriorityQueue&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    clear();
    root_ = other.root_;
    insertion_queue_head_ = other.insertion_queue_head_;
    insertion_queue_tail_ = other.insertion_queue_tail_;
    size_ = other.size_;
    compare_ = other.compare_;

    other.root_ = nullptr;
    other.insertion_queue_head_ = nullptr;
    other.insertion_queue_tail_ = nullptr;
    other.size_ = 0;
    return *this;
  }

  bool empty() const { return size_ == 0; }
  std::size_t size() const { return size_; }

  const T& top() const {
    if (root_ == nullptr) {
      throw std::out_of_range("PriorityQueue is empty.");
    }
    return root_->value;
  }

  void push(const T& value) {
    HeapNode* node = new HeapNode(value);
    ++size_;

    if (root_ == nullptr) {
      root_ = node;
      enqueueInsertionNode(node);
      return;
    }

    HeapNode* parent = insertion_queue_head_;
    node->parent = parent;
    if (parent->left == nullptr) {
      parent->left = node;
    } else {
      parent->right = node;
      dequeueInsertionNode();
    }

    enqueueInsertionNode(node);
    bubbleUp(node);
  }

  void pop() {
    if (root_ == nullptr) {
      throw std::out_of_range("PriorityQueue is empty.");
    }

    if (size_ == 1) {
      delete root_;
      root_ = nullptr;
      insertion_queue_head_ = nullptr;
      insertion_queue_tail_ = nullptr;
      size_ = 0;
      return;
    }

    HeapNode* last = findLastNodeLevelOrder(root_);
    root_->value = last->value;

    HeapNode* parent = last->parent;
    if (parent != nullptr) {
      if (parent->right == last) {
        parent->right = nullptr;
      } else {
        parent->left = nullptr;
      }
    }
    delete last;
    --size_;

    insertion_queue_head_ = nullptr;
    insertion_queue_tail_ = nullptr;
    if (root_ != nullptr) {
      HeapNode* bfs_head = nullptr;
      HeapNode* bfs_tail = nullptr;
      enqueueInsertionNode(root_);
      bfs_head = insertion_queue_head_;
      bfs_tail = insertion_queue_tail_;
      insertion_queue_head_ = nullptr;
      insertion_queue_tail_ = nullptr;

      while (bfs_head != nullptr) {
        HeapNode* current = bfs_head;
        bfs_head = bfs_head->next_insert;
        if (bfs_head == nullptr) {
          bfs_tail = nullptr;
        }

        current->next_insert = nullptr;
        if (current->left == nullptr || current->right == nullptr) {
          enqueueInsertionNode(current);
        }
        if (current->left != nullptr) {
          current->left->next_insert = nullptr;
          if (bfs_tail == nullptr) {
            bfs_head = current->left;
            bfs_tail = current->left;
          } else {
            bfs_tail->next_insert = current->left;
            bfs_tail = current->left;
          }
        }
        if (current->right != nullptr) {
          current->right->next_insert = nullptr;
          if (bfs_tail == nullptr) {
            bfs_head = current->right;
            bfs_tail = current->right;
          } else {
            bfs_tail->next_insert = current->right;
            bfs_tail = current->right;
          }
        }
      }
    }

    bubbleDown(root_);
  }

  void clear() {
    clearSubtree(root_);
    root_ = nullptr;
    insertion_queue_head_ = nullptr;
    insertion_queue_tail_ = nullptr;
    size_ = 0;
  }
};
