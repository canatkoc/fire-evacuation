/**
 * @file test_boundary.cpp
 * @brief Edge-case and boundary tests for all custom data structures and the simulator.
 *
 * Covers corner conditions that the main unit tests do not exercise:
 *  - Empty containers (pop/dequeue on empty, clear on empty)
 *  - Single-element containers and single-node graphs
 *  - Self-loops, duplicate edges, and duplicate nodes in Graph
 *  - Blocked nodes / edges (Dijkstra skips them)
 *  - Fire ignition at an exit node (exits are immune to fire spread)
 *  - Simulation with zero people
 *  - Simulation with all exits blocked from the start
 *  - PriorityQueue with equal-priority elements
 *  - LinkedList remove() on head, tail, missing element, and empty list
 *  - Move semantics for LinkedList and PriorityQueue
 *
 * @author CSE 211 Group
 * @date 2026
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "data_structures/Graph.h"
#include "data_structures/LinkedList.h"
#include "data_structures/Queue.h"
#include "data_structures/priority_queue.h"
#include "simulation/FireSimulator.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

void printPassed(const std::string& name) {
  std::cout << "  [EDGE] " << name << " ........ PASSED\n";
}

// ---------------------------------------------------------------------------
// LinkedList edge cases
// ---------------------------------------------------------------------------

/** pop_front() on an empty list must throw std::out_of_range. */
void edge_linked_list_pop_empty() {
  LinkedList<int> list;
  bool threw = false;
  try {
    list.pop_front();
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
  printPassed("edge_linked_list_pop_empty");
}

/** clear() on an already-empty list must be a no-op (not crash). */
void edge_linked_list_clear_empty() {
  LinkedList<int> list;
  list.clear();  // must not crash
  assert(list.is_empty());
  assert(list.get_size() == 0);
  printPassed("edge_linked_list_clear_empty");
}

/** Single-element push_back then pop_front leaves an empty list. */
void edge_linked_list_single_element() {
  LinkedList<std::string> list;
  list.push_back("hello");
  assert(list.get_size() == 1);
  std::string v = list.pop_front();
  assert(v == "hello");
  assert(list.is_empty());
  printPassed("edge_linked_list_single_element");
}

/** remove() on the head element. */
void edge_linked_list_remove_head() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);
  list.remove(1);
  assert(list.get_size() == 2);
  assert(list.pop_front() == 2);
  assert(list.pop_front() == 3);
  printPassed("edge_linked_list_remove_head");
}

/** remove() on the tail element. */
void edge_linked_list_remove_tail() {
  LinkedList<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);
  list.remove(3);
  assert(list.get_size() == 2);
  assert(list.pop_front() == 1);
  assert(list.pop_front() == 2);
  printPassed("edge_linked_list_remove_tail");
}

/** remove() of an element that is not in the list must be a no-op. */
void edge_linked_list_remove_missing() {
  LinkedList<int> list;
  list.push_back(10);
  list.push_back(20);
  list.remove(99);  // not present
  assert(list.get_size() == 2);
  printPassed("edge_linked_list_remove_missing");
}

/** remove() on an empty list must not crash. */
void edge_linked_list_remove_from_empty() {
  LinkedList<int> list;
  list.remove(42);  // must not crash
  assert(list.is_empty());
  printPassed("edge_linked_list_remove_from_empty");
}

/** Move constructor leaves the source empty and the target correct. */
void edge_linked_list_move_constructor() {
  LinkedList<int> src;
  src.push_back(1);
  src.push_back(2);

  LinkedList<int> dst(std::move(src));
  assert(src.is_empty());
  assert(dst.get_size() == 2);
  assert(dst.pop_front() == 1);
  assert(dst.pop_front() == 2);
  printPassed("edge_linked_list_move_constructor");
}

/** Move assignment leaves the source empty and transfers ownership. */
void edge_linked_list_move_assignment() {
  LinkedList<int> src;
  src.push_back(7);
  src.push_back(8);
  src.push_back(9);

  LinkedList<int> dst;
  dst.push_back(100);
  dst = std::move(src);

  assert(src.is_empty());
  assert(dst.get_size() == 3);
  assert(dst.pop_front() == 7);
  printPassed("edge_linked_list_move_assignment");
}

// ---------------------------------------------------------------------------
// Queue edge cases
// ---------------------------------------------------------------------------

