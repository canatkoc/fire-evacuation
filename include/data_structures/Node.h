/**
 * @file Node.h
 * @brief Generic singly-linked node used by LinkedList and Queue.
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once

/**
 * @brief A single node in a singly-linked list.
 *
 * Stores one element of type T and a raw pointer to the next node.
 * Ownership of the next pointer is managed by the containing LinkedList.
 *
 * @tparam T The type of data stored in the node.
 */
template <typename T>
struct Node {
    T        data; ///< The value stored in this node.
    Node<T>* next; ///< Raw pointer to the next node; nullptr if this is the last node.

    /**
     * @brief Constructs a Node with the given value and optional successor.
     *
     * @pre  None.
     * @post A Node object is created holding @p value.
     * @param value   The data to store.
     * @param nextPtr Pointer to the next node (defaults to nullptr).
     */
    explicit Node(const T& value, Node<T>* nextPtr = nullptr)
        : data(value), next(nextPtr) {}
};
