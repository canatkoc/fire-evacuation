# Design Document — Fire Evacuation Route Planner & Simulator

CSE 211 Data Structures Term Project — PROJ-02
Yeditepe University, Spring 2026

---

## 1. Architecture Overview

The system simulates the evacuation of a building during a fire. The building
is modelled as a graph: nodes are rooms, hallways, stairwells and exits, and
edges are corridors with a traversal time and a capacity limit. Fire spreads
over discrete time steps, blocking corridors and forcing occupants to replan
their routes toward the safest reachable exit.

The codebase is organised into four layers, each in its own directory under
`include/`:

1. **`data_structures/`** — the custom, pointer-based containers that satisfy
   the course's "no STL containers" constraint: a singly linked list, a queue
   built on it, a pointer-based binary-heap priority queue, and a graph.
2. **`io/`** — the input pipeline: a dependency-free JSON parser, a plain data
   model for a parsed scene, and a loader that turns a parsed scene into live
   simulator objects.
3. **`simulation/`** — the simulation engine (`FireSimulator`) and its
   per-tick entity structs (`CellNode`, `Corridor`, `Person`, `WaitingQueue`).
4. **`gui/`** — the SDL2 graphical frontend: a renderer, an input-agnostic
   `SimulationController`, and a `Layout` that maps node ids to screen
   coordinates.

Two executables are produced. `sim_demo` is the command-line frontend; it
loads a scene and prints the evacuation timeline. `sim_gui` is the SDL2
graphical frontend with Start/Pause/Step/Reset controls. Both share the same
engine and the same `io/` loading path, so the scene-building logic is written
exactly once.

The data flow is strictly one-directional during setup:

```
data/*.json --> JsonSceneParser --> SceneData --> SceneLoader --> FireSimulator
                                                            \--> Layout (GUI)
```

## 2. Module Descriptions

| Module | Responsibility |
|--------|----------------|
| `Node` / `LinkedList` | Generic singly linked list; the project's base linear container. |
| `Queue` | FIFO queue, composed on top of `LinkedList`; used for corridor waiting lines. |
| `PriorityQueue` | Pointer-based binary min-heap; the frontier structure for Dijkstra. |
| `Graph` | Adjacency-list graph; the public source of truth for building topology and shortest paths. |
| `SceneData` / `SceneNode` / `SceneEdge` | Plain data model holding one parsed input file. |
| `JsonSceneParser` | Hand-written recursive JSON scanner; fills a `SceneData`. |
| `SceneLoader` | Translates a `SceneData` into a populated `FireSimulator` and `Layout`. |
| `FireSimulator` | The simulation engine: fire spread, routing, movement, casualties. |
| `SimulationController` | Drives the engine at a fixed tick interval; handles GUI signals. |
| `Renderer` / `Layout` | SDL2 drawing and node-to-pixel position mapping. |

## 3. Data Structure Choices

The project implements **four** custom data structures from **three different
categories**, all pointer-based, satisfying the requirement of at least two
structures from distinct categories.

- **Linked List (Linear).** Chosen as the project's universal sequence type.
  It backs the queue, stores evacuation paths (`EvacPath::node_sequence`), and
  is used wherever an ordered, growable collection is needed without STL.
  Insertion at either end and removal by value are all O(1)/O(n) with no
  reallocation, which matters because the simulator builds many short-lived
  path lists every tick.

- **Queue (Linear).** Built by composition on the linked list. Each corridor
  that is over capacity owns a `WaitingQueue` whose FIFO ordering models
  realistic queuing at evacuation bottlenecks — the first person to arrive at
  a congested stairwell is the first to be let through.

- **Priority Queue (Tree / heap-based).** A pointer-based binary min-heap with
  explicit `parent`/`left`/`right` links and a secondary insertion queue to
  locate the next insertion slot in O(1). It is the frontier structure for the
  Dijkstra search inside `findSafestExit`, giving O(log n) push/pop.

- **Graph (Graph category).** An adjacency list of `Vertex` nodes, each owning
  an intrusive linked list of `Edge` records. It is the natural model for a
  building floor plan and supports the shortest-path and nearest-exit queries
  directly.

This selection spans the Linear, Tree and Graph categories required by the
specification, and every structure uses raw-pointer node linkage — no
array-based or index-based implementations.

## 4. Algorithm Explanations

**Fire spread (`propagateFire`).** Each tick, fire intensity flows from a
burning node to its neighbours at a rate proportional to the global
`fire_spread_rate` and the neighbour's `flammability`. When a corridor's
accumulated intensity crosses the block threshold it becomes impassable and is
removed from routing. Complexity: O(V + E) per tick.

**Predictive safest-exit routing (`findSafestExit`).** A Dijkstra search from
the occupant's node to the nearest exit, using the custom priority queue as
the frontier. Edge cost is not merely traversal time: a *predictive penalty*
is added for corridors expected to be on fire by the time the occupant would
reach them. This avoids routes that are momentarily clear but will be blocked
en route. Complexity: O((V + E) log V) per query.

**Time-step movement (`stepPeople`).** People advance along corridors one tick
at a time. When a corridor is at capacity, additional people are placed in its
`WaitingQueue` and released in FIFO order as space frees up. People reaching an
exit are counted as evacuated.

**Casualty detection (`processCasualties`).** Occupants whose node reaches a
lethal fire intensity, or who are fully encircled by fire with no reachable
exit, are marked as casualties. The simulation ends when every person is
either evacuated or a casualty.

## 5. Design Decisions

- **Custom JSON parser instead of a library.** The `data/` input is JSON, but
  pulling in `nlohmann/json` would mean either an STL-heavy dependency or a
  network download in `make deps`. A small hand-written scanner keeps the
  project self-contained, builds with zero external dependencies, and is
  itself a demonstration of pointer-based parsing into intrusive lists.

- **Scene loading isolated in `SceneLoader`.** Both frontends need to build the
  same world. Putting that logic in one loader (rather than duplicating it in
  `sim_demo.cpp` and `main_gui.cpp`) follows the Single Responsibility
  Principle and means a change to the input format touches one file.

- **`Graph` and the simulator's `CellNode` list coexist.** `Graph` is the
  public topology used for shortest-path queries and would be used by any
  exporter or GUI; the simulator additionally keeps `CellNode`/`Corridor`
  structs holding per-tick state (fire intensity, capacity load) that the
  generic graph deliberately does not model. This separation keeps the graph
  reusable and the simulation state cohesive.

- **Optional `x`/`y` coordinates in the input.** Input files may specify
  screen positions for the GUI. When absent, `SceneLoader::applyLayout`
  arranges nodes on an automatic grid, so any valid scene file renders without
  hand-placed coordinates.

- **GUI kept out of the default build.** `make` builds the CLI demo and the
  test suite, which have no external dependencies and therefore always
  succeed on a clean machine. The SDL2 GUI is built explicitly with
  `make gui` after `make deps`, so a missing SDL2 install can never cause the
  graded `make` / `make test` to fail.

---

See `docs/uml/` for the class diagram and the simulation-tick behavioural
diagram (PlantUML sources and rendered PNGs).
