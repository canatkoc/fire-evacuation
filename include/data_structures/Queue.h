/**
 * @file Queue.h
 * @brief Generic FIFO queue built on LinkedList.
 *
 * Implements a FIFO queue by composition on the custom LinkedList.
 * No STL containers are used as primary storage.
 * Used in the evacuation simulator to model people waiting at
 * over-capacity corridors and stairwells.
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once
#include "LinkedList.h"

/**
 * @brief A generic first-in, first-out (FIFO) queue.
 *
 * Backed by a pointer-based LinkedList. Enqueue is O(1) (push_back),
 * dequeue is O(1) (pop_front). The queue owns the memory of its elements
 * through the underlying list.
 *
 * @tparam T The element type. Must be copy-constructible.
 */
template<typename T>
class Queue {
 private:
    LinkedList<T> l_list; ///< Underlying linked list providing O(1) front/back access.

 public:
    /**
     * @brief Default constructor -- creates an empty queue.
     * @post is_empty() == true.
     */
    Queue();

    /**
     * @brief Destructor -- releases all enqueued elements via the linked list.
     * @post All heap memory owned by this queue is freed.
     */
    ~Queue();

    /**
     * @brief Adds @p value to the back of the queue.
     *
     * @param value The element to enqueue.
     * @post get_size() is incremented by 1; @p value is accessible via
     *       dequeue() after all previously enqueued elements are removed.
     */
    void enqueue(const T& value);

    /**
     * @brief Removes and returns the element at the front of the queue.
     *
     * @pre  !is_empty()
     * @post get_size() is decremented by 1.
     * @return The element that has been in the queue the longest.
     * @throws std::out_of_range if the queue is empty.
     */
    T dequeue();

    /**
     * @brief Reports whether the queue has no elements.
     * @return true if the queue is empty, false otherwise.
     */
    bool is_empty() const;

    /**
     * @brief Returns the number of elements currently in the queue.
     * @return A non-negative element count.
     */
    int get_size() const;
};

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

/// @cond IMPLEMENTATION

template<typename T>
Queue<T>::Queue() {}

template<typename T>
Queue<T>::~Queue() {}

template<typename T>
void Queue<T>::enqueue(const T& value) {
    l_list.push_back(value);
}

template<typename T>
T Queue<T>::dequeue() {
    return l_list.pop_front();
}

template<typename T>
bool Queue<T>::is_empty() const {
    return l_list.is_empty();
}

template<typename T>
int Queue<T>::get_size() const {
    return l_list.get_size();
}

/// @endcond
