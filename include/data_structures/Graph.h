/**
 * @file Graph.h
 * @brief Adjacency-list directed/undirected graph with Dijkstra shortest-path.
 *
 * Implements a weighted graph using a singly-linked vertex list and per-vertex
 * singly-linked edge lists -- no STL containers are used for primary storage.
 * Dijkstra's algorithm is implemented with the custom PriorityQueue to compute
 * shortest paths; the result is returned as a PathResult containing the
 * reconstructed node sequence and total traversal cost.
 *
 * Used by FireSimulator to model the building layout and to find the safest
 * evacuation route from any room to the nearest unblocked exit.
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once

#include <limits>
#include <stdexcept>
#include <string>

#include "LinkedList.h"
#include "priority_queue.h"

/**
 * @brief Weighted graph with pointer-based adjacency lists and Dijkstra routing.
 *
 * Vertices are stored in a singly-linked list (vertices_head_ -> vertices_tail_).
 * Each vertex owns a singly-linked list of outgoing Edge objects.
 * addEdge() can create bidirectional (undirected) edges automatically.
 *
 * Copy construction and copy assignment are disabled; move semantics are
 * provided so the graph can be transferred without copying the entire
 * vertex/edge structure.
 */
class Graph {
 public:
  // --------------------------------------------------------------------------
  // Public types
  // --------------------------------------------------------------------------

  /**
   * @brief Semantic category of a graph node in the building model.
   */
  enum class NodeType { Room, Hallway, Stairwell, Exit, Unknown };

  /**
   * @brief Result returned by the shortest-path query methods.
   *
   * If @c reachable is false the other fields are undefined.
   */
  struct PathResult {
    bool reachable;                    ///< true if a path from source to target exists.
    int total_cost;                    ///< Sum of edge traversal_time values along the path.
    LinkedList<std::string> path;      ///< Ordered node IDs from source to target (inclusive).

    /**
     * @brief Constructs an unreachable PathResult (default state).
     * @post reachable == false, total_cost == 0, path is empty.
     */
    PathResult() : reachable(false), total_cost(0), path() {}
  };

 private:
  struct Edge;

  /**
   * @brief A vertex (node) in the graph.
   */
  struct Vertex {
    std::string id;        ///< Unique identifier for this node.
    NodeType    type;      ///< Semantic category of this node.
    int         floor;     ///< Floor number this node resides on.
    int         occupants; ///< Current number of people in this node.
    bool        blocked;   ///< true when the node is impassable.
    Edge*       adjacency; ///< Head of the outgoing edge list; nullptr if no edges.
    Vertex*     next;      ///< Next vertex in the global vertex list.

    /**
     * @brief Constructs a Vertex with the given identifier.
     * @param node_id Unique string identifier for this vertex.
     * @post All numeric fields are 0, blocked is false, pointers are nullptr,
     *       type is NodeType::Unknown.
     */
    explicit Vertex(const std::string& node_id)
        : id(node_id),
          type(NodeType::Unknown),
          floor(0),
          occupants(0),
          blocked(false),
          adjacency(nullptr),
          next(nullptr) {}
  };

  /**
   * @brief A directed weighted edge between two vertices.
   */
  struct Edge {
    Vertex* to;             ///< Destination vertex of this directed edge.
    int     traversal_time; ///< Cost (time units) to traverse this edge.
    int     capacity;       ///< Maximum simultaneous occupants allowed.
    bool    blocked;        ///< true when the corridor/stair is impassable.
    Edge*   next;           ///< Next edge in the adjacency list.

    /**
     * @brief Constructs a directed Edge to @p destination.
     * @param destination  Pointer to the target Vertex.
     * @param time_cost    Non-negative traversal cost.
     * @param max_capacity Non-negative maximum occupant capacity.
     * @post blocked == false, next == nullptr.
     */
    Edge(Vertex* destination, int time_cost, int max_capacity)
        : to(destination),
          traversal_time(time_cost),
          capacity(max_capacity),
          blocked(false),
          next(nullptr) {}
  };

  /**
   * @brief A per-vertex record used by Dijkstra's distance table.
   */
  struct DistRecord {
    Vertex*     vertex;   ///< The vertex this record describes.
    int         distance; ///< Tentative shortest distance from the source vertex.
    Vertex*     previous; ///< Predecessor vertex on the shortest path.
    bool        visited;  ///< true once this vertex has been finalised by Dijkstra.
    DistRecord* next;     ///< Next record in the singly-linked distance table.

    /**
     * @brief Constructs a DistRecord for @p v with infinite initial distance.
     * @param v Pointer to the vertex being tracked.
     * @post distance == INT_MAX, visited == false, previous == nullptr.
     */
    explicit DistRecord(Vertex* v)
        : vertex(v),
          distance(std::numeric_limits<int>::max()),
          previous(nullptr),
          visited(false),
          next(nullptr) {}
  };

