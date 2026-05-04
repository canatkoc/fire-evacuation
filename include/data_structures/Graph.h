#pragma once

#include <limits>
#include <stdexcept>
#include <string>

#include "LinkedList.h"
#include "priority_queue.h"

class Graph {
 public:
  enum class NodeType { Room, Hallway, Stairwell, Exit, Unknown };

  struct PathResult {
    bool reachable;
    int total_cost;
    LinkedList<std::string> path;

    PathResult() : reachable(false), total_cost(0), path() {}
  };

 private:
  struct Edge;

  struct Vertex {
    std::string id;
    NodeType type;
    int floor;
    int occupants;
    bool blocked;
    Edge* adjacency;
    Vertex* next;

    explicit Vertex(const std::string& node_id)
        : id(node_id),
          type(NodeType::Unknown),
          floor(0),
          occupants(0),
          blocked(false),
          adjacency(nullptr),
          next(nullptr) {}
  };

  struct Edge {
    Vertex* to;
    int traversal_time;
    int capacity;
    bool blocked;
    Edge* next;

    Edge(Vertex* destination, int time_cost, int max_capacity)
        : to(destination),
          traversal_time(time_cost),
          capacity(max_capacity),
          blocked(false),
          next(nullptr) {}
  };

  struct DistRecord {
    Vertex* vertex;
    int distance;
    Vertex* previous;
    bool visited;
    DistRecord* next;

    explicit DistRecord(Vertex* v)
        : vertex(v),
          distance(std::numeric_limits<int>::max()),
          previous(nullptr),
          visited(false),
          next(nullptr) {}
  };

  struct QueueItem {
    Vertex* vertex;
    int distance;
  };

  struct QueueCompare {
    bool operator()(const QueueItem& lhs, const QueueItem& rhs) const {
      return lhs.distance < rhs.distance;
    }
  };

  Vertex* vertices_head_;
  Vertex* vertices_tail_;
  int vertex_count_;

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

  void freeDistanceTable(DistRecord* head) const {
    DistRecord* current = head;
    while (current != nullptr) {
      DistRecord* temp = current;
      current = current->next;
      delete temp;
    }
  }

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
  Graph() : vertices_head_(nullptr), vertices_tail_(nullptr), vertex_count_(0) {}

  ~Graph() { clear(); }

  Graph(const Graph&) = delete;
  Graph& operator=(const Graph&) = delete;

  Graph(Graph&& other) noexcept
      : vertices_head_(other.vertices_head_),
        vertices_tail_(other.vertices_tail_),
        vertex_count_(other.vertex_count_) {
    other.vertices_head_ = nullptr;
    other.vertices_tail_ = nullptr;
    other.vertex_count_ = 0;
  }

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

  bool setNodeBlocked(const std::string& node_id, bool blocked) {
    Vertex* vertex = findVertex(node_id);
    if (vertex == nullptr) {
      return false;
    }
    vertex->blocked = blocked;
    return true;
  }

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

  bool isReachableToAnyExit(const std::string& start_id) const {
    PathResult result = shortestPathToNearestExit(start_id);
    return result.reachable;
  }

  int nodeCount() const { return vertex_count_; }

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
