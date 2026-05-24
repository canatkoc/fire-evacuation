/**
 * @file LinkedList.h
 * @brief Generic singly-linked list with pointer-based node storage.
 *
 * Implements a singly-linked list from scratch — no STL containers are used
 * as primary storage. The list satisfies the Rule of Five (copy constructor,
 * copy assignment, move constructor, move assignment, destructor).
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once
#include <stdexcept>
#include "Node.h"

/**
 * @brief A generic singly-linked list backed by pointer-based Node objects.
 *
 * Provides O(1) push/pop at both ends and O(n) search/remove.
 * Memory is managed internally: all nodes are heap-allocated and freed by
 * the destructor or @c clear().
 *
 * @tparam T The element type. Must be copy-constructible.
 */
template<typename T>
class LinkedList {
 private:
    Node<T>* head; ///< Pointer to the first node; nullptr if the list is empty.
    Node<T>* tail; ///< Pointer to the last node; nullptr if the list is empty.
    int      size; ///< Number of elements currently stored.

 public:
    /**
     * @brief Default constructor — creates an empty list.
     * @post head == nullptr, tail == nullptr, size == 0.
     */
    LinkedList();

    /**
     * @brief Destructor — frees all nodes.
     * @post All heap memory owned by this list is released.
     */
    ~LinkedList();

    /**
     * @brief Copy constructor — performs a deep copy of @p other.
     * @param other The source list to copy from.
     * @post This list contains independent copies of all elements in @p other.
     */
    LinkedList(const LinkedList& other);

    /**
     * @brief Copy-assignment operator.
     * @param other The source list.
     * @return Reference to this list.
     * @post This list contains independent copies of all elements in @p other.
     *       Previous elements are freed.
     */
    LinkedList& operator=(const LinkedList& other);

    /**
     * @brief Move constructor — takes ownership of @p other's nodes.
     * @param other The source list (left in an empty but valid state).
     * @post This list owns the nodes previously owned by @p other.
     */
    LinkedList(LinkedList&& other) noexcept;

    /**
     * @brief Move-assignment operator.
     * @param other The source list (left in an empty but valid state).
     * @return Reference to this list.
     * @post Previous nodes of this list are freed; ownership transferred from @p other.
     */
    LinkedList& operator=(LinkedList&& other) noexcept;

    /**
     * @brief Removes and returns the element at the front of the list.
     * @pre  !is_empty()
     * @post The former head node is freed; size decremented by 1.
     * @return The value that was stored at the front.
     * @throws std::out_of_range if the list is empty.
     */
    T pop_front();

    /**
     * @brief Appends a new element to the back of the list.
     * @param value The value to append.
     * @post size is incremented by 1; the new node becomes the tail.
     */
    void push_back(const T& value);

    /**
     * @brief Prepends a new element to the front of the list.
     * @param value The value to prepend.
     * @post size is incremented by 1; the new node becomes the head.
     */
    void push_front(const T& value);

    /**
     * @brief Removes the first occurrence of @p value from the list.
     *
     * Does nothing if @p value is not found.
     *
     * @param value The value to search for and remove.
     * @post If the value was present, size is decremented by 1 and the
     *       corresponding node is freed.
     */
    void remove(const T& value);

    /**
     * @brief Reports whether the list is empty.
     * @return true if size == 0, false otherwise.
     */
    bool is_empty() const { return size == 0; }

    /**
     * @brief Returns the number of elements in the list.
     * @return The current element count (>= 0).
     */
    int get_size() const { return size; }

    /**
     * @brief Removes all elements and frees all nodes.
     * @post head == nullptr, tail == nullptr, size == 0.
     */
    void clear();
};

// ---------------------------------------------------------------------------
// Implementation (template — must remain in the header)
// ---------------------------------------------------------------------------

/// @cond IMPLEMENTATION

template<typename T>
LinkedList<T>::LinkedList() : head(nullptr), tail(nullptr), size(0) {}

template<typename T>
LinkedList<T>::~LinkedList() { clear(); }

template<typename T>
LinkedList<T>::LinkedList(const LinkedList& other)
    : head(nullptr), tail(nullptr), size(0) {
    Node<T>* current = other.head;
    while (current != nullptr) {
        push_back(current->data);
        current = current->next;
    }
}

template<typename T>
LinkedList<T>& LinkedList<T>::operator=(const LinkedList& other) {
    if (this == &other) return *this;
    clear();
    Node<T>* current = other.head;
    while (current != nullptr) {
        push_back(current->data);
        current = current->next;
    }
    return *this;
}

template<typename T>
LinkedList<T>::LinkedList(LinkedList&& other) noexcept
    : head(other.head), tail(other.tail), size(other.size) {
    other.head = nullptr;
    other.tail = nullptr;
    other.size = 0;
}

template<typename T>
LinkedList<T>& LinkedList<T>::operator=(LinkedList&& other) noexcept {
    if (this == &other) return *this;
    clear();
    head = other.head;
    tail = other.tail;
    size = other.size;
    other.head = nullptr;
    other.tail = nullptr;
    other.size = 0;
    return *this;
}

template<typename T>
T LinkedList<T>::pop_front() {
    if (is_empty()) {
        throw std::out_of_range("Cannot pop from an empty LinkedList.");
    }
    Node<T>* temp = head;
    T value = temp->data;
    head = head->next;
    if (head == nullptr) tail = nullptr;
    delete temp;
    --size;
    return value;
}

template<typename T>
void LinkedList<T>::clear() {
    Node<T>* current = head;
    while (current != nullptr) {
        Node<T>* temp = current;
        current = current->next;
        delete temp;
    }
    head = nullptr;
    tail = nullptr;
    size = 0;
}

template<typename T>
void LinkedList<T>::remove(const T& value) {
    if (is_empty()) return;
    Node<T>* current  = head;
    Node<T>* previous = nullptr;
    while (current != nullptr) {
        if (current->data == value) {
            if (previous == nullptr) head = current->next;
            else                     previous->next = current->next;
            if (current == tail) tail = previous;
            delete current;
            --size;
            return;
        }
        previous = current;
        current  = current->next;
    }
}

template<typename T>
void LinkedList<T>::push_front(const T& value) {
    Node<T>* new_node = new Node<T>(value);
    if (is_empty()) { head = tail = new_node; }
    else            { new_node->next = head; head = new_node; }
    ++size;
}

template<typename T>
void LinkedList<T>::push_back(const T& value) {
    Node<T>* new_node = new Node<T>(value);
    if (is_empty()) { head = tail = new_node; }
    else            { tail->next = new_node; tail = new_node; }
    ++size;
}

/// @endcond
