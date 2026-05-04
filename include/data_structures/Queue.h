#pragma once
#include "LinkedList.h"

template<typename T>
class Queue{
    private:
        LinkedList<T> l_list;
    public:
        Queue();
        ~Queue();

        void enqueue(const T& value);
        T dequeue();
        bool is_empty() const;
        int get_size() const;
};

template<typename T>
Queue<T>::Queue(){}

template<typename T>
Queue<T>::~Queue(){}

template<typename T>
void Queue<T>::enqueue(const T& value){
    l_list.push_back(value);
}

template<typename T>
T Queue<T>::dequeue(){
    return l_list.pop_front();
}

template<typename T>
bool Queue<T>::is_empty() const{
    return l_list.is_empty();
}

template<typename T>
int Queue<T>::get_size() const{
    return l_list.get_size();
}
