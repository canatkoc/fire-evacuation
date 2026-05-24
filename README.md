# Fire Evacuation Route Planner & Simulator

CSE 211 — Data Structures Term Project — **PROJ-02**
Yeditepe University, Spring 2026

A C++17 application that models a building as a graph and simulates the
evacuation of its occupants during a fire. Fire spreads over discrete time
steps, blocking corridors and forcing occupants to replan their routes toward
the nearest safe exit while respecting corridor capacity limits. The project
ships with both a command-line frontend and an SDL2 graphical frontend.

---

## 1. Project Description

The building is a graph: nodes are rooms, hallways, stairwells and exits;
edges are corridors with a traversal time and a per-tick capacity. Each tick
the fire spreads, occupants move, congested corridors queue people, and routes
are recomputed with a fire-aware predictive cost. Occupants who cannot reach
any exit are reported as casualties.

All core data structures are implemented from scratch with pointer-based node
linkage — no STL containers are used as primary storage.

## 2. Group Information

| Member | Role |
|--------|------|
|Özgür Sak| Custom data structures (LinkedList, Queue, PriorityQueue, Graph) |
|Ömer Kağan Zafer| I/O pipeline & integration (JSON parser, SceneLoader, Makefile) |
|Canat Koç| Simulation engine (FireSimulator: fire spread, routing, casualties) |
|Eren Kocagökcen| Frontend (SDL2 Renderer, SimulationController, Layout) |


## 3. Build Instructions

The project requires a C++17 compiler (`g++` or `clang++`) and `make`.
The CLI demo and the test suite have **no external dependencies**.

```sh
make deps     # optional: installs SDL2 + doxygen + valgrind (for GUI/docs)
make          # builds the CLI demo and the test suite
make run      # builds and runs the CLI demo
make test     # builds and runs all unit + integration tests
make gui      # builds the SDL2 graphical frontend (needs `make deps` first)
make run-gui  # builds and runs the GUI
make docs     # generates Doxygen HTML documentation into docs/html/
make memcheck # runs the test suite under valgrind
make clean    # removes all build artifacts
```

`make` and `make test` succeed on a clean machine with no extra packages.
The SDL2 GUI is intentionally a separate target so a missing SDL2 install can
never break the graded build.

## 4. Usage Examples

```sh
# Run the default building scenario
./build/sim_demo

# Run a specific scenario from the data/ directory
./build/sim_demo data/sample_proj02.json
./build/sim_demo data/edge_case_trapped.json

# Launch the graphical simulator on a chosen scene
./build/sim_gui data/input_default.json
```

GUI keyboard shortcuts: `SPACE` play/pause, `S` single step, `R` reset,
`+`/`-` change speed, `ESC` quit.

The CLI demo prints the initial evacuation plans, a per-tick population
snapshot (`alive` / `evac` / `dead`), and a final summary.

## 5. Project Structure

```
include/
  data_structures/   Node, LinkedList, Queue, PriorityQueue, Graph
  io/                SceneData, JsonSceneParser, SceneLoader
  simulation/        SimEntities, FireSimulator
  gui/               Layout, Renderer, SimulationController
src/
  sim_demo.cpp       CLI frontend entry point
  main_gui.cpp       SDL2 GUI frontend entry point
tests/
  test_main.cpp      test runner
  unit/              data structure + fire simulator unit tests
  integration/       graph workflow + scene I/O tests
data/                JSON input scenes (default, sample, edge cases, stress)
docs/
  uml/               class diagram + tick-flow diagram (PlantUML + PNG)
  design_document.md architecture and design rationale
Doxyfile             Doxygen configuration
Makefile             build system
.clang-format        code formatting configuration
```

## 6. Data Structures Used

| Structure | Category | Why it was chosen |
|-----------|----------|-------------------|
| **LinkedList** | Linear | Universal growable sequence; backs the queue and stores evacuation paths without reallocation. |
| **Queue** | Linear | FIFO ordering models realistic queuing at over-capacity corridors and stairwells. |
| **PriorityQueue** | Tree (binary heap) | O(log n) frontier for the Dijkstra safest-exit search. |
| **Graph** | Graph | Natural model of a building floor plan; supports shortest-path and nearest-exit queries. |

The four structures span three distinct categories (Linear, Tree, Graph) and
are all pointer-based — see `docs/design_document.md` for full rationale.

## 7. Input Format

Scenes are JSON files in `data/`. Example:

```json
{
  "building": { "floors": 2, "fire_origin": "ROOM_C" },
  "nodes": [
    {"id": "ROOM_A", "type": "room", "floor": 1, "occupants": 4,
     "flammability": 0.4, "x": 170, "y": 260}
  ],
  "edges": [
    {"from": "ROOM_A", "to": "HALL_1", "capacity": 2,
     "traversal_time": 2, "flammability": 0.3}
  ],
  "fire_spread_rate": 0.18
}
```

`type` is one of `room`, `hallway`, `stairwell`, `exit`. The `x`/`y` fields
are optional; when omitted the GUI lays nodes out on an automatic grid.

## 8. Known Limitations

- The JSON parser supports the subset used by the project's scene files; it
  does not handle comments or `\u` unicode escapes.
- The GUI requires SDL2 and SDL2_ttf; without them only the CLI demo and the
  test suite are available.
- Text labels in the GUI require a TrueType font; if the configured font path
  does not exist, the simulation still renders but node labels are skipped.
- The simulator uses a fixed tick budget as a safety guard against degenerate
  inputs that never terminate.

## 9. Testing

```sh
make test       # runs all unit + integration tests
make memcheck   # same suite under valgrind --leak-check=full
```

The suite covers the custom data structures (construction, insertion,
deletion, search, edge cases), the fire simulator (spread, routing,
casualties), the graph workflow, and the JSON input pipeline (parsing, edge
cases, malformed input, end-to-end loading).
