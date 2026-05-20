#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

#include "data_structures/Graph.h"
#include "data_structures/LinkedList.h"
#include "simulation/FireSimulator.h"
#include "simulation/SimEntities.h"

namespace {

void printTestPassed(const std::string& test_name) {
  std::cout << test_name << " ........ PASSED\n";
}

void runAndReport(const std::string& test_name, void (*test_func)()) {
  test_func();
  printTestPassed(test_name);
}

// ===================== FIRE SPREAD =====================

void test_fire_initial_intensity_is_zero() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("A", Graph::NodeType::Room, 0, 1, 0.5);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("A", "EXIT", 2, 1, 0.5);

  assert(sim.fireIntensityAt("A") == 0.0);
  assert(sim.fireIntensityAt("EXIT") == 0.0);
  assert(!sim.isCorridorBlocked("A", "EXIT"));
}

void test_fire_ignite_sets_origin_intensity() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("ORIGIN", Graph::NodeType::Room, 0, 0, 0.5);
  const bool ignited = sim.igniteNode("ORIGIN", 1.0);

  assert(ignited);
  assert(sim.fireIntensityAt("ORIGIN") >= 0.9);
}

void test_fire_ignite_unknown_node_returns_false() {
  Graph g;
  FireSimulator sim(g);

  const bool ignited = sim.igniteNode("NOPE", 1.0);
  assert(!ignited);
}

void test_fire_spreads_to_neighbor_after_one_step() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("HOT", Graph::NodeType::Room, 0, 0, 0.5);
  sim.addNode("COLD", Graph::NodeType::Room, 0, 0, 0.5);
  sim.addCorridor("HOT", "COLD", 2, 5, 0.9);

  sim.igniteNode("HOT", 1.0);
  assert(sim.fireIntensityAt("COLD") == 0.0);

  sim.propagateFire();
  assert(sim.fireIntensityAt("COLD") > 0.0);
}

void test_fire_eventually_blocks_corridor() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("HOT", Graph::NodeType::Room, 0, 0, 1.0);
  sim.addNode("FAR", Graph::NodeType::Room, 0, 0, 0.5);
  sim.addCorridor("HOT", "FAR", 2, 1, 1.0);

  sim.igniteNode("HOT", 1.0);
  for (int i = 0; i < 20; ++i) {
    sim.propagateFire();
  }

  assert(sim.isCorridorBlocked("HOT", "FAR"));
  assert(sim.isCorridorBlocked("FAR", "HOT"));
}

void test_fire_block_is_propagated_into_graph() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("HOT", Graph::NodeType::Room, 0, 0, 1.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("HOT", "EXIT", 2, 1, 1.0);

  // No fire yet: Person 1's Graph should reach EXIT from HOT.
  assert(g.isReachableToAnyExit("HOT"));

  sim.igniteNode("HOT", 1.0);
  for (int i = 0; i < 20; ++i) {
    sim.propagateFire();
  }

  // After fire fully consumes the corridor, Graph must also see it blocked.
  assert(!g.isReachableToAnyExit("HOT"));
}

// ===================== DYNAMIC ROUTING =====================

void test_route_finds_shortest_path_no_fire() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("START", Graph::NodeType::Room, 0, 1, 0.1);
  sim.addNode("MID", Graph::NodeType::Hallway, 0, 0, 0.1);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  sim.addCorridor("START", "MID", 2, 5, 0.1);
  sim.addCorridor("MID", "EXIT", 3, 5, 0.1);

  FireSimulator::EvacPath path = sim.findSafestExit("START");
  assert(path.reachable);
  assert(path.total_cost == 5);
  assert(path.node_sequence.get_size() == 3);

  LinkedList<std::string> copy = path.node_sequence;
  assert(copy.pop_front() == "START");
  assert(copy.pop_front() == "MID");
  assert(copy.pop_front() == "EXIT");
}

void test_route_returns_unreachable_when_isolated() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("ISOLATED", Graph::NodeType::Room, 0, 1, 0.1);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  // No corridor between them.

  FireSimulator::EvacPath path = sim.findSafestExit("ISOLATED");
  assert(!path.reachable);
  assert(path.node_sequence.is_empty());
}

void test_route_unknown_start_returns_unreachable() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  FireSimulator::EvacPath path = sim.findSafestExit("DOES_NOT_EXIST");
  assert(!path.reachable);
}

