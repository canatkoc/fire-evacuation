/**
 * @file test_main.cpp
 * @brief Entry point for the project's unit + integration test suite.
 *
 * Each test module exposes a single run_*_tests() function; this runner
 * invokes them all and reports an aggregate result. A non-zero exit code is
 * returned if any assertion fails (assertions abort) or an exception escapes.
 */

#include <exception>
#include <iostream>

void run_data_structures_unit_tests();
void run_fire_simulator_tests();
void run_graph_integration_tests();
void run_scene_io_tests();
void run_performance_tests();
void run_edge_case_tests();

int main() {
    try {
        run_data_structures_unit_tests();
        run_fire_simulator_tests();
        run_graph_integration_tests();
        run_scene_io_tests();
        run_performance_tests();
        run_edge_case_tests();
        std::cout << "\nSUMMARY: all tests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\nTEST RUN FAILED: " << ex.what() << "\n";
        return 1;
    }
}
