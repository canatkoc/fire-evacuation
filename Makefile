# =============================================================================
# Fire Evacuation Route Planner & Simulator
# Yeditepe Üniversitesi - CSE 211
# =============================================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS   :=

# ---- SDL2 / SDL2_ttf (yalniz GUI hedefi icin) ----
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs   sdl2 SDL2_ttf 2>/dev/null)

# ---- Dizin yapisi ----
BUILD_DIR := build

# ---- Kaynaklar ----
DEMO_SRC      := src/sim_demo.cpp
GUI_SRC       := src/main_gui.cpp

TEST_MAIN     := tests/test_main.cpp
TEST_UNIT_DS  := tests/unit/test_data_structures.cpp
TEST_UNIT_SIM := tests/unit/test_fire_simulator.cpp
TEST_INTEG    := tests/integration/test_graph_workflow.cpp

# ---- Hedefler ----
DEMO_BIN  := $(BUILD_DIR)/sim_demo
GUI_BIN   := $(BUILD_DIR)/sim_gui
TEST_BIN  := $(BUILD_DIR)/sim_tests

# ---- Header listesi (incremental rebuild icin) ----
HEADERS := \
    include/data_structures/Node.h          \
    include/data_structures/LinkedList.h    \
    include/data_structures/Queue.h         \
    include/data_structures/priority_queue.h \
    include/data_structures/Graph.h         \
    include/simulation/SimEntities.h        \
    include/simulation/FireSimulator.h      \
    include/gui/Layout.h                    \
    include/gui/Renderer.h                  \
    include/gui/SimulationController.h

.PHONY: all demo gui test run run-gui clean help

# Varsayilan: demo + testler. GUI ayrica `make gui` ile derlenir
# (pkg-config / SDL2 sistemde kurulu olmayabilir).
all: demo test

help:
	@echo "Hedefler:"
	@echo "  make demo     - Komut satiri simulasyon demosu  ($(DEMO_BIN))"
	@echo "  make gui      - SDL2 tabanli gorsel arayuz       ($(GUI_BIN))"
	@echo "  make test     - Unit + integration testler        ($(TEST_BIN))"
	@echo "  make run      - demo'yu calistir"
	@echo "  make run-gui  - gui'yi calistir"
	@echo "  make clean    - build/ klasorunu temizle"

# =============================================================================
# DEMO  (CLI)
# =============================================================================
demo: $(DEMO_BIN)

$(DEMO_BIN): $(DEMO_SRC) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEMO_SRC) -o $@

run: $(DEMO_BIN)
	./$(DEMO_BIN)

# =============================================================================
# GUI  (SDL2 + SDL2_ttf)
# =============================================================================
gui: $(GUI_BIN)

$(GUI_BIN): $(GUI_SRC) $(HEADERS) | $(BUILD_DIR)
	@if [ -z "$(SDL_CFLAGS)" ]; then \
	    echo ">> SDL2 / SDL2_ttf bulunamadi. Kurulum:"; \
	    echo ">>   macOS:  brew install sdl2 sdl2_ttf"; \
	    echo ">>   Ubuntu: sudo apt install libsdl2-dev libsdl2-ttf-dev pkg-config"; \
	    exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(GUI_SRC) $(SDL_LIBS) -o $@

run-gui: $(GUI_BIN)
	./$(GUI_BIN)

# =============================================================================
# TESTS  (data structures + fire simulator + integration)
# =============================================================================
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_MAIN) $(TEST_UNIT_DS) $(TEST_UNIT_SIM) $(TEST_INTEG) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) \
	    $(TEST_MAIN) $(TEST_UNIT_DS) $(TEST_UNIT_SIM) $(TEST_INTEG) \
	    -o $@

# =============================================================================
# Klasor
# =============================================================================
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
