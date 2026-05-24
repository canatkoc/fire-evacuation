/**
 * @file priority_queue.h
 * @brief Pointer-based binary min-heap priority queue.
 *
 * Implements a binary heap using explicit parent/left/right node pointers --
 * no arrays or index arithmetic. A secondary singly-linked insertion queue
 * tracks the next available slot so that push() locates the insertion parent
 * in O(1) rather than O(n). Used as the Dijkstra frontier in FireSimulator.
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once

#include <cstddef>
#include <stdexcept>

/**
 * @brief A generic binary min-heap priority queue with pointer-based nodes.
 *
 * Elements are ordered by a caller-supplied comparator (@p Compare).
 * The heap invariant is maintained via bubbleUp() on push and bubbleDown()
 * on pop. push() and pop() both run in O(log n).
 *
 * Copy construction and copy assignment are disabled to prevent accidental
 * deep-copy of the heap tree; move semantics are provided instead.
 *
 * @tparam T       The element type. Must be copy-constructible and swappable.
 * @tparam Compare A strict-weak-ordering comparator (default: std::less<T>).
 *                 compare_(a, b) returns true when @a a should be popped
 *                 before @a b.
 */
template <typename T, typename Compare>
class PriorityQueue {
 private:
  // -------------------------------------------------------------------------
  // Internal node
  // -------------------------------------------------------------------------

  /**
   * @brief A node in the binary-heap tree.
   *
   * Stores the element value and four raw pointers: parent, left child,
   * right child, and next_insert (used by the insertion-order queue).
   */
  struct HeapNode {
    T         value;        ///< The element stored at this node.
    HeapNode* parent;       ///< Pointer to parent; nullptr for the root.
    HeapNode* left;         ///< Pointer to the left child; nullptr if absent.
    HeapNode* right;        ///< Pointer to the right child; nullptr if absent.
    HeapNode* next_insert;  ///< Intrusive link used by the insertion-order queue.

    /**
     * @brief Constructs a HeapNode holding @p v.
     * @param v The value to store.
     * @post All pointers are nullptr.
     */
    explicit HeapNode(const T& v)
        : value(v), parent(nullptr), left(nullptr), right(nullptr),
          next_insert(nullptr) {}
  };

  HeapNode*   root_;                  ///< Root of the heap tree.
  HeapNode*   insertion_queue_head_;  ///< Head of the BFS insertion-order queue.
  HeapNode*   insertion_queue_tail_;  ///< Tail of the BFS insertion-order queue.
  std::size_t size_;                  ///< Number of elements in the heap.
  Compare     compare_;               ///< Comparator determining priority order.

  // -------------------------------------------------------------------------
  // Private helpers
  // -------------------------------------------------------------------------

  /**
   * @brief Appends @p node to the tail of the insertion-order queue.
   * @param node The node to enqueue; must not be nullptr.
   * @post @p node is reachable from insertion_queue_head_ via next_insert links.
   */
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

  /**
   * @brief Removes and returns the head of the insertion-order queue.
   * @return The former head node, or nullptr if the queue is empty.
   * @post insertion_queue_head_ advances to the next node.
   */
  HeapNode* dequeueInsertionNode() {
    if (insertion_queue_head_ == nullptr) return nullptr;
    HeapNode* result = insertion_queue_head_;
    insertion_queue_head_ = insertion_queue_head_->next_insert;
    if (insertion_queue_head_ == nullptr) insertion_queue_tail_ = nullptr;
    result->next_insert = nullptr;
    return result;
  }

  /**
   * @brief Restores the heap property upward from @p node.
   *
   * Swaps @p node's value with its parent's while the comparator prefers the
   * child over the parent (i.e., the child has higher priority).
   *
   * @param node The node from which to begin the upward traversal.
   * @post The heap invariant holds from @p node up to the root.
   */
  void bubbleUp(HeapNode* node) {
    while (node->parent != nullptr &&
           compare_(node->value, node->parent->value)) {
      T temp = node->value;
      node->value = node->parent->value;
      node->parent->value = temp;
      node = node->parent;
    }
  }

