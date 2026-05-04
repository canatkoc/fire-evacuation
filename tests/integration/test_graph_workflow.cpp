#include <cassert>
#include <iostream>
#include <string>

#include "data_structures/Graph.h"

namespace {

void printTestPassed(const std::string& test_name) {
  std::cout << test_name << " ........ PASSED\n";
}

void runAndReport(const std::string& test_name, void (*test_func)()) {
  test_func();
  printTestPassed(test_name);
}

Graph buildBaseGraph() {
  Graph graph;

  assert(graph.addNode("R1", Graph::NodeType::Room, 1, 5));
  assert(graph.addNode("R2", Graph::NodeType::Room, 1, 3));
  assert(graph.addNode("H1", Graph::NodeType::Hallway, 1, 0));
  assert(graph.addNode("S1", Graph::NodeType::Stairwell, 1, 0));
  assert(graph.addNode("E1", Graph::NodeType::Exit, 1, 0));
  assert(graph.addNode("E2", Graph::NodeType::Exit, 1, 0));

  assert(graph.addEdge("R1", "H1", 1, 3));
  assert(graph.addEdge("R2", "H1", 1, 3));
  assert(graph.addEdge("H1", "S1", 2, 2));
  assert(graph.addEdge("S1", "E1", 2, 2));
  assert(graph.addEdge("H1", "E2", 5, 4));

  return graph;
}

void test_add_node_and_duplicate_rejection() {
  Graph graph;
  assert(graph.addNode("R1", Graph::NodeType::Room, 1, 5));
  assert(!graph.addNode("R1", Graph::NodeType::Room, 1, 5));
}

void test_add_edge_valid_and_invalid_nodes() {
  Graph graph;
  assert(graph.addNode("A", Graph::NodeType::Room, 1, 1));
  assert(graph.addNode("B", Graph::NodeType::Exit, 1, 0));
  assert(graph.addEdge("A", "B", 1, 2));
  assert(!graph.addEdge("A", "MISSING", 1, 2));
}

void test_node_count_is_correct() {
  Graph graph = buildBaseGraph();
  assert(graph.nodeCount() == 6);
}

void test_shortest_path_r1_to_e1_cost_is_five() {
  Graph graph = buildBaseGraph();
  Graph::PathResult result = graph.shortestPath("R1", "E1");

  assert(result.reachable);
  assert(result.total_cost == 5);
}

void test_shortest_path_to_nearest_exit_prefers_e1() {
  Graph graph = buildBaseGraph();
  Graph::PathResult nearest = graph.shortestPathToNearestExit("R1");

  assert(nearest.reachable);
  assert(nearest.total_cost == 5);
}

void test_blocked_stairwell_forces_alternative_exit() {
  Graph graph = buildBaseGraph();
  assert(graph.setNodeBlocked("S1", true));

  Graph::PathResult to_e1 = graph.shortestPath("R1", "E1");
  Graph::PathResult nearest = graph.shortestPathToNearestExit("R1");

  assert(!to_e1.reachable);
  assert(nearest.reachable);
  assert(nearest.total_cost == 6);
}

void test_blocked_edge_closes_e2_path() {
  Graph graph = buildBaseGraph();
  assert(graph.setNodeBlocked("S1", true));
  assert(graph.setEdgeBlocked("H1", "E2", true));

  Graph::PathResult nearest = graph.shortestPathToNearestExit("R1");
  assert(!nearest.reachable);
}

void test_reachable_to_any_exit_reflects_state() {
  Graph graph = buildBaseGraph();
  assert(graph.isReachableToAnyExit("R1"));

  assert(graph.setNodeBlocked("S1", true));
  assert(graph.setEdgeBlocked("H1", "E2", true));
  assert(!graph.isReachableToAnyExit("R1"));
}

void test_graph_clear_resets_node_count() {
  Graph graph = buildBaseGraph();
  assert(graph.nodeCount() == 6);
  graph.clear();
  assert(graph.nodeCount() == 0);
}

}  // namespace

void run_graph_integration_tests() {
  std::cout << "\n[INTEGRATION] Graph\n";
  runAndReport("test_add_node_and_duplicate_rejection", test_add_node_and_duplicate_rejection);
  runAndReport("test_add_edge_valid_and_invalid_nodes", test_add_edge_valid_and_invalid_nodes);
  runAndReport("test_node_count_is_correct", test_node_count_is_correct);
  runAndReport("test_shortest_path_r1_to_e1_cost_is_five", test_shortest_path_r1_to_e1_cost_is_five);
  runAndReport("test_shortest_path_to_nearest_exit_prefers_e1", test_shortest_path_to_nearest_exit_prefers_e1);
  runAndReport("test_blocked_stairwell_forces_alternative_exit", test_blocked_stairwell_forces_alternative_exit);
  runAndReport("test_blocked_edge_closes_e2_path", test_blocked_edge_closes_e2_path);
  runAndReport("test_reachable_to_any_exit_reflects_state", test_reachable_to_any_exit_reflects_state);
  runAndReport("test_graph_clear_resets_node_count", test_graph_clear_resets_node_count);
}