  /**
   * @brief Priority-queue item pairing a vertex with its tentative distance.
   */
  struct QueueItem {
    Vertex* vertex;   ///< The vertex being enqueued.
    int     distance; ///< Tentative distance at the time of insertion.
  };

  /**
   * @brief Strict-weak-ordering comparator for QueueItem by distance.
   */
  struct QueueCompare {
    /**
     * @brief Compares two QueueItems by ascending distance.
     * @param lhs Left-hand operand.
     * @param rhs Right-hand operand.
     * @return true if lhs.distance < rhs.distance.
     */
    bool operator()(const QueueItem& lhs, const QueueItem& rhs) const {
      return lhs.distance < rhs.distance;
    }
  };

  Vertex* vertices_head_; ///< Head of the global vertex list.
  Vertex* vertices_tail_; ///< Tail of the global vertex list for O(1) append.
  int     vertex_count_;  ///< Total number of vertices currently in the graph.

  // --------------------------------------------------------------------------
  // Private helpers
  // --------------------------------------------------------------------------

  /**
   * @brief Searches the vertex list for a vertex with the given ID.
   * @param node_id The unique string identifier to search for.
   * @return Pointer to the matching Vertex, or nullptr if not found.
   */
  Vertex* findVertex(const std::string& node_id) const {
    Vertex* current = vertices_head_;
    while (current != nullptr) {
      if (current->id == node_id) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  /**
   * @brief Searches @p from's adjacency list for a directed edge to @p to.
   * @param from Source vertex (may be nullptr).
   * @param to   Destination vertex (may be nullptr).
   * @return Pointer to the matching Edge, or nullptr if not found.
   */
  Edge* findEdge(Vertex* from, Vertex* to) const {
    if (from == nullptr || to == nullptr) {
      return nullptr;
    }
    Edge* current = from->adjacency;
    while (current != nullptr) {
      if (current->to == to) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  /**
   * @brief Allocates a DistRecord for every vertex in the graph.
   * @return Pointer to the head of the newly allocated distance-table list.
   */
  DistRecord* buildDistanceTable() const {
    DistRecord* head = nullptr;
    DistRecord* tail = nullptr;

    Vertex* current_vertex = vertices_head_;
    while (current_vertex != nullptr) {
      DistRecord* rec = new DistRecord(current_vertex);
      if (head == nullptr) {
        head = rec;
        tail = rec;
      } else {
        tail->next = rec;
        tail = rec;
      }
      current_vertex = current_vertex->next;
    }
    return head;
  }

  /**
   * @brief Looks up the DistRecord for a specific vertex.
   * @param head   Head of the distance-table list.
   * @param vertex The vertex whose record is sought.
   * @return Pointer to the matching DistRecord, or nullptr if not found.
   */
  DistRecord* recordFor(DistRecord* head, Vertex* vertex) const {
    DistRecord* current = head;
    while (current != nullptr) {
      if (current->vertex == vertex) {
        return current;
      }
      current = current->next;
    }
    return nullptr;
  }

  /**
   * @brief Frees all DistRecord nodes in the linked list headed by @p head.
   * @param head Head of the distance-table list to free (may be nullptr).
   * @post All DistRecord nodes in the list are deleted.
   */
  void freeDistanceTable(DistRecord* head) const {
    DistRecord* current = head;
    while (current != nullptr) {
      DistRecord* temp = current;
      current = current->next;
      delete temp;
    }
  }

  /**
   * @brief Frees all Edge nodes in @p vertex's adjacency list.
   * @param vertex The vertex whose outgoing edges are to be freed.
   * @post vertex->adjacency == nullptr; all edge heap memory is released.
   */
  void clearEdges(Vertex* vertex) {
    Edge* edge = vertex->adjacency;
    while (edge != nullptr) {
      Edge* temp = edge;
      edge = edge->next;
      delete temp;
    }
    vertex->adjacency = nullptr;
  }

 public:
  // --------------------------------------------------------------------------
  // Constructors / destructor / assignment
  // --------------------------------------------------------------------------

  /**
   * @brief Default constructor -- creates an empty graph.
   * @post vertices_head_ == nullptr, vertex_count_ == 0.
   */
  Graph() : vertices_head_(nullptr), vertices_tail_(nullptr), vertex_count_(0) {}

  /**
   * @brief Destructor -- frees all vertices and their edge lists.
   * @post All heap memory owned by this graph is released.
   */
  ~Graph() { clear(); }

  /// Copy construction disabled.
  Graph(const Graph&) = delete;
  /// Copy assignment disabled.
  Graph& operator=(const Graph&) = delete;

  /**
   * @brief Move constructor -- takes ownership of @p other's vertex list.
   * @param other Source graph (left empty but valid after the move).
   */
  Graph(Graph&& other) noexcept
      : vertices_head_(other.vertices_head_),
        vertices_tail_(other.vertices_tail_),
        vertex_count_(other.vertex_count_) {
    other.vertices_head_ = nullptr;
    other.vertices_tail_ = nullptr;
    other.vertex_count_ = 0;
  }

  /**
   * @brief Move-assignment operator.
   * @param other Source graph (left empty but valid after the move).
   * @return Reference to this graph.
   */
  Graph& operator=(Graph&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    clear();
    vertices_head_ = other.vertices_head_;
    vertices_tail_ = other.vertices_tail_;
    vertex_count_ = other.vertex_count_;

    other.vertices_head_ = nullptr;
    other.vertices_tail_ = nullptr;
    other.vertex_count_ = 0;
    return *this;
  }

  // --------------------------------------------------------------------------
  // Graph mutation
  // --------------------------------------------------------------------------

  /**
   * @brief Adds a new vertex to the graph.
   *
   * @param node_id   Unique string identifier for the new vertex.
   * @param type      Semantic category (default: NodeType::Unknown).
   * @param floor     Floor number (default: 0).
   * @param occupants Initial occupant count (default: 0).
   * @return true if the vertex was created; false if the ID already exists.
   * @post If true, nodeCount() increases by 1.
   */
  bool addNode(const std::string& node_id,
               NodeType type = NodeType::Unknown,
               int floor = 0,
               int occupants = 0) {
    if (findVertex(node_id) != nullptr) {
      return false;
    }

    Vertex* vertex = new Vertex(node_id);
    vertex->type = type;
    vertex->floor = floor;
    vertex->occupants = occupants;

    if (vertices_tail_ == nullptr) {
      vertices_head_ = vertex;
      vertices_tail_ = vertex;
    } else {
      vertices_tail_->next = vertex;
      vertices_tail_ = vertex;
    }
    ++vertex_count_;
    return true;
  }

  /**
   * @brief Adds a directed (or bidirectional) weighted edge between two vertices.
   *
   * @param from_id        ID of the source vertex.
   * @param to_id          ID of the destination vertex.
   * @param traversal_time Non-negative time cost to traverse the edge.
   * @param capacity       Non-negative maximum concurrent occupant capacity.
   * @param bidirectional  If true (default), also adds the reverse edge.
   * @return true if both vertices were found; false otherwise.
   * @throws std::invalid_argument if traversal_time or capacity is negative.
   */
  bool addEdge(const std::string& from_id,
               const std::string& to_id,
               int traversal_time,
               int capacity,
               bool bidirectional = true) {
    if (traversal_time < 0 || capacity < 0) {
      throw std::invalid_argument("Traversal time and capacity must be non-negative.");
    }

    Vertex* from = findVertex(from_id);
    Vertex* to = findVertex(to_id);
    if (from == nullptr || to == nullptr) {
      return false;
    }

    if (findEdge(from, to) == nullptr) {
      Edge* edge = new Edge(to, traversal_time, capacity);
      edge->next = from->adjacency;
      from->adjacency = edge;
    }

    if (bidirectional && findEdge(to, from) == nullptr) {
      Edge* reverse = new Edge(from, traversal_time, capacity);
      reverse->next = to->adjacency;
      to->adjacency = reverse;
    }

    return true;
  }

  /**
   * @brief Sets or clears the blocked flag on a vertex.
   * @param node_id ID of the target vertex.
   * @param blocked New blocked state.
   * @return true if the vertex was found and updated; false otherwise.
   */
  bool setNodeBlocked(const std::string& node_id, bool blocked) {
    Vertex* vertex = findVertex(node_id);
    if (vertex == nullptr) {
      return false;
    }
    vertex->blocked = blocked;
    return true;
  }

  /**
   * @brief Sets or clears the blocked flag on both directions of an edge.
   * @param from_id ID of the source vertex.
   * @param to_id   ID of the destination vertex.
   * @param blocked New blocked state for the edge(s).
   * @return true if at least one edge direction was found; false otherwise.
   */
  bool setEdgeBlocked(const std::string& from_id, const std::string& to_id, bool blocked) {
    Vertex* from = findVertex(from_id);
    Vertex* to = findVertex(to_id);
    if (from == nullptr || to == nullptr) {
      return false;
    }

    bool updated = false;
    Edge* forward = findEdge(from, to);
    if (forward != nullptr) {
      forward->blocked = blocked;
      updated = true;
    }
    Edge* backward = findEdge(to, from);
    if (backward != nullptr) {
      backward->blocked = blocked;
      updated = true;
    }
    return updated;
  }

  // --------------------------------------------------------------------------
  // Shortest-path queries
  // --------------------------------------------------------------------------

  /**
   * @brief Computes the shortest path between two vertices using Dijkstra.
   *
   * Blocked vertices and blocked edges are excluded from consideration.
   *
   * @param start_id  ID of the source vertex.
   * @param target_id ID of the destination vertex.
   * @return A PathResult describing reachability, total cost, and the ordered
   *         node sequence.
   */
  PathResult shortestPath(const std::string& start_id, const std::string& target_id) const {
    PathResult result;
    Vertex* start = findVertex(start_id);
    Vertex* target = findVertex(target_id);

    if (start == nullptr || target == nullptr || start->blocked || target->blocked) {
      return result;
    }

    DistRecord* table = buildDistanceTable();
    DistRecord* start_rec = recordFor(table, start);
    if (start_rec == nullptr) {
      freeDistanceTable(table);
      return result;
    }

    start_rec->distance = 0;

    PriorityQueue<QueueItem, QueueCompare> pq;
    pq.push({start, 0});

    while (!pq.empty()) {
      QueueItem current_item = pq.top();
      pq.pop();

      DistRecord* current_rec = recordFor(table, current_item.vertex);
      if (current_rec == nullptr || current_rec->visited) {
        continue;
      }
      if (current_item.distance > current_rec->distance) {
        continue;
      }

      current_rec->visited = true;
      if (current_item.vertex == target) {
        break;
      }

      Edge* edge = current_item.vertex->adjacency;
      while (edge != nullptr) {
        if (!edge->blocked && !edge->to->blocked) {
          DistRecord* next_rec = recordFor(table, edge->to);
          if (next_rec != nullptr && !next_rec->visited &&
              current_rec->distance != std::numeric_limits<int>::max()) {
            const int candidate = current_rec->distance + edge->traversal_time;
            if (candidate < next_rec->distance) {
              next_rec->distance = candidate;
              next_rec->previous = current_item.vertex;
              pq.push({edge->to, candidate});
            }
          }
        }
        edge = edge->next;
      }
    }

    DistRecord* target_rec = recordFor(table, target);
    if (target_rec != nullptr && target_rec->distance != std::numeric_limits<int>::max()) {
      result.reachable = true;
      result.total_cost = target_rec->distance;

      Vertex* walk = target;
      while (walk != nullptr) {
        result.path.push_front(walk->id);
        DistRecord* walk_rec = recordFor(table, walk);
        walk = (walk_rec == nullptr) ? nullptr : walk_rec->previous;
      }
    }

    freeDistanceTable(table);
    return result;
  }

  /**
   * @brief Finds the shortest path from @p start_id to the nearest unblocked exit.
   *
   * @param start_id ID of the source vertex.
   * @return A PathResult for the nearest reachable exit, or an unreachable
   *         PathResult if no exit is reachable.
   */
  PathResult shortestPathToNearestExit(const std::string& start_id) const {
    PathResult best;
    Vertex* start = findVertex(start_id);
    if (start == nullptr || start->blocked) {
      return best;
    }

    Vertex* current = vertices_head_;
    while (current != nullptr) {
      if (current->type == NodeType::Exit && !current->blocked) {
        PathResult candidate = shortestPath(start_id, current->id);
        if (candidate.reachable &&
            (!best.reachable || candidate.total_cost < best.total_cost)) {
          best = candidate;
        }
      }
      current = current->next;
    }
    return best;
  }

  /**
   * @brief Reports whether any unblocked exit is reachable from @p start_id.
   * @param start_id ID of the source vertex.
   * @return true if at least one unblocked exit is reachable; false otherwise.
   */
  bool isReachableToAnyExit(const std::string& start_id) const {
    PathResult result = shortestPathToNearestExit(start_id);
    return result.reachable;
  }

  // --------------------------------------------------------------------------
  // Accessors
  // --------------------------------------------------------------------------

  /**
   * @brief Returns the number of vertices currently in the graph.
   * @return Non-negative vertex count.
   */
  int nodeCount() const { return vertex_count_; }

  /**
   * @brief Removes all vertices and edges, freeing all heap memory.
   * @post nodeCount() == 0, vertices_head_ == nullptr.
   */
  void clear() {
    Vertex* current = vertices_head_;
    while (current != nullptr) {
      Vertex* temp = current;
      current = current->next;
      clearEdges(temp);
      delete temp;
    }
    vertices_head_ = nullptr;
    vertices_tail_ = nullptr;
    vertex_count_ = 0;
  }
};