  /**
   * @brief Restores the heap property downward from @p node.
   *
   * Repeatedly swaps @p node's value with the higher-priority child until
   * the heap invariant is satisfied at every level.
   *
   * @param node The node from which to begin the downward traversal.
   * @post The heap invariant holds from @p node down to the leaves.
   */
  void bubbleDown(HeapNode* node) {
    while (node != nullptr) {
      HeapNode* best = node;
      if (node->left  != nullptr && compare_(node->left->value,  best->value)) best = node->left;
      if (node->right != nullptr && compare_(node->right->value, best->value)) best = node->right;
      if (best == node) return;
      T temp = node->value;
      node->value = best->value;
      best->value = temp;
      node = best;
    }
  }

  /**
   * @brief Recursively frees the subtree rooted at @p node.
   * @param node Root of the subtree to free (may be nullptr).
   * @post All nodes in the subtree are deleted.
   */
  void clearSubtree(HeapNode* node) {
    if (node == nullptr) return;
    clearSubtree(node->left);
    clearSubtree(node->right);
    delete node;
  }

  /**
   * @brief Returns the last node in BFS (level) order.
   *
   * Used by pop() to find the node that will replace the root before removal.
   * The function uses the @c next_insert field as a temporary BFS link and
   * restores it to nullptr before returning.
   *
   * @param node Root of the heap tree.
   * @return Pointer to the deepest, rightmost node; nullptr if @p node is nullptr.
   * @pre  @p node != nullptr (caller must guard).
   * @post All next_insert pointers in the tree are nullptr.
   */
  HeapNode* findLastNodeLevelOrder(HeapNode* node) const;

 public:
  // -------------------------------------------------------------------------
  // Public interface
  // -------------------------------------------------------------------------

  /**
   * @brief Constructs an empty priority queue with the given comparator.
   * @param compare Comparator instance (defaults to a default-constructed Compare).
   * @post empty() == true.
   */
  explicit PriorityQueue(const Compare& compare = Compare())
      : root_(nullptr),
        insertion_queue_head_(nullptr),
        insertion_queue_tail_(nullptr),
        size_(0),
        compare_(compare) {}

  /**
   * @brief Destructor -- frees all heap nodes.
   * @post All memory owned by this priority queue is released.
   */
  ~PriorityQueue() { clear(); }

  /// Copy construction is disabled (deep-copy of a pointer tree is expensive
  /// and not required by the project).
  PriorityQueue(const PriorityQueue&) = delete;
  /// Copy assignment is disabled for the same reason.
  PriorityQueue& operator=(const PriorityQueue&) = delete;

  /**
   * @brief Move constructor -- takes ownership of @p other's heap tree.
   * @param other The source queue (left empty but valid).
   * @post This queue owns the nodes previously owned by @p other.
   */
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

  /**
   * @brief Move-assignment operator.
   * @param other The source queue (left empty but valid).
   * @return Reference to this queue.
   * @post Previous nodes of this queue are freed; ownership transferred from @p other.
   */
  PriorityQueue& operator=(PriorityQueue&& other) noexcept {
    if (this == &other) return *this;
    clear();
    root_                 = other.root_;
    insertion_queue_head_ = other.insertion_queue_head_;
    insertion_queue_tail_ = other.insertion_queue_tail_;
    size_                 = other.size_;
    compare_              = other.compare_;
    other.root_ = nullptr;
    other.insertion_queue_head_ = nullptr;
    other.insertion_queue_tail_ = nullptr;
    other.size_ = 0;
    return *this;
  }

  /**
   * @brief Reports whether the queue is empty.
   * @return true if size_ == 0, false otherwise.
   */
  bool empty() const { return size_ == 0; }

  /**
   * @brief Returns the number of elements in the queue.
   * @return Current element count (>= 0).
   */
  std::size_t size() const { return size_; }

