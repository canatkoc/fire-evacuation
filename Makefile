CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

TEST_DIR = tests
UNIT_TEST = $(TEST_DIR)/unit/test_data_structures.cpp
INTEGRATION_TEST = $(TEST_DIR)/integration/test_graph_workflow.cpp
TEST_MAIN = $(TEST_DIR)/test_main.cpp

TEST_BIN = build/tests.exe

.PHONY: all run test clean

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_MAIN) $(UNIT_TEST) $(INTEGRATION_TEST)
	@if not exist build mkdir build
	$(CXX) $(CXXFLAGS) $(TEST_MAIN) $(UNIT_TEST) $(INTEGRATION_TEST) -o $(TEST_BIN)

run: $(TEST_BIN)
	./$(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	@if exist build rmdir /s /q build
