#pragma once

#include <string>
#include "../data_structures/Queue.h"
#include "../data_structures/Graph.h"

// ---------------------------------------------------------------------------
// Simulation-side mirrored topology + entities.
// Person 1's Graph is the public source of truth (used by GUI/exporters);
// these structs hold the EXTRA per-tick simulation state that Graph does not
// expose: fire intensity, flammability, capacity occupancy, etc.
// All structures are pointer/intrusive-list based: no STL containers.
// ---------------------------------------------------------------------------

struct CellNode {
    std::string id;
    Graph::NodeType type;
    int floor;
    bool is_exit;
    bool on_fire;
    double fire_intensity;   // 0.0 .. 1.0
    double flammability;     // 0.0 .. 1.0  (how easily this cell ignites)
    int people_in_node;      // current occupants (live, not casualties)
    CellNode* next;          // intrusive global node list

    explicit CellNode(const std::string& node_id)
        : id(node_id),
          type(Graph::NodeType::Unknown),
          floor(0),
          is_exit(false),
          on_fire(false),
          fire_intensity(0.0),
          flammability(0.0),
          people_in_node(0),
          next(nullptr) {}
};

struct Corridor {
    CellNode* from;
    CellNode* to;
    int traversal_time;       // ticks to walk across
    int capacity;             // max simultaneous walkers (0 = unlimited)
    int current_load;         // people currently traversing
    double flammability;      // 0.0 .. 1.0
    double fire_intensity;    // 0.0 .. 1.0
    bool blocked;             // true once fire intensity >= block threshold
    Corridor* next_global;    // intrusive global corridor list

    Corridor(CellNode* a, CellNode* b, int t, int c, double f)
        : from(a),
          to(b),
          traversal_time(t),
          capacity(c),
          current_load(0),
          flammability(f),
          fire_intensity(0.0),
          blocked(false),
          next_global(nullptr) {}
};

struct Person {
    int id;
    std::string current_location;   // node id the person is at / just left
    std::string next_location;      // node id heading toward (when on_edge)
    int progress_on_edge;
    int edge_traversal_time;
    bool on_edge;
    bool in_queue;
    bool evacuated;
    bool dead;
    Person* next;                   // intrusive global person list

    Person(int person_id, const std::string& start)
        : id(person_id),
          current_location(start),
          next_location(""),
          progress_on_edge(0),
          edge_traversal_time(0),
          on_edge(false),
          in_queue(false),
          evacuated(false),
          dead(false),
          next(nullptr) {}
};

// Per-corridor FIFO of people waiting because the corridor is at capacity.
struct WaitingQueue {
    Corridor* corridor;
    Queue<Person*> queue;
    WaitingQueue* next;

    explicit WaitingQueue(Corridor* c) : corridor(c), queue(), next(nullptr) {}
};