void test_route_avoids_blocked_path_after_fire() {
  Graph g;
  FireSimulator sim(g);

  // Cheap path: START -> CHEAP -> EXIT_A  (cost 1+1 = 2)
  // Safe path : START -> SAFE  -> EXIT_B  (cost 3+3 = 6)
  sim.addNode("START", Graph::NodeType::Room, 0, 1, 0.05);
  sim.addNode("CHEAP", Graph::NodeType::Hallway, 0, 0, 1.0);
  sim.addNode("SAFE", Graph::NodeType::Hallway, 0, 0, 0.0);
  sim.addNode("EXIT_A", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addNode("EXIT_B", Graph::NodeType::Exit, 0, 0, 0.0);

  sim.addCorridor("START", "CHEAP", 1, 5, 1.0);
  sim.addCorridor("CHEAP", "EXIT_A", 1, 5, 1.0);
  sim.addCorridor("START", "SAFE", 3, 5, 0.0);
  sim.addCorridor("SAFE", "EXIT_B", 3, 5, 0.0);

  FireSimulator::EvacPath before = sim.findSafestExit("START");
  assert(before.reachable);
  assert(before.total_cost == 2);

  sim.igniteNode("CHEAP", 1.0);
  for (int i = 0; i < 10; ++i) {
    sim.propagateFire();
  }

  FireSimulator::EvacPath after = sim.findSafestExit("START");
  assert(after.reachable);
  assert(after.total_cost == 6);

  LinkedList<std::string> copy = after.node_sequence;
  assert(copy.pop_front() == "START");
  assert(copy.pop_front() == "SAFE");
  assert(copy.pop_front() == "EXIT_B");
}

void test_route_predictive_cost_inflates_near_fire() {
  Graph g;
  FireSimulator sim(g);

  // Tek yol senaryosu: START -> NEAR_FIRE -> EXIT.
  // NEAR_FIRE'ı sub-lethal (lethal eşik altı) ama yoğun bir yangınla tutuştur;
  // predictive cost devreye girip total_cost'u artırmalı.
  sim.addNode("START", Graph::NodeType::Room, 0, 1, 0.05);
  sim.addNode("NEAR_FIRE", Graph::NodeType::Hallway, 0, 0, 1.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  sim.addCorridor("START", "NEAR_FIRE", 2, 5, 1.0);
  sim.addCorridor("NEAR_FIRE", "EXIT", 2, 5, 1.0);

  FireSimulator::EvacPath before = sim.findSafestExit("START");
  assert(before.reachable);
  const long long cost_before = before.total_cost;

  sim.igniteNode("NEAR_FIRE", 0.9);

  FireSimulator::EvacPath after = sim.findSafestExit("START");
  assert(after.reachable);
  assert(after.total_cost > cost_before);
}

// ===================== BOTTLENECK (CAPACITY / QUEUE) =====================

void test_bottleneck_capacity_one_does_not_evacuate_in_one_step() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("ROOM", Graph::NodeType::Room, 0, 5, 0.0);
  sim.addNode("HALL", Graph::NodeType::Hallway, 0, 0, 0.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  sim.addCorridor("ROOM", "HALL", 2, 1, 0.0);   // dar koridor
  sim.addCorridor("HALL", "EXIT", 1, 5, 0.0);

  sim.step();
  assert(sim.evacuatedCount() == 0);
  assert(sim.casualtyCount() == 0);
}

void test_bottleneck_all_people_eventually_evacuate() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("ROOM", Graph::NodeType::Room, 0, 5, 0.0);
  sim.addNode("HALL", Graph::NodeType::Hallway, 0, 0, 0.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);

  sim.addCorridor("ROOM", "HALL", 2, 1, 0.0);
  sim.addCorridor("HALL", "EXIT", 1, 5, 0.0);

  sim.run(200);
  assert(sim.evacuatedCount() == 5);
  assert(sim.casualtyCount() == 0);
  assert(sim.currentTime() >= 5);
}

void test_bottleneck_unlimited_capacity_is_faster_than_capacity_one() {
  Graph g_slow;
  FireSimulator sim_slow(g_slow);
  sim_slow.addNode("R", Graph::NodeType::Room, 0, 5, 0.0);
  sim_slow.addNode("E", Graph::NodeType::Exit, 0, 0, 0.0);
  sim_slow.addCorridor("R", "E", 2, 1, 0.0);
  sim_slow.run(200);

  Graph g_fast;
  FireSimulator sim_fast(g_fast);
  sim_fast.addNode("R", Graph::NodeType::Room, 0, 5, 0.0);
  sim_fast.addNode("E", Graph::NodeType::Exit, 0, 0, 0.0);
  sim_fast.addCorridor("R", "E", 2, 0, 0.0);   // 0 = sınırsız
  sim_fast.run(200);

  assert(sim_slow.evacuatedCount() == 5);
  assert(sim_fast.evacuatedCount() == 5);
  assert(sim_fast.currentTime() < sim_slow.currentTime());
}

// ===================== CASUALTY =====================

void test_casualty_isolated_people_are_marked_dead_after_run() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("TRAPPED", Graph::NodeType::Room, 0, 2, 0.1);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  // İki düğüm birbirine bağlı değil: kimse çıkamaz.

  sim.run(20);
  assert(sim.evacuatedCount() == 0);
  assert(sim.casualtyCount() == 2);
}