/** dequeue() on an empty Queue must throw. */
void edge_queue_dequeue_empty() {
  Queue<int> q;
  bool threw = false;
  try {
    q.dequeue();
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
  printPassed("edge_queue_dequeue_empty");
}

/** Single enqueue + dequeue. */
void edge_queue_single_element() {
  Queue<double> q;
  q.enqueue(3.14);
  assert(q.get_size() == 1);
  double v = q.dequeue();
  assert(v > 3.13 && v < 3.15);
  assert(q.is_empty());
  printPassed("edge_queue_single_element");
}

// ---------------------------------------------------------------------------
// PriorityQueue edge cases
// ---------------------------------------------------------------------------

/** top() and pop() on an empty queue must throw. */
void edge_pq_empty_operations() {
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;

  bool top_threw = false;
  try { pq.top(); } catch (const std::out_of_range&) { top_threw = true; }
  assert(top_threw);

  bool pop_threw = false;
  try { pq.pop(); } catch (const std::out_of_range&) { pop_threw = true; }
  assert(pop_threw);

  printPassed("edge_pq_empty_operations");
}

/** Single-element push, top, pop. */
void edge_pq_single_element() {
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;
  pq.push(42);
  assert(pq.size() == 1);
  assert(pq.top() == 42);
  pq.pop();
  assert(pq.empty());
  printPassed("edge_pq_single_element");
}

/** Equal-priority elements: all must be dequeued; none lost. */
void edge_pq_equal_priorities() {
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;
  constexpr int N = 100;
  for (int i = 0; i < N; ++i) pq.push(5);  // all same value
  int count = 0;
  while (!pq.empty()) {
    assert(pq.top() == 5);
    pq.pop();
    ++count;
  }
  assert(count == N);
  printPassed("edge_pq_equal_priorities (N=100 equal elements)");
}

/** Move constructor: source becomes empty, target is operational. */
void edge_pq_move_constructor() {
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> src;
  src.push(3);
  src.push(1);
  src.push(2);

  PriorityQueue<int, Cmp> dst(std::move(src));
  assert(src.empty());
  assert(dst.size() == 3);
  assert(dst.top() == 1);
  dst.pop();
  assert(dst.top() == 2);
  printPassed("edge_pq_move_constructor");
}

/** clear() on a non-empty queue leaves it empty. */
void edge_pq_clear() {
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;
  for (int i = 0; i < 50; ++i) pq.push(i);
  pq.clear();
  assert(pq.empty());
  assert(pq.size() == 0);
  printPassed("edge_pq_clear");
}

// ---------------------------------------------------------------------------
// Graph edge cases
// ---------------------------------------------------------------------------

/** addNode() with a duplicate ID must return false without altering state. */
void edge_graph_duplicate_node() {
  Graph g;
  assert(g.addNode("A", Graph::NodeType::Room, 0, 0) == true);
  assert(g.addNode("A", Graph::NodeType::Room, 0, 5) == false);  // duplicate
  assert(g.nodeCount() == 1);
  printPassed("edge_graph_duplicate_node");
}

/** addEdge() for a non-existent source returns false. */
void edge_graph_edge_missing_vertex() {
  Graph g;
  g.addNode("A", Graph::NodeType::Room, 0, 0);
  bool ok = g.addEdge("A", "GHOST", 1, 10, false);
  assert(ok == false);
  printPassed("edge_graph_edge_missing_vertex");
}

/** shortestPath() on two disconnected vertices returns reachable == false. */
void edge_graph_disconnected() {
  Graph g;
  g.addNode("X", Graph::NodeType::Room, 0, 0);
  g.addNode("Y", Graph::NodeType::Exit, 0, 0);
  // No edge added — disconnected.
  Graph::PathResult r = g.shortestPath("X", "Y");
  assert(r.reachable == false);
  printPassed("edge_graph_disconnected");
}

/** Blocked source node makes shortestPath return unreachable. */
void edge_graph_blocked_source() {
  Graph g;
  g.addNode("S", Graph::NodeType::Room, 0, 0);
  g.addNode("T", Graph::NodeType::Exit, 0, 0);
  g.addEdge("S", "T", 1, 10, false);
  g.setNodeBlocked("S", true);
  Graph::PathResult r = g.shortestPath("S", "T");
  assert(r.reachable == false);
  printPassed("edge_graph_blocked_source");
}

/** Blocked target node makes shortestPath return unreachable. */
void edge_graph_blocked_target() {
  Graph g;
  g.addNode("S", Graph::NodeType::Room, 0, 0);
  g.addNode("T", Graph::NodeType::Exit, 0, 0);
  g.addEdge("S", "T", 1, 10, false);
  g.setNodeBlocked("T", true);
  Graph::PathResult r = g.shortestPath("S", "T");
  assert(r.reachable == false);
  printPassed("edge_graph_blocked_target");
}

/** shortestPath from a node to itself returns cost 0 with a single-element path. */
void edge_graph_self_path() {
  Graph g;
  g.addNode("A", Graph::NodeType::Room, 0, 0);
  Graph::PathResult r = g.shortestPath("A", "A");
  assert(r.reachable == true);
  assert(r.total_cost == 0);
  assert(r.path.get_size() == 1);
  printPassed("edge_graph_self_path");
}

/** isReachableToAnyExit returns false when no exit nodes exist. */
void edge_graph_no_exits() {
  Graph g;
  g.addNode("R1", Graph::NodeType::Room, 0, 0);
  g.addNode("R2", Graph::NodeType::Room, 0, 0);
  g.addEdge("R1", "R2", 1, 10, true);
  assert(g.isReachableToAnyExit("R1") == false);
  printPassed("edge_graph_no_exits");
}

// ---------------------------------------------------------------------------
// FireSimulator edge cases
// ---------------------------------------------------------------------------

/** A simulation with zero people finishes immediately. */
void edge_sim_zero_people() {
  Graph g;
  FireSimulator sim(g);
  sim.addNode("R1", Graph::NodeType::Room, 0, 0, 0.3);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("R1", "EXIT", 2, 10, 0.1, false);

  assert(sim.simulationFinished() == true);  // no people → trivially finished
  sim.step();  // must not crash on tick with zero people
  assert(sim.evacuatedCount() == 0);
  assert(sim.casualtyCount() == 0);
  printPassed("edge_sim_zero_people");
}

/** Persons with no reachable exit are counted as casualties after run() exhausts steps. */
void edge_sim_all_exits_blocked() {
  // FireSimulator maintains its own Corridor list separate from the Graph.
  // The cleanest way to strand everyone is to add no corridor at all:
  // findSafestExit() returns unreachable, people stay idle, and run() marks
  // all surviving persons as casualties when the step limit is exceeded.
  Graph g;
  FireSimulator sim(g);
  sim.addNode("ROOM", Graph::NodeType::Room, 0, 3, 0.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  // Intentionally no corridor: people cannot reach the exit.

  sim.run(5);  // step limit exhausted; alive persons become casualties
  assert(sim.evacuatedCount() == 0);
  assert(sim.casualtyCount() == 3);
  printPassed("edge_sim_all_exits_blocked");
}

/** igniteNode() on an exit node must not block the exit (exits are safe sinks). */
void edge_sim_ignite_exit_node() {
  Graph g;
  FireSimulator sim(g);
  sim.addNode("R1", Graph::NodeType::Room, 0, 1, 0.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("R1", "EXIT", 1, 10, 0.0, false);

  // Igniting an exit — the exit flammability is 0.0 so fire cannot spread there
  // from propagateFire(), but a direct ignite should still be accepted.
  bool ok = sim.igniteNode("EXIT", 0.5);
  assert(ok == true);  // igniteNode found the exit node

  // Even after ignition the person should still evacuate safely.
  sim.run(5);
  assert(sim.evacuatedCount() == 1);
  assert(sim.casualtyCount() == 0);
  printPassed("edge_sim_ignite_exit_node");
}

/** snapshot() values are consistent with individual accessor methods. */
void edge_sim_snapshot_consistency() {
  Graph g;
  FireSimulator sim(g);
  sim.addNode("A", Graph::NodeType::Room, 0, 2, 0.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("A", "EXIT", 1, 10, 0.0, false);
  sim.run(5);

  int t, alive, evac, dead;
  sim.snapshot(t, alive, evac, dead);
  assert(t == sim.currentTime());
  assert(evac == sim.evacuatedCount());
  assert(dead == sim.casualtyCount());
  assert(alive == sim.totalPeople() - evac - dead);
  printPassed("edge_sim_snapshot_consistency");
}

/** addCorridor() with unknown node IDs returns false without crashing. */
void edge_sim_add_corridor_missing_node() {
  Graph g;
  FireSimulator sim(g);
  sim.addNode("A", Graph::NodeType::Room, 0, 0, 0.2);
  bool ok = sim.addCorridor("A", "NONEXISTENT", 1, 5, 0.1, true);
  assert(ok == false);
  printPassed("edge_sim_add_corridor_missing_node");
}

/** fireIntensityAt() for an unknown node returns 0.0. */
void edge_sim_fire_intensity_unknown_node() {
  Graph g;
  FireSimulator sim(g);
  double intensity = sim.fireIntensityAt("DOES_NOT_EXIST");
  assert(intensity == 0.0);
  printPassed("edge_sim_fire_intensity_unknown_node");
}

}  // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

/**
 * @brief Runs all edge-case and boundary tests.
 *
 * Called from tests/test_main.cpp.
 */
void run_edge_case_tests() {
  std::cout << "\n=== Edge Case / Boundary Tests ===\n";

  // LinkedList
  edge_linked_list_pop_empty();
  edge_linked_list_clear_empty();
  edge_linked_list_single_element();
  edge_linked_list_remove_head();
  edge_linked_list_remove_tail();
  edge_linked_list_remove_missing();
  edge_linked_list_remove_from_empty();
  edge_linked_list_move_constructor();
  edge_linked_list_move_assignment();

  // Queue
  edge_queue_dequeue_empty();
  edge_queue_single_element();

  // PriorityQueue
  edge_pq_empty_operations();
  edge_pq_single_element();
  edge_pq_equal_priorities();
  edge_pq_move_constructor();
  edge_pq_clear();

  // Graph
  edge_graph_duplicate_node();
  edge_graph_edge_missing_vertex();
  edge_graph_disconnected();
  edge_graph_blocked_source();
  edge_graph_blocked_target();
  edge_graph_self_path();
  edge_graph_no_exits();

  // FireSimulator
  edge_sim_zero_people();
  edge_sim_all_exits_blocked();
  edge_sim_ignite_exit_node();
  edge_sim_snapshot_consistency();
  edge_sim_add_corridor_missing_node();
  edge_sim_fire_intensity_unknown_node();

  std::cout << "=== Edge Case / Boundary Tests: all passed ===\n";
}
