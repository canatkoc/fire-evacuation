#pragma once

template <typename T>
struct Node {
    T     data;
    Node<T>* next;

    explicit Node(const T& value, Node<T>* nextPtr = nullptr)
        : data(value), next(nextPtr) {}
};