# =============================================================================
# Fire Evacuation Route Planner & Simulator
# Yeditepe University - CSE 211 Data Structures Term Project (PROJ-02)
# =============================================================================
#
# Required targets (per project specification):
#   make          - build the CLI demo + test suite
#   make run      - build and run the CLI demo
#   make test     - build and run all tests
#   make gui      - build the SDL2 graphical frontend
#   make run-gui  - build and run the GUI
#   make deps     - install external dependencies (SDL2, doxygen)
#   make docs     - generate Doxygen documentation
#   make memcheck - run the test suite under valgrind
#   make clean    - remove all build artifacts
# =============================================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS   :=

# ---- SDL2 / SDL2_ttf (GUI target only) --------------------------------------
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

# ---- Directories ------------------------------------------------------------
BUILD_DIR := build

# ---- Sources ----------------------------------------------------------------
DEMO_SRC  := src/sim_demo.cpp
GUI_SRC   := src/main_gui.cpp

TEST_SRCS := tests/test_main.cpp \
             tests/unit/test_data_structures.cpp \
             tests/unit/test_fire_simulator.cpp \
             tests/integration/test_graph_workflow.cpp \
             tests/integration/test_scene_io.cpp

# ---- Output binaries --------------------------------------------------------
DEMO_BIN  := $(BUILD_DIR)/sim_demo
GUI_BIN   := $(BUILD_DIR)/sim_gui
TEST_BIN  := $(BUILD_DIR)/sim_tests

# ---- Header list (for incremental rebuilds) ---------------------------------
HEADERS := \
    include/data_structures/Node.h           \
    include/data_structures/LinkedList.h     \
    include/data_structures/Queue.h          \
    include/data_structures/priority_queue.h \
    include/data_structures/Graph.h          \
    include/simulation/SimEntities.h         \
    include/simulation/FireSimulator.h       \
    include/io/SceneData.h                   \
    include/io/JsonSceneParser.h             \
    include/io/SceneLoader.h                 \
    include/gui/Layout.h                     \
    include/gui/Renderer.h                   \
    include/gui/SimulationController.h

.PHONY: all demo gui test run run-gui deps docs memcheck clean help

# Default: CLI demo + tests. The GUI is built separately with `make gui`
# because SDL2 may not be present on a clean machine.
all: demo test

help:
	@echo "Targets:"
	@echo "  make          - build CLI demo + tests (default)"
	@echo "  make demo     - build the CLI simulation demo  ($(DEMO_BIN))"
	@echo "  make gui      - build the SDL2 graphical frontend ($(GUI_BIN))"
	@echo "  make test     - build and run unit + integration tests"
	@echo "  make run      - build and run the CLI demo"
	@echo "  make run-gui  - build and run the GUI"
	@echo "  make deps     - install external dependencies (SDL2, doxygen)"
	@echo "  make docs     - generate Doxygen documentation into docs/html"
	@echo "  make memcheck - run the test suite under valgrind"
	@echo "  make clean    - remove the build/ directory"

# =============================================================================
# DEMO  (command-line frontend)
# =============================================================================
demo: $(DEMO_BIN)

$(DEMO_BIN): $(DEMO_SRC) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEMO_SRC) -o $@

run: $(DEMO_BIN)
	./$(DEMO_BIN)

# =============================================================================
# GUI  (SDL2 + SDL2_ttf frontend)
# =============================================================================
gui: $(GUI_BIN)

$(GUI_BIN): $(GUI_SRC) $(HEADERS) | $(BUILD_DIR)
	@if [ -z "$(SDL_CFLAGS)" ]; then \
	    echo ">> SDL2 / SDL2_ttf not found. Install with 'make deps' or:"; \
	    echo ">>   Ubuntu: sudo apt install libsdl2-dev libsdl2-ttf-dev pkg-config"; \
	    echo ">>   macOS : brew install sdl2 sdl2_ttf pkg-config"; \
	    exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(GUI_SRC) $(SDL_LIBS) -o $@

run-gui: $(GUI_BIN)
	./$(GUI_BIN)

# =============================================================================
# TESTS
# =============================================================================
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $@

# =============================================================================
# DEPENDENCIES
# =============================================================================
# Installs the optional SDL2 GUI stack and Doxygen. The core demo and tests
# build with no external dependencies, so this target is only needed for the
# graphical frontend and documentation generation.
deps:
	@echo "Installing dependencies..."
	@if command -v apt-get >/dev/null 2>&1; then \
	    sudo apt-get update && \
	    sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev pkg-config \
	                            doxygen graphviz valgrind; \
	elif command -v brew >/dev/null 2>&1; then \
	    brew install sdl2 sdl2_ttf pkg-config doxygen graphviz; \
	else \
	    echo ">> No supported package manager found (apt-get / brew)."; \
	    echo ">> Please install SDL2, SDL2_ttf, doxygen and valgrind manually."; \
	fi

# =============================================================================
# DOCUMENTATION
# =============================================================================
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
	    doxygen Doxyfile && \
	    echo ">> Documentation generated in docs/html/index.html"; \
	else \
	    echo ">> doxygen not found. Run 'make deps' first."; \
	fi

# =============================================================================
# MEMORY CHECK
# =============================================================================
memcheck: $(TEST_BIN)
	@if command -v valgrind >/dev/null 2>&1; then \
	    valgrind --leak-check=full --show-leak-kinds=all \
	             --error-exitcode=1 ./$(TEST_BIN); \
	else \
	    echo ">> valgrind not found. Run 'make deps' first."; \
	fi

# =============================================================================
# HOUSEKEEPING
# =============================================================================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) docs/html
