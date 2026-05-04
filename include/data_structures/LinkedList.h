#pragma once
#include <stdexcept>
#include "Node.h"

template<typename T>
class LinkedList{
    private:
        Node<T>* head;
        Node<T>* tail;
        int size;

    public:
        LinkedList();
        ~LinkedList();
        LinkedList(const LinkedList& other);
        LinkedList& operator=(const LinkedList& other);
        LinkedList(LinkedList&& other) noexcept;
        LinkedList& operator=(LinkedList&& other) noexcept;

        T pop_front();
        void push_back(const T& value);
        void push_front(const T& value);
        void remove(const T& value);
        bool is_empty() const{
            return size == 0;
        }
        int get_size() const{
            return size;
        }
        void clear();
};

//constructor
template<typename T>
LinkedList<T>::LinkedList(): head(nullptr), tail(nullptr), size(0){}
//desctructor
template<typename T>
LinkedList<T>::~LinkedList(){
    clear();
}
//copy constructor
template<typename T>
LinkedList<T>::LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), size(0) {
    Node<T>* current = other.head;
    while (current != nullptr) {
        push_back(current->data);
        current = current->next;
    }
}
//copy assignment
template<typename T>
LinkedList<T>& LinkedList<T>::operator=(const LinkedList& other) {
    if (this == &other) {
        return *this;
    }

    clear();
    Node<T>* current = other.head;
    while (current != nullptr) {
        push_back(current->data);
        current = current->next;
    }
    return *this;
}
//move constructor
template<typename T>
LinkedList<T>::LinkedList(LinkedList&& other) noexcept
    : head(other.head), tail(other.tail), size(other.size) {
    other.head = nullptr;
    other.tail = nullptr;
    other.size = 0;
}
//move assignment
template<typename T>
LinkedList<T>& LinkedList<T>::operator=(LinkedList&& other) noexcept {
    if (this == &other) {
        return *this;
    }

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
T LinkedList<T>::pop_front(){
    if(is_empty()){
        throw std::out_of_range("You can not pop from an empty list...");
    }

    Node<T>* temp = head;
    T value = temp->data;
    head = head->next;

    if(head == nullptr){
        tail = nullptr;
    }
    
    delete temp;
    --size;
    return value;
}

template<typename T>
void LinkedList<T>::clear(){
    Node<T>* current = head;

    while(current != nullptr){
        Node<T>* temp = current;
        current = current->next;
        delete temp;
    }
    head = nullptr;
    tail = nullptr;
    size = 0;
}

template<typename T>
void LinkedList<T>::remove(const T& value){
    Node<T>* current = head;
    Node<T>* previous = nullptr;

    if(is_empty()){
        return;
    }

    while (current != nullptr) {
        if(current->data == value){
            if(previous == nullptr){
                head = current->next;
            }
            else{
                previous->next = current->next;
            }


            if(current == tail){
                tail = previous;
            }

            delete current;
            --size;
            return;
        }
        previous = current;
        current = current->next;
    }

}

template<typename T>
void LinkedList<T>::push_front(const T& value){
    Node<T>* new_node = new Node<T>(value);

    if(is_empty()){
        head = new_node;
        tail = new_node;
        ++size;
        return;
    }
    new_node->next = head;
    head = new_node;
    ++size;
}

template<typename T>
void LinkedList<T>::push_back(const T& value){
    Node<T>* new_node = new Node<T>(value);

    if(is_empty()){
        head = new_node;
        tail = new_node;
        size++;
        return;
    }
    tail->next = new_node;
    tail = new_node;
    size++;
}