void test_casualty_person_in_lethal_node_dies() {
  Graph g;
  FireSimulator sim(g);

  sim.addNode("DOOMED", Graph::NodeType::Room, 0, 1, 1.0);
  sim.addNode("EXIT", Graph::NodeType::Exit, 0, 0, 0.0);
  sim.addCorridor("DOOMED", "EXIT", 100, 1, 1.0);   // çıkış çok uzak

  sim.igniteNode("DOOMED", 1.0);
  sim.run(30);

  assert(sim.casualtyCount() == 1);
  assert(sim.evacuatedCount() == 0);
}

// ===================== MEMORY SCOPE =====================

void test_memory_scope_fire_simulator_create_destroy() {
  {
    Graph g;
    FireSimulator sim(g);
    sim.addNode("A", Graph::NodeType::Room, 0, 3, 0.5);
    sim.addNode("E", Graph::NodeType::Exit, 0, 0, 0.0);
    sim.addCorridor("A", "E", 2, 1, 0.5);
    sim.igniteNode("A", 1.0);
    sim.step();
  }
  {
    Graph g;
    FireSimulator sim(g);
    sim.addNode("X", Graph::NodeType::Room, 0, 2, 0.1);
    sim.run(5);
  }
}

}  // namespace

void run_fire_simulator_tests() {
  std::cout << "\n[UNIT] FireSimulator: Fire Spread\n";
  runAndReport("test_initial_intensity_is_zero", test_fire_initial_intensity_is_zero);
  runAndReport("test_ignite_sets_origin_intensity", test_fire_ignite_sets_origin_intensity);
  runAndReport("test_ignite_unknown_node_returns_false", test_fire_ignite_unknown_node_returns_false);
  runAndReport("test_spreads_to_neighbor_after_one_step", test_fire_spreads_to_neighbor_after_one_step);
  runAndReport("test_eventually_blocks_corridor", test_fire_eventually_blocks_corridor);
  runAndReport("test_block_is_propagated_into_graph", test_fire_block_is_propagated_into_graph);

  std::cout << "\n[UNIT] FireSimulator: Dynamic Routing\n";
  runAndReport("test_finds_shortest_path_no_fire", test_route_finds_shortest_path_no_fire);
  runAndReport("test_returns_unreachable_when_isolated", test_route_returns_unreachable_when_isolated);
  runAndReport("test_unknown_start_returns_unreachable", test_route_unknown_start_returns_unreachable);
  runAndReport("test_avoids_blocked_path_after_fire", test_route_avoids_blocked_path_after_fire);
  runAndReport("test_predictive_cost_inflates_near_fire", test_route_predictive_cost_inflates_near_fire);

  std::cout << "\n[UNIT] FireSimulator: Bottleneck\n";
  runAndReport("test_capacity_one_does_not_evacuate_in_one_step", test_bottleneck_capacity_one_does_not_evacuate_in_one_step);
  runAndReport("test_all_people_eventually_evacuate", test_bottleneck_all_people_eventually_evacuate);
  runAndReport("test_unlimited_capacity_is_faster", test_bottleneck_unlimited_capacity_is_faster_than_capacity_one);

  std::cout << "\n[UNIT] FireSimulator: Casualty\n";
  runAndReport("test_isolated_people_marked_dead", test_casualty_isolated_people_are_marked_dead_after_run);
  runAndReport("test_person_in_lethal_node_dies", test_casualty_person_in_lethal_node_dies);

  std::cout << "\n[UNIT] FireSimulator: MemoryScope\n";
  runAndReport("test_fire_simulator_create_destroy", test_memory_scope_fire_simulator_create_destroy);
}
