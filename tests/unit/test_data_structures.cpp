#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "data_structures/Graph.h"
#include "data_structures/LinkedList.h"
#include "data_structures/Queue.h"
#include "data_structures/priority_queue.h"

namespace {

void printTestPassed(const std::string& test_name) {
  std::cout << test_name << " ........ PASSED\n";
}

void runAndReport(const std::string& test_name, void (*test_func)()) {
  test_func();
  printTestPassed(test_name);
}

void test_linked_list_empty_initial_state() {
  LinkedList<int> list;
  assert(list.is_empty());
  assert(list.get_size() == 0);
}

void test_linked_list_push_back_and_pop_front_order() {
  LinkedList<int> list;
  list.push_back(10);
  list.push_back(20);
  list.push_back(30);

  assert(list.get_size() == 3);
  assert(list.pop_front() == 10);
  assert(list.pop_front() == 20);
  assert(list.pop_front() == 30);
  assert(list.is_empty());
}

void test_linked_list_push_front_and_pop_front_order() {
  LinkedList<int> list;
  list.push_front(10);
  list.push_front(20);
  list.push_front(30);

  assert(list.pop_front() == 30);
  assert(list.pop_front() == 20);
  assert(list.pop_front() == 10);
  assert(list.is_empty());
}

void test_linked_list_pop_front_single_item_leaves_empty() {
  LinkedList<int> list;
  list.push_back(7);
  assert(list.pop_front() == 7);
  assert(list.is_empty());
  assert(list.get_size() == 0);
}

void test_linked_list_pop_front_empty_throws() {
  LinkedList<int> list;
  bool threw = false;
  try {
    (void)list.pop_front();
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
}

void test_linked_list_remove_head() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  list.remove(1);
  assert(list.get_size() == 2);
  assert(list.pop_front() == 2);
  assert(list.pop_front() == 3);
}

void test_linked_list_remove_middle() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  list.remove(2);
  assert(list.get_size() == 2);
  assert(list.pop_front() == 1);
  assert(list.pop_front() == 3);
}

void test_linked_list_remove_tail() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  list.remove(3);
  assert(list.get_size() == 2);
  assert(list.pop_front() == 1);
  assert(list.pop_front() == 2);
}

void test_linked_list_remove_non_existent_keeps_size() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  const int size_before = list.get_size();
  list.remove(999);
  assert(list.get_size() == size_before);
}

void test_linked_list_clear_resets_state() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);

  list.clear();
  assert(list.is_empty());
  assert(list.get_size() == 0);
}

void test_linked_list_copy_constructor_deep_copy() {
  LinkedList<int> original;
  original.push_back(10);
  original.push_back(20);
  original.push_back(30);

  LinkedList<int> copy(original);
  original.remove(20);

  assert(original.get_size() == 2);
  assert(copy.get_size() == 3);
  assert(copy.pop_front() == 10);
  assert(copy.pop_front() == 20);
  assert(copy.pop_front() == 30);
}

void test_linked_list_copy_assignment_deep_copy() {
  LinkedList<int> source;
  source.push_back(5);
  source.push_back(6);
  source.push_back(7);

  LinkedList<int> target;
  target.push_back(100);
  target = source;
  source.remove(6);

  assert(source.get_size() == 2);
  assert(target.get_size() == 3);
  assert(target.pop_front() == 5);
  assert(target.pop_front() == 6);
  assert(target.pop_front() == 7);
}

void test_linked_list_move_constructor_transfers_ownership() {
  LinkedList<int> source;
  source.push_back(11);
  source.push_back(22);

  LinkedList<int> moved(std::move(source));
  assert(source.is_empty());
  assert(source.get_size() == 0);
  assert(moved.get_size() == 2);
  assert(moved.pop_front() == 11);
  assert(moved.pop_front() == 22);
}

void test_linked_list_move_assignment_transfers_ownership() {
  LinkedList<int> source;
  source.push_back(44);
  source.push_back(55);

  LinkedList<int> target;
  target.push_back(99);
  target = std::move(source);

  assert(source.is_empty());
  assert(source.get_size() == 0);
  assert(target.get_size() == 2);
  assert(target.pop_front() == 44);
  assert(target.pop_front() == 55);
}

void test_queue_empty_initial_state() {
  Queue<int> queue;
  assert(queue.is_empty());
  assert(queue.get_size() == 0);
}

void test_queue_fifo_order() {
  Queue<int> queue;
  queue.enqueue(10);
  queue.enqueue(20);
  queue.enqueue(30);

  assert(queue.get_size() == 3);
  assert(queue.dequeue() == 10);
  assert(queue.dequeue() == 20);
  assert(queue.dequeue() == 30);
  assert(queue.is_empty());
}

void test_queue_size_decreases_after_dequeue() {
  Queue<int> queue;
  queue.enqueue(1);
  queue.enqueue(2);
  queue.enqueue(3);

  assert(queue.get_size() == 3);
  (void)queue.dequeue();
  assert(queue.get_size() == 2);
  (void)queue.dequeue();
  assert(queue.get_size() == 1);
}

