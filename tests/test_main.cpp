#include <exception>
#include <iostream>

void run_data_structures_unit_tests();
void run_graph_integration_tests();

int main() {
  try {
    run_data_structures_unit_tests();
    run_graph_integration_tests();
    std::cout << "\nSUMMARY: all tests passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "\nTEST RUN FAILED: " << ex.what() << "\n";
    return 1;
  }
}
