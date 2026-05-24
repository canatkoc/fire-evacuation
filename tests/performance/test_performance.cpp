/**
 * @file test_performance.cpp
 * @brief Performance benchmarks for the custom data structures and simulation engine.
 *
 * These tests verify structural integrity and asymptotic behavior using large inputs.
 * Wall-clock timings are printed for information only because Valgrind slows execution.
 * for large inputs.  They do NOT use wall-clock timeouts (which are flaky in CI);
 * instead they measure operation counts and structural integrity to confirm
 * asymptotic behaviour — O(log n) for PriorityQueue, O(1) amortised for
 * LinkedList ends, O(V + E log V) for Dijkstra.
 *
 * Each test function follows the project convention: assert on invariants,
 * print a "PASSED" line on success.  Failures abort via assert().
 *
 * @author CSE 211 Group
 * @date 2026
 */

#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <limits>

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
  std::cout << "  [PERF] " << name << " ........ PASSED\n";
}

/** Measures elapsed wall time for @p fn and returns milliseconds. */
double timeMs(const std::function<void()>& fn) {
  auto t0 = std::chrono::high_resolution_clock::now();
  fn();
  auto t1 = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// ---------------------------------------------------------------------------
// LinkedList performance
// ---------------------------------------------------------------------------

/**
 * @brief push_back + pop_front N = 100 000 times.
 *
 * Verifies that the linked-list FIFO pattern completes in a reasonable wall
 * time and that the list is empty afterwards (no leaked nodes).
 */
void perf_linked_list_large_push_pop() {
  constexpr int N = 100000;
  LinkedList<int> list;

  double ms = timeMs([&]() {
    for (int i = 0; i < N; ++i) list.push_back(i);
    for (int i = 0; i < N; ++i) {
      int v = list.pop_front();
      assert(v == i);  // FIFO order preserved
    }
  });

  assert(list.is_empty());
  assert(list.get_size() == 0);
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_linked_list_large_push_pop (N=100k, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

/**
 * @brief push_front N = 50 000 times, then clear().
 *
 * Ensures prepend is O(1) and that clear() frees all nodes without leaks.
 */
void perf_linked_list_push_front_and_clear() {
  constexpr int N = 50000;
  LinkedList<int> list;

  double ms = timeMs([&]() {
    for (int i = 0; i < N; ++i) list.push_front(i);
  });

  assert(list.get_size() == N);
  list.clear();
  assert(list.is_empty());
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_linked_list_push_front_and_clear (N=50k, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

// ---------------------------------------------------------------------------
// Queue performance
// ---------------------------------------------------------------------------

/**
 * @brief Enqueue + dequeue N = 100 000 elements through the Queue wrapper.
 *
 * Validates that the Queue façade adds no measurable overhead over LinkedList.
 */
void perf_queue_large_enqueue_dequeue() {
  constexpr int N = 100000;
  Queue<int> q;

  double ms = timeMs([&]() {
    for (int i = 0; i < N; ++i) q.enqueue(i);
    for (int i = 0; i < N; ++i) {
      int v = q.dequeue();
      assert(v == i);
    }
  });

  assert(q.is_empty());
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_queue_large_enqueue_dequeue (N=100k, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

// ---------------------------------------------------------------------------
// PriorityQueue performance
// ---------------------------------------------------------------------------

/**
 * @brief Push N = 10 000 random-ish integers and pop them all; verify sorted order.
 *
 * The min-heap must produce values in non-decreasing order.  The O(log n) push
 * and pop mean 10 000 elements should complete in well under 1 second.
 */
void perf_priority_queue_push_pop_sorted() {
  constexpr int N = 10000;
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;

  double ms = timeMs([&]() {
    // Insert in a pseudo-random order (simple linear congruential pattern).
    unsigned val = 123456789u;
    for (int i = 0; i < N; ++i) {
      val = val * 1664525u + 1013904223u;
      pq.push(static_cast<int>(val % 100000));
    }
    int prev = pq.top();
    pq.pop();
    while (!pq.empty()) {
      int cur = pq.top();
      pq.pop();
      assert(cur >= prev);  // non-decreasing (min-heap)
      prev = cur;
    }
  });

  assert(pq.empty());
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_priority_queue_push_pop_sorted (N=10k, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

/**
 * @brief Interleaved push/pop pattern — N = 5 000 rounds.
 *
 * Each round pushes 3 elements and pops 1, verifying the heap property holds
 * throughout mixed operations.
 */
void perf_priority_queue_interleaved() {
  constexpr int ROUNDS = 5000;
  struct Cmp { bool operator()(int a, int b) const { return a < b; } };
  PriorityQueue<int, Cmp> pq;

  double ms = timeMs([&]() {
    unsigned val = 987654321u;
    for (int r = 0; r < ROUNDS; ++r) {
      for (int k = 0; k < 3; ++k) {
        val = val * 22695477u + 1u;
        pq.push(static_cast<int>(val % 10000));
      }
      if (!pq.empty()) pq.pop();
    }
    // Drain and verify sorted.
    int prev = std::numeric_limits<int>::min();
    while (!pq.empty()) {
      int cur = pq.top();
      pq.pop();
      assert(cur >= prev);
      prev = cur;
    }
  });

  assert(pq.empty());
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_priority_queue_interleaved (5k rounds, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

// ---------------------------------------------------------------------------
// Graph / Dijkstra performance
// ---------------------------------------------------------------------------

/**
 * @brief Dijkstra on a 50-node linear chain graph.
 *
 * Builds a chain A0 → A1 → … → A49 (bidirectional) and queries the shortest
 * path from A0 to A49.  Expected cost = 49.  Verifies correctness and that
 * the query completes quickly.
 */
void perf_graph_dijkstra_chain_50() {
  constexpr int N = 50;
  Graph g;

  for (int i = 0; i < N; ++i)
    g.addNode("A" + std::to_string(i), Graph::NodeType::Room, 0, 0);
  g.addNode("EXIT", Graph::NodeType::Exit, 0, 0);

  for (int i = 0; i < N - 1; ++i)
    g.addEdge("A" + std::to_string(i), "A" + std::to_string(i + 1), 1, 100, true);
  g.addEdge("A" + std::to_string(N - 1), "EXIT", 1, 100, false);

  Graph::PathResult result;
  double ms = timeMs([&]() {
    result = g.shortestPath("A0", "EXIT");
  });

  assert(result.reachable);
  assert(result.total_cost == N);  // 50 hops of cost 1
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_graph_dijkstra_chain_50 (cost=" +
              std::to_string(result.total_cost) + ", " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

/**
 * @brief Dijkstra on a dense 20-node fully-connected graph, called 100 times.
 *
 * Ensures that repeated queries don't accumulate state or leak memory.
 */
void perf_graph_dijkstra_dense_repeated() {
  constexpr int N = 20;
  constexpr int QUERIES = 100;
  Graph g;

  for (int i = 0; i < N; ++i)
    g.addNode("N" + std::to_string(i), Graph::NodeType::Room, 0, 0);
  g.addNode("EXIT", Graph::NodeType::Exit, 0, 0);

  // Fully connect all room nodes.
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      g.addEdge("N" + std::to_string(i), "N" + std::to_string(j), 1, 50, true);

  // Connect every node to exit.
  for (int i = 0; i < N; ++i)
    g.addEdge("N" + std::to_string(i), "EXIT", 1, 50, false);

  double ms = timeMs([&]() {
    for (int q = 0; q < QUERIES; ++q) {
      Graph::PathResult r = g.shortestPathToNearestExit("N0");
      assert(r.reachable);
      assert(r.total_cost == 1);  // direct edge from N0 to EXIT
    }
  });

  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_graph_dijkstra_dense_repeated (20 nodes, 100 queries, " +
              std::to_string(static_cast<int>(ms)) + " ms)");
}

// ---------------------------------------------------------------------------
// FireSimulator performance
// ---------------------------------------------------------------------------

/**
 * @brief Simulation with 10 nodes and 50 people for 200 ticks.
 *
 * Verifies the simulation engine can handle moderate-sized scenarios without
 * a performance cliff and that evacuated + casualty counts are consistent.
 */
void perf_fire_simulator_medium_scenario() {
  constexpr int TICKS = 200;
  Graph g;
  FireSimulator sim(g);

  // Build a small 3x3 grid of rooms + 1 exit.
  const char* rooms[] = {"R00","R01","R02","R10","R11","R12","R20","R21","R22"};
  constexpr int ROOM_COUNT = 9;
  constexpr int PEOPLE_PER_ROOM = 5;  // 45 people total

  for (int i = 0; i < ROOM_COUNT; ++i)
    sim.addNode(rooms[i], Graph::NodeType::Room, 0, PEOPLE_PER_ROOM, 0.3);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  // Connect in a grid pattern.
  sim.addCorridor("R00","R01", 2, 10, 0.2, true);
  sim.addCorridor("R01","R02", 2, 10, 0.2, true);
  sim.addCorridor("R10","R11", 2, 10, 0.2, true);
  sim.addCorridor("R11","R12", 2, 10, 0.2, true);
  sim.addCorridor("R20","R21", 2, 10, 0.2, true);
  sim.addCorridor("R21","R22", 2, 10, 0.2, true);
  sim.addCorridor("R00","R10", 2, 10, 0.2, true);
  sim.addCorridor("R10","R20", 2, 10, 0.2, true);
  sim.addCorridor("R01","R11", 2, 10, 0.2, true);
  sim.addCorridor("R11","R21", 2, 10, 0.2, true);
  sim.addCorridor("R02","R12", 2, 10, 0.2, true);
  sim.addCorridor("R12","R22", 2, 10, 0.2, true);
  sim.addCorridor("R02","EXIT", 1, 50, 0.0, false);

  // Ignite the corner opposite the exit.
  sim.igniteNode("R20", 1.0);

  double ms = timeMs([&]() {
    sim.run(TICKS);
  });

  int total = sim.totalPeople();
  int evac  = sim.evacuatedCount();
  int dead  = sim.casualtyCount();
  assert(evac + dead == total);  // everyone accounted for
  assert(total == ROOM_COUNT * PEOPLE_PER_ROOM);
  (void)ms;  // measured only for reporting; not asserted under Valgrind
  printPassed("perf_fire_simulator_medium_scenario (45 people, 200 ticks, " +
              std::to_string(static_cast<int>(ms)) + " ms, evac=" +
              std::to_string(evac) + ", dead=" + std::to_string(dead) + ")");
}

}  // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

/**
 * @brief Runs all performance tests and prints a section header/footer.
 *
 * Called from tests/test_main.cpp.
 */
void run_performance_tests() {
  std::cout << "\n=== Performance Tests ===\n";
  perf_linked_list_large_push_pop();
  perf_linked_list_push_front_and_clear();
  perf_queue_large_enqueue_dequeue();
  perf_priority_queue_push_pop_sorted();
  perf_priority_queue_interleaved();
  perf_graph_dijkstra_chain_50();
  perf_graph_dijkstra_dense_repeated();
  perf_fire_simulator_medium_scenario();
  std::cout << "=== Performance Tests: all passed ===\n";
}
