/**
 * @file SimEntities.h
 * @brief Per-tick simulation entities: CellNode, Corridor, Person, WaitingQueue.
 *
 * These structs mirror the topology stored in the Graph, enriching it with
 * extra per-tick simulation state (fire intensity, flammability, capacity load,
 * person progress) that Graph does not expose.
 *
 * All structures use intrusive pointer links rather than STL containers.
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once

#include <string>
#include "../data_structures/Queue.h"
#include "../data_structures/Graph.h"

/**
 * @brief A simulation-side node mirroring a Graph vertex.
 *
 * Stores fire state (on_fire, fire_intensity, flammability), current occupant
 * count, and a @c next pointer for the global singly-linked node list owned
 * by FireSimulator.
 */
struct CellNode {
    std::string     id;             ///< Unique node identifier (matches Graph vertex id).
    Graph::NodeType type;           ///< Semantic category of this node.
    int             floor;          ///< Floor number this node resides on.
    bool            is_exit;        ///< true if this node is an evacuation exit.
    bool            on_fire;        ///< true once fire_intensity >= 0.5.
    double          fire_intensity; ///< Fire intensity in [0,1]; 0=no fire, 1=fully ablaze.
    double          flammability;   ///< Rate at which this node catches fire in [0,1].
    int             people_in_node; ///< Number of live occupants currently inside this node.
    CellNode*       next;           ///< Next node in the global node list; nullptr if last.

    /**
     * @brief Constructs a CellNode with the given identifier and safe defaults.
     * @param node_id Unique string identifier matching the Graph vertex.
     * @post on_fire == false, fire_intensity == 0.0, people_in_node == 0,
     *       type == NodeType::Unknown, is_exit == false, next == nullptr.
     */
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

/**
 * @brief A directed corridor connecting two CellNodes.
 *
 * Carries both topology (from, to, traversal_time, capacity) and fire/load
 * state (fire_intensity, flammability, current_load, blocked).
 */
struct Corridor {
    CellNode* from;           ///< Source node of this directed corridor.
    CellNode* to;             ///< Destination node of this directed corridor.
    int       traversal_time; ///< Number of simulation ticks to cross this corridor.
    int       capacity;       ///< Maximum simultaneous walkers (0 = unlimited).
    int       current_load;   ///< Number of people currently traversing this corridor.
    double    flammability;   ///< How quickly this corridor ignites in [0,1].
    double    fire_intensity; ///< Current fire intensity in [0,1].
    bool      blocked;        ///< true once fire_intensity >= fire_block_threshold.
    Corridor* next_global;    ///< Next corridor in the global list; nullptr if last.

    /**
     * @brief Constructs a Corridor between two CellNodes.
     * @param a Source CellNode.
     * @param b Destination CellNode.
     * @param t Traversal time in ticks (>= 1).
     * @param c Capacity (0 = unlimited).
     * @param f Flammability in [0,1].
     * @post current_load == 0, fire_intensity == 0.0, blocked == false,
     *       next_global == nullptr.
     */
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

/**
 * @brief A single person being evacuated through the building.
 *
 * Tracks position (current_location, next_location), progress along a
 * corridor, and status flags (on_edge, in_queue, evacuated, dead).
 */
struct Person {
    int         id;                  ///< Unique non-negative integer identifier.
    std::string current_location;    ///< ID of the node the person is at or just left.
    std::string next_location;       ///< ID of the node being moved toward; empty if at rest.
    int         progress_on_edge;    ///< Ticks already spent traversing the current corridor.
    int         edge_traversal_time; ///< Total ticks required for the current corridor.
    bool        on_edge;             ///< true while traversing a corridor.
    bool        in_queue;            ///< true while waiting in a WaitingQueue.
    bool        evacuated;           ///< true once the person has reached an exit node.
    bool        dead;                ///< true once the person has been consumed by fire.
    Person*     next;                ///< Next person in the global person list.

    /**
     * @brief Constructs a Person at the given starting node.
     * @param person_id Unique identifier assigned by FireSimulator.
     * @param start     ID of the node where the person begins.
     * @post on_edge == false, in_queue == false, evacuated == false, dead == false,
     *       progress_on_edge == 0, next == nullptr.
     */
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

/**
 * @brief A per-corridor FIFO queue of people blocked by a capacity bottleneck.
 *
 * Created on demand by FireSimulator::getOrCreateQueue() the first time a
 * corridor reaches capacity.
 */
struct WaitingQueue {
    Corridor*      corridor; ///< The corridor this queue is associated with.
    Queue<Person*> queue;    ///< FIFO of persons waiting to enter the corridor.
    WaitingQueue*  next;     ///< Next WaitingQueue in the list; nullptr if last.

    /**
     * @brief Constructs an empty WaitingQueue for the given corridor.
     * @param c The corridor at capacity that this queue buffers.
     * @post queue.is_empty() == true, next == nullptr.
     */
    explicit WaitingQueue(Corridor* c) : corridor(c), queue(), next(nullptr) {}
};