  /**
   * @brief Returns a const reference to the highest-priority element.
   *
   * @pre  !empty()
   * @return Const reference to the root element (highest priority).
   * @throws std::out_of_range if the queue is empty.
   */
  const T& top() const {
    if (root_ == nullptr) throw std::out_of_range("PriorityQueue is empty.");
    return root_->value;
  }

  /**
   * @brief Inserts @p value into the priority queue.
   *
   * Allocates a new HeapNode, appends it to the next available slot
   * (tracked by the insertion-order queue), then calls bubbleUp() to
   * restore the heap invariant. O(log n).
   *
   * @param value The element to insert.
   * @post size() is incremented by 1; the heap invariant holds.
   */
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

  /**
   * @brief Removes the highest-priority element from the queue.
   *
   * Replaces the root's value with the last node's value (BFS order),
   * deletes the last node, rebuilds the insertion-order queue, then
   * calls bubbleDown() to restore the heap invariant. O(log n).
   *
   * @pre  !empty()
   * @post size() is decremented by 1; the heap invariant holds.
   * @throws std::out_of_range if the queue is empty.
   */
  void pop();

  /**
   * @brief Removes all elements and frees all heap nodes.
   * @post empty() == true.
   */
  void clear() {
    clearSubtree(root_);
    root_ = nullptr;
    insertion_queue_head_ = nullptr;
    insertion_queue_tail_ = nullptr;
    size_ = 0;
  }
};

// ---------------------------------------------------------------------------
// Out-of-line implementations
// ---------------------------------------------------------------------------

/// @cond IMPLEMENTATION

template <typename T, typename Compare>
typename PriorityQueue<T, Compare>::HeapNode*
PriorityQueue<T, Compare>::findLastNodeLevelOrder(HeapNode* node) const {
  if (node == nullptr) return nullptr;

  HeapNode* bfs_head = node;
  HeapNode* bfs_tail = node;
  node->next_insert  = nullptr;
  HeapNode* last     = nullptr;

  while (bfs_head != nullptr) {
    HeapNode* current = bfs_head;
    bfs_head = bfs_head->next_insert;
    if (bfs_head == nullptr) bfs_tail = nullptr;
    current->next_insert = nullptr;
    last = current;

    auto enq = [&](HeapNode* n) {
      n->next_insert = nullptr;
      if (bfs_tail == nullptr) { bfs_head = bfs_tail = n; }
      else { bfs_tail->next_insert = n; bfs_tail = n; }
    };
    if (current->left  != nullptr) enq(current->left);
    if (current->right != nullptr) enq(current->right);
  }
  return last;
}

template <typename T, typename Compare>
void PriorityQueue<T, Compare>::pop() {
  if (root_ == nullptr) throw std::out_of_range("PriorityQueue is empty.");

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
    if (parent->right == last) parent->right = nullptr;
    else                       parent->left  = nullptr;
  }
  delete last;
  --size_;

  // Rebuild the insertion-order queue via BFS over the remaining tree.
  insertion_queue_head_ = nullptr;
  insertion_queue_tail_ = nullptr;
  if (root_ != nullptr) {
    HeapNode* bfs_head = root_;
    HeapNode* bfs_tail = root_;
    root_->next_insert = nullptr;

    while (bfs_head != nullptr) {
      HeapNode* current = bfs_head;
      bfs_head = bfs_head->next_insert;
      if (bfs_head == nullptr) bfs_tail = nullptr;
      current->next_insert = nullptr;

      if (current->left == nullptr || current->right == nullptr) {
        enqueueInsertionNode(current);
      }

      auto enq = [&](HeapNode* n) {
        n->next_insert = nullptr;
        if (bfs_tail == nullptr) { bfs_head = bfs_tail = n; }
        else { bfs_tail->next_insert = n; bfs_tail = n; }
      };
      if (current->left  != nullptr) enq(current->left);
      if (current->right != nullptr) enq(current->right);
    }
  }

  bubbleDown(root_);
}

/// @endcond
