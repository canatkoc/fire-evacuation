#pragma once

/**
 * @file SceneData.h
 * @brief Plain data structures describing a building evacuation scene.
 *
 * These structs are the in-memory result of parsing a @c data/ input file.
 * They use only intrusive singly linked node lists (no STL containers) so the
 * project's "no STL containers as primary storage" constraint is respected.
 */

#include <string>

/**
 * @brief One building node (room, hallway, stairwell or exit).
 *
 * Stored as an intrusive linked list element through @c next.
 */
struct SceneNode {
    std::string id;          ///< Unique node identifier.
    std::string type;        ///< "room", "hallway", "stairwell" or "exit".
    int floor;               ///< Floor index the node belongs to.
    int occupants;           ///< Number of people initially in the node.
    double flammability;     ///< Flammability rating in [0, 1].
    int x;                   ///< Optional GUI x coordinate (-1 if absent).
    int y;                   ///< Optional GUI y coordinate (-1 if absent).
    SceneNode* next;         ///< Next node in the parsed list (intrusive).

    SceneNode()
        : id(), type("room"), floor(0), occupants(0),
          flammability(0.3), x(-1), y(-1), next(nullptr) {}

    /// @return true if explicit screen coordinates were provided in the file.
    bool hasCoordinates() const { return x >= 0 && y >= 0; }
};

/**
 * @brief One corridor (graph edge) connecting two nodes.
 *
 * Stored as an intrusive linked list element through @c next.
 */
struct SceneEdge {
    std::string from;        ///< Source node id.
    std::string to;          ///< Destination node id.
    int capacity;            ///< People per time step the corridor allows.
    int traversal_time;      ///< Time steps required to traverse.
    double flammability;     ///< Corridor flammability rating in [0, 1].
    SceneEdge* next;         ///< Next edge in the parsed list (intrusive).

    SceneEdge()
        : from(), to(), capacity(1), traversal_time(1),
          flammability(0.3), next(nullptr) {}
};

/**
 * @brief Full parsed scene: a building, its nodes and its corridors.
 *
 * Owns the two intrusive linked lists and frees them on destruction.
 */
struct SceneData {
    int floors;                  ///< Number of floors in the building.
    std::string fire_origin;     ///< Node id where the fire starts.
    double fire_spread_rate;     ///< Global fire spread rate.
    SceneNode* nodes;            ///< Head of the parsed node list.
    SceneEdge* edges;            ///< Head of the parsed edge list.

    SceneData()
        : floors(1), fire_origin(), fire_spread_rate(0.5),
          nodes(nullptr), edges(nullptr) {}

    /// Non-copyable: ownership of raw lists must stay unique.
    SceneData(const SceneData&) = delete;
    SceneData& operator=(const SceneData&) = delete;

    /// Releases both intrusive linked lists.
    ~SceneData() {
        while (nodes != nullptr) {
            SceneNode* tmp = nodes;
            nodes = nodes->next;
            delete tmp;
        }
        while (edges != nullptr) {
            SceneEdge* tmp = edges;
            edges = edges->next;
            delete tmp;
        }
    }

    /**
     * @brief Appends a node to the end of the node list.
     * @param n Heap-allocated node; ownership is transferred to SceneData.
     */
    void appendNode(SceneNode* n) {
        n->next = nullptr;
        if (nodes == nullptr) {
            nodes = n;
            return;
        }
        SceneNode* cur = nodes;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = n;
    }

    /**
     * @brief Appends a corridor to the end of the edge list.
     * @param e Heap-allocated edge; ownership is transferred to SceneData.
     */
    void appendEdge(SceneEdge* e) {
        e->next = nullptr;
        if (edges == nullptr) {
            edges = e;
            return;
        }
        SceneEdge* cur = edges;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = e;
    }
};