void test_queue_dequeue_empty_throws() {
  Queue<int> queue;
  bool threw = false;
  try {
    (void)queue.dequeue();
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
}

struct MinCompareInt {
  bool operator()(int lhs, int rhs) const { return lhs < rhs; }
};

void test_priority_queue_empty_initial_state() {
  PriorityQueue<int, MinCompareInt> pq;
  assert(pq.empty());
  assert(pq.size() == static_cast<std::size_t>(0));
}

void test_priority_queue_min_order_with_mixed_values() {
  PriorityQueue<int, MinCompareInt> pq;
  pq.push(50);
  pq.push(10);
  pq.push(30);
  pq.push(5);
  pq.push(20);

  assert(pq.size() == static_cast<std::size_t>(5));
  assert(pq.top() == 5);
  pq.pop();
  assert(pq.top() == 10);
  pq.pop();
  assert(pq.top() == 20);
  pq.pop();
  assert(pq.top() == 30);
  pq.pop();
  assert(pq.top() == 50);
  pq.pop();
  assert(pq.empty());
}

void test_priority_queue_size_decreases_after_pop() {
  PriorityQueue<int, MinCompareInt> pq;
  pq.push(3);
  pq.push(1);
  pq.push(2);

  assert(pq.size() == static_cast<std::size_t>(3));
  pq.pop();
  assert(pq.size() == static_cast<std::size_t>(2));
  pq.pop();
  assert(pq.size() == static_cast<std::size_t>(1));
}

void test_priority_queue_empty_top_and_pop_throw() {
  PriorityQueue<int, MinCompareInt> pq;

  bool top_threw = false;
  try {
    (void)pq.top();
  } catch (const std::out_of_range&) {
    top_threw = true;
  }
  assert(top_threw);

  bool pop_threw = false;
  try {
    pq.pop();
  } catch (const std::out_of_range&) {
    pop_threw = true;
  }
  assert(pop_threw);
}

void test_priority_queue_duplicate_values_can_be_popped() {
  PriorityQueue<int, MinCompareInt> pq;
  pq.push(10);
  pq.push(10);
  pq.push(5);

  assert(pq.top() == 5);
  pq.pop();
  assert(pq.top() == 10);
  pq.pop();
  assert(pq.top() == 10);
  pq.pop();
  assert(pq.empty());
}

void test_memory_scope_linked_list_create_destroy() {
  {
    LinkedList<int> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
  }
  {
    LinkedList<int> list;
    list.push_front(9);
    list.clear();
  }
}

void test_memory_scope_queue_create_destroy() {
  {
    Queue<int> queue;
    queue.enqueue(10);
    queue.enqueue(20);
    (void)queue.dequeue();
  }
  {
    Queue<int> queue;
    queue.enqueue(1);
  }
}

void test_memory_scope_priority_queue_create_destroy() {
  {
    PriorityQueue<int, MinCompareInt> pq;
    pq.push(10);
    pq.push(3);
    pq.push(7);
    pq.pop();
  }
  {
    PriorityQueue<int, MinCompareInt> pq;
    pq.push(42);
    pq.clear();
  }
}

void test_memory_scope_graph_create_destroy() {
  {
    Graph graph;
    graph.addNode("A", Graph::NodeType::Room, 1, 1);
    graph.addNode("B", Graph::NodeType::Exit, 1, 0);
    graph.addEdge("A", "B", 1, 2);
  }
  {
    Graph graph;
    graph.addNode("X", Graph::NodeType::Room, 1, 2);
    graph.clear();
  }
}

}  // namespace

void run_data_structures_unit_tests() {
  std::cout << "[UNIT] LinkedList\n";
  runAndReport("test_empty_initial_state", test_linked_list_empty_initial_state);
  runAndReport("test_push_back_and_pop_front_order", test_linked_list_push_back_and_pop_front_order);
  runAndReport("test_push_front_and_pop_front_order", test_linked_list_push_front_and_pop_front_order);
  runAndReport("test_pop_front_single_item_leaves_empty", test_linked_list_pop_front_single_item_leaves_empty);
  runAndReport("test_pop_front_empty_throws", test_linked_list_pop_front_empty_throws);
  runAndReport("test_remove_head", test_linked_list_remove_head);
  runAndReport("test_remove_middle", test_linked_list_remove_middle);
  runAndReport("test_remove_tail", test_linked_list_remove_tail);
  runAndReport("test_remove_non_existent_keeps_size", test_linked_list_remove_non_existent_keeps_size);
  runAndReport("test_clear_resets_state", test_linked_list_clear_resets_state);
  runAndReport("test_copy_constructor_deep_copy", test_linked_list_copy_constructor_deep_copy);
  runAndReport("test_copy_assignment_deep_copy", test_linked_list_copy_assignment_deep_copy);
  runAndReport("test_move_constructor_transfers_ownership", test_linked_list_move_constructor_transfers_ownership);
  runAndReport("test_move_assignment_transfers_ownership", test_linked_list_move_assignment_transfers_ownership);

  std::cout << "\n[UNIT] Queue\n";
  runAndReport("test_empty_initial_state", test_queue_empty_initial_state);
  runAndReport("test_fifo_order", test_queue_fifo_order);
  runAndReport("test_size_decreases_after_dequeue", test_queue_size_decreases_after_dequeue);
  runAndReport("test_dequeue_empty_throws", test_queue_dequeue_empty_throws);

  std::cout << "\n[UNIT] PriorityQueue\n";
  runAndReport("test_empty_initial_state", test_priority_queue_empty_initial_state);
  runAndReport("test_min_order_with_mixed_values", test_priority_queue_min_order_with_mixed_values);
  runAndReport("test_size_decreases_after_pop", test_priority_queue_size_decreases_after_pop);
  runAndReport("test_empty_top_and_pop_throw", test_priority_queue_empty_top_and_pop_throw);
  runAndReport("test_duplicate_values_can_be_popped", test_priority_queue_duplicate_values_can_be_popped);

  std::cout << "\n[UNIT] MemoryScope\n";
  runAndReport("test_linked_list_create_destroy", test_memory_scope_linked_list_create_destroy);
  runAndReport("test_queue_create_destroy", test_memory_scope_queue_create_destroy);
  runAndReport("test_priority_queue_create_destroy", test_memory_scope_priority_queue_create_destroy);
  runAndReport("test_graph_create_destroy", test_memory_scope_graph_create_destroy);
}
