/**
 * @file FireSimulator.h
 * @brief Core fire-evacuation simulation engine.
 *
 * FireSimulator implements two interleaved sub-tasks per simulation tick:
 *
 *  - Task 3 - Dynamic Route Planner: custom Dijkstra over CellNode/Corridor
 *    topology using the project's PriorityQueue. The cost model is predictive:
 *    edge weights incorporate fire-proximity penalties and capacity congestion.
 *
 *  - Task 4 - Fire Spread: per-tick propagation from ignited nodes through
 *    corridors based on per-element flammability. When a corridor's intensity
 *    crosses fire_block_threshold_ it is flagged blocked in both the internal
 *    Corridor struct and in the shared Graph.
 *
 * No STL containers are used: storage is exclusively through LinkedList,
 * Queue, PriorityQueue, Graph and intrusive pointer lists in SimEntities.h.
 *
 * @author CSE 211 Group
 * @date 2026
 */
#pragma once

#include <string>
#include <limits>
#include <cmath>

#include "../data_structures/LinkedList.h"
#include "../data_structures/Queue.h"
#include "../data_structures/priority_queue.h"
#include "../data_structures/Graph.h"
#include "SimEntities.h"

/**
 * @brief Fire-aware building evacuation simulator.
 *
 * Maintains a private mirrored topology (CellNode/Corridor lists) alongside
 * the caller-supplied Graph. Every addNode()/addCorridor() call registers
 * the element in both structures; every fire-spread step updates blocked flags
 * in both so that the Graph can be queried by external code (e.g., the GUI).
 *
 * The simulation loop is driven by step() (one tick) or run() (multiple ticks).
 *
 * Copy construction and copy assignment are disabled.
 */
class FireSimulator {
 public:
  // --------------------------------------------------------------------------
  // Public types
  // --------------------------------------------------------------------------

  /**
   * @brief Result of a fire-aware evacuation path query.
   *
   * If @c reachable is false the other fields are undefined.
   */
  struct EvacPath {
    bool reachable;                        ///< true if a safe path to an exit was found.
    long long total_cost;                  ///< Predictive cost of the best path found.
    LinkedList<std::string> node_sequence; ///< Ordered node IDs from start to exit (inclusive).

    /**
     * @brief Constructs an unreachable EvacPath (default state).
     * @post reachable == false, total_cost == 0, node_sequence is empty.
     */
    EvacPath() : reachable(false), total_cost(0), node_sequence() {}
  };

  // --------------------------------------------------------------------------
  // Constructor / destructor / assignment
  // --------------------------------------------------------------------------

  /**
   * @brief Constructs a FireSimulator bound to an external Graph.
   *
   * Default parameter values:
   *  - fire_block_threshold_  = 1.0
   *  - node_lethal_threshold_ = 0.95
   *  - fire_spread_rate_      = 0.18
   *  - prediction_horizon_    = 8
   *  - prediction_penalty_    = 2.5
   *
   * @param graph Reference to the Graph that mirrors this simulator's topology.
   * @post All counters are zero; simulation starts at time step 0.
   */
  explicit FireSimulator(Graph& graph)
      : graph_(&graph),
        nodes_head_(nullptr),
        nodes_tail_(nullptr),
        corridors_head_(nullptr),
        node_count_(0),
        corridor_count_(0),
        people_head_(nullptr),
        people_tail_(nullptr),
        person_count_(0),
        next_person_id_(0),
        queues_head_(nullptr),
        time_step_(0),
        evacuated_count_(0),
        casualty_count_(0),
        fire_block_threshold_(1.0),
        node_lethal_threshold_(0.95),
        fire_spread_rate_(0.18),
        prediction_horizon_(8),
        prediction_penalty_(2.5) {}

  /**
   * @brief Destructor -- frees all nodes, corridors, persons, and waiting queues.
   * @post All heap memory owned by this simulator is released.
   */
  ~FireSimulator() { clearAll(); }

  /// Copy construction disabled.
  FireSimulator(const FireSimulator&) = delete;
  /// Copy assignment disabled.
  FireSimulator& operator=(const FireSimulator&) = delete;

  // --------------------------------------------------------------------------
  // Setup API
  // --------------------------------------------------------------------------

  /**
   * @brief Registers a node in the simulator and in the shared Graph.
   *
   * @param id               Unique string identifier for the node.
   * @param type             Semantic category.
   * @param floor            Floor number.
   * @param initial_occupants Number of people to place in the node at t=0.
   * @param flammability     Ignition rate in [0,1]; clamped internally.
   * @return true if the node was created; false if the ID already exists.
   * @post nodeCount() incremented by 1; totalPeople() incremented by @p initial_occupants.
   */
  bool addNode(const std::string& id,
               Graph::NodeType type,
               int floor,
               int initial_occupants,
               double flammability) {
    if (findNode(id) != nullptr) return false;

    CellNode* n = new CellNode(id);
    n->type = type;
    n->floor = floor;
    n->is_exit = (type == Graph::NodeType::Exit);
    n->flammability = clamp01(flammability);
    n->people_in_node = initial_occupants;

    if (nodes_tail_ == nullptr) {
        nodes_head_ = n;
        nodes_tail_ = n;
    } else {
        nodes_tail_->next = n;
        nodes_tail_ = n;
    }
    ++node_count_;

    graph_->addNode(id, type, floor, initial_occupants);

    for (int i = 0; i < initial_occupants; ++i) {
        Person* p = new Person(next_person_id_++, id);
        if (people_tail_ == nullptr) {
            people_head_ = p;
            people_tail_ = p;
        } else {
            people_tail_->next = p;
            people_tail_ = p;
        }
        ++person_count_;
    }
    return true;
  }

  /**
   * @brief Registers a corridor between two existing nodes.
   *
   * @param from_id        ID of the source node.
   * @param to_id          ID of the destination node.
   * @param traversal_time Ticks to cross (clamped to >= 1).
   * @param capacity       Maximum concurrent walkers (0 = unlimited).
   * @param flammability   Ignition rate in [0,1]; clamped internally.
   * @param bidirectional  If true (default), the reverse direction is also added.
   * @return true if both nodes were found; false otherwise.
   * @post corridorCount() is incremented by 1 (or 2 if bidirectional).
   */
  bool addCorridor(const std::string& from_id,
                   const std::string& to_id,
                   int traversal_time,
                   int capacity,
                   double flammability,
                   bool bidirectional = true) {
    if (traversal_time < 1) traversal_time = 1;
    CellNode* a = findNode(from_id);
    CellNode* b = findNode(to_id);
    if (a == nullptr || b == nullptr) return false;

    if (findCorridor(a, b) == nullptr) {
        Corridor* c = new Corridor(a, b, traversal_time, capacity, clamp01(flammability));
        c->next_global = corridors_head_;
        corridors_head_ = c;
        ++corridor_count_;
    }
    if (bidirectional && findCorridor(b, a) == nullptr) {
        Corridor* c2 = new Corridor(b, a, traversal_time, capacity, clamp01(flammability));
        c2->next_global = corridors_head_;
        corridors_head_ = c2;
        ++corridor_count_;
    }

    graph_->addEdge(from_id, to_id, traversal_time, capacity, bidirectional);
    return true;
  }

  /**
   * @brief Sets a node on fire with a given initial intensity.
   * @param id                Unique identifier of the node to ignite.
   * @param initial_intensity Starting fire intensity in [0,1] (clamped).
   * @return true if the node was found and ignited; false otherwise.
   * @post node->on_fire == true, node->fire_intensity == clamp01(initial_intensity).
   */
  bool igniteNode(const std::string& id, double initial_intensity = 1.0) {
    CellNode* n = findNode(id);
    if (n == nullptr) return false;
    n->on_fire = true;
    n->fire_intensity = clamp01(initial_intensity);
    return true;
  }

  // --------------------------------------------------------------------------
  // Simulation tick
  // --------------------------------------------------------------------------

  /**
   * @brief Advances the simulation by one tick.
   *
   * Executes in order: propagateFire(), processCasualties(),
   * releaseStaleQueues(), stepPeople().
   *
   * @post time_step_ is incremented by 1.
   */
  void step() {
    propagateFire();
    processCasualties();
    releaseStaleQueues();
    stepPeople();
    ++time_step_;
  }

  /**
   * @brief Runs the simulation for up to @p max_steps ticks.
   *
   * Stops early if simulationFinished() returns true. Any persons still
   * active when the step limit is reached are counted as casualties.
   *
   * @param max_steps Maximum number of ticks to execute.
   */
  void run(int max_steps) {
    int guard = 0;
    while (guard < max_steps && !simulationFinished()) {
        step();
        ++guard;
    }
    Person* p = people_head_;
    while (p != nullptr) {
        if (!p->evacuated && !p->dead) {
            p->dead = true;
            ++casualty_count_;
        }
        p = p->next;
    }
  }

  // --------------------------------------------------------------------------
  // Task 4: Fire spread
  // --------------------------------------------------------------------------

  /**
   * @brief Propagates fire through corridors and nodes for one tick.
   *
   * Phase 1: each unblocked corridor absorbs fire from its endpoint nodes.
   * Phase 2: each non-exit node accumulates fire from incoming corridors
   * (compute-apply separated to prevent cascading within one tick).
   *
   * @post fire_intensity fields updated; blocked flags set where thresholds crossed.
   */
  void propagateFire() {
    Corridor* c = corridors_head_;
    while (c != nullptr) {
        if (!c->blocked) {
            double from_i = c->from->fire_intensity;
            double to_i   = c->to->fire_intensity;
            double endpoint_max = (from_i > to_i) ? from_i : to_i;

            if (endpoint_max > 0.0) {
                double increment = fire_spread_rate_ * c->flammability * endpoint_max;
                c->fire_intensity = clamp01(c->fire_intensity + increment);
                if (c->fire_intensity >= fire_block_threshold_) {
                    c->blocked = true;
                    graph_->setEdgeBlocked(c->from->id, c->to->id, true);
                }
            }
        }
        c = c->next_global;
    }

    struct Delta {
        CellNode* node;
        double delta;
        Delta* next;
    };
    Delta* deltas_head = nullptr;

    CellNode* n = nodes_head_;
    while (n != nullptr) {
        if (!n->is_exit) {
            double incoming_max = 0.0;
            Corridor* cc = corridors_head_;
            while (cc != nullptr) {
                if (cc->to == n && cc->fire_intensity > incoming_max) {
                    incoming_max = cc->fire_intensity;
                }
                cc = cc->next_global;
            }
            if (incoming_max > 0.0) {
                double d = fire_spread_rate_ * n->flammability * incoming_max;
                Delta* item = new Delta{n, d, deltas_head};
                deltas_head = item;
            }
        }
        n = n->next;
    }

    Delta* dcur = deltas_head;
    while (dcur != nullptr) {
        dcur->node->fire_intensity = clamp01(dcur->node->fire_intensity + dcur->delta);
        if (dcur->node->fire_intensity >= 0.5) {
            dcur->node->on_fire = true;
        }
        if (dcur->node->fire_intensity >= node_lethal_threshold_) {
            Corridor* nb = corridors_head_;
            while (nb != nullptr) {
                if ((nb->from == dcur->node || nb->to == dcur->node) && !nb->blocked) {
                    nb->blocked = true;
                    graph_->setEdgeBlocked(nb->from->id, nb->to->id, true);
                }
                nb = nb->next_global;
            }
        }
        Delta* tmp = dcur;
        dcur = dcur->next;
        delete tmp;
    }
  }

  // --------------------------------------------------------------------------
  // Task 3: Dynamic evacuation routing
  // --------------------------------------------------------------------------

  /**
   * @brief Finds the safest path from @p start_id to the nearest unblocked exit.
   *
   * Runs a custom Dijkstra using the project's PriorityQueue. The edge cost
   * function is predictiveCost(), which adds a fire-proximity penalty and a
   * capacity penalty. Nodes at or above node_lethal_threshold_ are skipped.
   *
   * @param start_id ID of the node from which to plan the evacuation route.
   * @return An EvacPath with reachable == true if a safe path exists.
   * @pre  @p start_id must exist in the simulator's node list.
   * @post The internal Dijkstra distance table is allocated and freed locally.
   */
  EvacPath findSafestExit(const std::string& start_id) const {
    EvacPath result;
    CellNode* start = findNode(start_id);
    if (start == nullptr) return result;
    if (start->fire_intensity >= node_lethal_threshold_) return result;

    DijkstraRecord* table = buildRecords();
    DijkstraRecord* start_rec = recFor(table, start);
    if (start_rec == nullptr) {
        freeRecords(table);
        return result;
    }
    start_rec->distance = 0;

    PriorityQueue<DjItem, DjCompare> pq;
    pq.push({start, 0});

    CellNode* best_exit = nullptr;
    long long best_cost = std::numeric_limits<long long>::max();

    while (!pq.empty()) {
        DjItem item = pq.top();
        pq.pop();

        DijkstraRecord* rec = recFor(table, item.node);
        if (rec == nullptr || rec->visited) continue;
        if (item.distance > rec->distance) continue;
        rec->visited = true;

        if (item.node->is_exit) {
            best_cost = item.distance;
            best_exit = item.node;
            break;
        }

        if (item.node->fire_intensity >= node_lethal_threshold_) continue;

        Corridor* cc = corridors_head_;
        while (cc != nullptr) {
            if (cc->from == item.node && !cc->blocked) {
                long long cost = predictiveCost(cc);
                if (cost != std::numeric_limits<long long>::max() &&
                    rec->distance != std::numeric_limits<long long>::max()) {
                    long long cand = rec->distance + cost;
                    DijkstraRecord* nxt = recFor(table, cc->to);
                    if (nxt != nullptr && !nxt->visited && cand < nxt->distance) {
                        nxt->distance = cand;
                        nxt->previous = item.node;
                        pq.push({cc->to, cand});
                    }
                }
            }
            cc = cc->next_global;
        }
    }

    if (best_exit != nullptr) {
        result.reachable = true;
        result.total_cost = best_cost;
        CellNode* walk = best_exit;
        while (walk != nullptr) {
            result.node_sequence.push_front(walk->id);
            DijkstraRecord* wrec = recFor(table, walk);
            walk = (wrec == nullptr) ? nullptr : wrec->previous;
        }
    }

    freeRecords(table);
    return result;
  }

  // --------------------------------------------------------------------------
  // Combined: movement + queuing
  // --------------------------------------------------------------------------

  /**
   * @brief Advances all persons by one tick (three phases).
   *
   * Phase A: advance persons on corridors; place arrivals in destination nodes.
   * Phase B: drain WaitingQueues where capacity allows.
   * Phase C: assign routes to idle persons via findSafestExit().
   *
   * @post Person states updated; evacuated_count_ and casualty_count_ may increase.
   */
  void stepPeople() {
    // Phase A: advance persons on corridors.
    Person* p = people_head_;
    while (p != nullptr) {
        if (p->dead || p->evacuated || !p->on_edge) { p = p->next; continue; }

        ++p->progress_on_edge;
        if (p->progress_on_edge >= p->edge_traversal_time) {
            CellNode* src = findNode(p->current_location);
            CellNode* dst = findNode(p->next_location);
            Corridor* c   = (src && dst) ? findCorridor(src, dst) : nullptr;
            if (c != nullptr) {
                --c->current_load;
                if (c->current_load < 0) c->current_load = 0;
            }

            p->current_location = p->next_location;
            p->next_location = "";
            p->on_edge = false;
            p->progress_on_edge = 0;
            p->edge_traversal_time = 0;

            if (dst != nullptr) {
                ++dst->people_in_node;
                if (dst->is_exit) {
                    p->evacuated = true;
                    ++evacuated_count_;
                    --dst->people_in_node;
                } else if (dst->fire_intensity >= node_lethal_threshold_) {
                    p->dead = true;
                    ++casualty_count_;
                    --dst->people_in_node;
                }
            }
        }
        p = p->next;
    }

    // Phase B: drain waiting queues.
    WaitingQueue* wq = queues_head_;
    while (wq != nullptr) {
        Corridor* c = wq->corridor;
        while (!wq->queue.is_empty() &&
               !c->blocked &&
               (c->capacity <= 0 || c->current_load < c->capacity)) {
            Person* qp = wq->queue.dequeue();
            if (qp == nullptr || qp->dead || qp->evacuated) continue;

            CellNode* src = findNode(qp->current_location);
            if (src == nullptr || src != c->from) {
                qp->in_queue = false;
                continue;
            }
            if (src->fire_intensity >= node_lethal_threshold_) {
                qp->dead = true;
                ++casualty_count_;
                if (src->people_in_node > 0) --src->people_in_node;
                qp->in_queue = false;
                continue;
            }

            qp->in_queue = false;
            qp->on_edge = true;
            qp->next_location = c->to->id;
            qp->progress_on_edge = 0;
            qp->edge_traversal_time = c->traversal_time;
            ++c->current_load;
            if (src->people_in_node > 0) --src->people_in_node;
        }
        wq = wq->next;
    }

    // Phase C: assign routes to idle persons.
    Person* q = people_head_;
    while (q != nullptr) {
        if (q->dead || q->evacuated || q->on_edge || q->in_queue) {
            q = q->next; continue;
        }

        CellNode* here = findNode(q->current_location);
        if (here == nullptr) { q = q->next; continue; }

        if (here->is_exit) {
            q->evacuated = true;
            ++evacuated_count_;
            if (here->people_in_node > 0) --here->people_in_node;
            q = q->next; continue;
        }
        if (here->fire_intensity >= node_lethal_threshold_) {
            q->dead = true;
            ++casualty_count_;
            if (here->people_in_node > 0) --here->people_in_node;
            q = q->next; continue;
        }

        EvacPath plan = findSafestExit(here->id);
        if (!plan.reachable) { q = q->next; continue; }

        LinkedList<std::string> seq = plan.node_sequence;
        if (seq.get_size() < 2) { q = q->next; continue; }
        seq.pop_front();
        std::string next_id = seq.pop_front();

        CellNode* nxt = findNode(next_id);
        if (nxt == nullptr) { q = q->next; continue; }

        Corridor* c = findCorridor(here, nxt);
        if (c == nullptr || c->blocked) { q = q->next; continue; }

        if (c->capacity > 0 && c->current_load >= c->capacity) {
            WaitingQueue* w = getOrCreateQueue(c);
            w->queue.enqueue(q);
            q->in_queue = true;
        } else {
            q->on_edge = true;
            q->next_location = nxt->id;
            q->progress_on_edge = 0;
            q->edge_traversal_time = c->traversal_time;
            ++c->current_load;
            if (here->people_in_node > 0) --here->people_in_node;
        }
        q = q->next;
    }
  }

  /**
   * @brief Marks persons killed by fire for the current tick.
   *
   * Stationary persons in a lethal node and persons on a fully ablaze blocked
   * corridor are killed immediately.
   *
   * @post dead == true and casualty_count_ incremented for affected persons.
   */
  void processCasualties() {
    Person* p = people_head_;
    while (p != nullptr) {
        if (p->dead || p->evacuated) { p = p->next; continue; }

        if (!p->on_edge) {
            CellNode* here = findNode(p->current_location);
            if (here != nullptr && here->fire_intensity >= node_lethal_threshold_) {
                p->dead = true;
                ++casualty_count_;
                if (here->people_in_node > 0) --here->people_in_node;
            }
        } else {
            CellNode* src = findNode(p->current_location);
            CellNode* dst = findNode(p->next_location);
            Corridor* c = (src && dst) ? findCorridor(src, dst) : nullptr;
            if (c != nullptr && c->blocked &&
                c->fire_intensity >= fire_block_threshold_) {
                p->dead = true;
                ++casualty_count_;
                --c->current_load;
                if (c->current_load < 0) c->current_load = 0;
            }
        }
        p = p->next;
    }
  }

  /**
   * @brief Releases all persons queued in now-blocked corridors.
   *
   * @post Persons in queues of blocked corridors have in_queue reset to false.
   */
  void releaseStaleQueues() {
    WaitingQueue* wq = queues_head_;
    while (wq != nullptr) {
        Corridor* c = wq->corridor;
        if (c->blocked) {
            while (!wq->queue.is_empty()) {
                Person* qp = wq->queue.dequeue();
                if (qp != nullptr) qp->in_queue = false;
            }
        }
        wq = wq->next;
    }
  }

  // --------------------------------------------------------------------------
  // State queries
  // --------------------------------------------------------------------------

  /**
   * @brief Reports whether all persons have either evacuated or died.
   * @return true if no person is still alive and moving.
   */
  bool simulationFinished() const {
    Person* p = people_head_;
    while (p != nullptr) {
        if (!p->evacuated && !p->dead) return false;
        p = p->next;
    }
    return true;
  }

  /** @brief Returns the current simulation time (completed ticks). */
  int currentTime()     const { return time_step_; }
  /** @brief Returns the number of persons that have reached an exit. */
  int evacuatedCount()  const { return evacuated_count_; }
  /** @brief Returns the number of persons that have died. */
  int casualtyCount()   const { return casualty_count_; }
  /** @brief Returns the total persons created at simulation start. */
  int totalPeople()     const { return person_count_; }
  /** @brief Returns the number of nodes registered in the simulator. */
  int nodeCount()       const { return node_count_; }
  /** @brief Returns the number of corridor directions registered. */
  int corridorCount()   const { return corridor_count_; }

  /**
   * @brief Returns the current fire intensity of the specified node.
   * @param node_id ID of the node to query.
   * @return Fire intensity in [0,1]; 0.0 if the node is not found.
   */
  double fireIntensityAt(const std::string& node_id) const {
    CellNode* n = findNode(node_id);
    return (n == nullptr) ? 0.0 : n->fire_intensity;
  }

  /**
   * @brief Reports whether the corridor from @p from_id to @p to_id is blocked.
   * @return true if blocked or does not exist.
   */
  bool isCorridorBlocked(const std::string& from_id, const std::string& to_id) const {
    CellNode* a = findNode(from_id);
    CellNode* b = findNode(to_id);
    Corridor* c = (a && b) ? findCorridor(a, b) : nullptr;
    return (c == nullptr) ? true : c->blocked;
  }

  /**
   * @brief Fills output parameters with a summary snapshot of the current state.
   * @param out_time  Receives the current time step.
   * @param out_alive Receives the number of living, non-evacuated persons.
   * @param out_evac  Receives the evacuated count.
   * @param out_dead  Receives the casualty count.
   */
  void snapshot(int& out_time, int& out_alive, int& out_evac, int& out_dead) const {
    out_time  = time_step_;
    out_evac  = evacuated_count_;
    out_dead  = casualty_count_;
    out_alive = person_count_ - evacuated_count_ - casualty_count_;
  }

  // --------------------------------------------------------------------------
  // Tuning parameters
  // --------------------------------------------------------------------------

  /** @brief Sets the fire spread rate (fraction of endpoint intensity per tick). */
  void setFireSpreadRate(double r)      { fire_spread_rate_ = r; }
  /** @brief Sets the corridor-blocking threshold. @param t clamped to [0,1]. */
  void setFireBlockThreshold(double t)  { fire_block_threshold_ = clamp01(t); }
  /** @brief Sets the node lethal threshold. @param t clamped to [0,1]. */
  void setNodeLethalThreshold(double t) { node_lethal_threshold_ = clamp01(t); }
  /** @brief Sets the look-ahead horizon for the predictive cost model. */
  void setPredictionHorizon(int h)      { prediction_horizon_ = (h < 1) ? 1 : h; }
  /** @brief Sets the multiplier applied to the fire-proximity penalty. */
  void setPredictionPenalty(double k)   { prediction_penalty_ = (k < 0.0) ? 0.0 : k; }

  /** @brief Returns the head of the global person list (read-only). */
  const Person*   peopleHead()    const { return people_head_; }
  /** @brief Returns the head of the global CellNode list (read-only). */
  const CellNode* nodesHead()     const { return nodes_head_; }
  /** @brief Returns the head of the global Corridor list (read-only). */
  const Corridor* corridorsHead() const { return corridors_head_; }

 private:
  // --------------------------------------------------------------------------
  // Internal Dijkstra types
  // --------------------------------------------------------------------------

  /** @brief Per-node record for the fire-aware Dijkstra distance table. */
  struct DijkstraRecord {
    CellNode*       node;     ///< The CellNode this record describes.
    long long       distance; ///< Tentative predictive cost from the start node.
    CellNode*       previous; ///< Predecessor on the best path; nullptr if none.
    bool            visited;  ///< true once this node has been finalised.
    DijkstraRecord* next;     ///< Next record in the singly-linked distance table.

    /**
     * @brief Constructs a DijkstraRecord with infinite initial distance.
     * @param n CellNode to track.
     */
    explicit DijkstraRecord(CellNode* n)
        : node(n),
          distance(std::numeric_limits<long long>::max()),
          previous(nullptr),
          visited(false),
          next(nullptr) {}
  };

  /** @brief Priority-queue item for the fire-aware Dijkstra. */
  struct DjItem {
    CellNode* node;     ///< The node being enqueued.
    long long distance; ///< Predictive cost at insertion time.
  };

  /** @brief Min-heap comparator for DjItem by ascending distance. */
  struct DjCompare {
    bool operator()(const DjItem& a, const DjItem& b) const {
      return a.distance < b.distance;
    }
  };

  // --------------------------------------------------------------------------
  // Private helpers
  // --------------------------------------------------------------------------

  /** @brief Clamps @p x to [0,1]. */
  static double clamp01(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
  }

  /** @brief Searches the node list for a node with the given identifier. */
  CellNode* findNode(const std::string& id) const {
    CellNode* cur = nodes_head_;
    while (cur != nullptr) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return nullptr;
  }

  /** @brief Searches the corridor list for a directed corridor from @p from to @p to. */
  Corridor* findCorridor(CellNode* from, CellNode* to) const {
    Corridor* cur = corridors_head_;
    while (cur != nullptr) {
        if (cur->from == from && cur->to == to) return cur;
        cur = cur->next_global;
    }
    return nullptr;
  }

  /** @brief Returns the WaitingQueue for @p c, creating one if needed. */
  WaitingQueue* getOrCreateQueue(Corridor* c) {
    WaitingQueue* cur = queues_head_;
    while (cur != nullptr) {
        if (cur->corridor == c) return cur;
        cur = cur->next;
    }
    WaitingQueue* nq = new WaitingQueue(c);
    nq->next = queues_head_;
    queues_head_ = nq;
    return nq;
  }

  /** @brief Allocates a DijkstraRecord for every CellNode in the simulator. */
  DijkstraRecord* buildRecords() const {
    DijkstraRecord* head = nullptr;
    DijkstraRecord* tail = nullptr;
    CellNode* n = nodes_head_;
    while (n != nullptr) {
        DijkstraRecord* r = new DijkstraRecord(n);
        if (head == nullptr) { head = r; tail = r; }
        else                 { tail->next = r; tail = r; }
        n = n->next;
    }
    return head;
  }

  /** @brief Looks up the DijkstraRecord for CellNode @p n. */
  DijkstraRecord* recFor(DijkstraRecord* head, CellNode* n) const {
    DijkstraRecord* cur = head;
    while (cur != nullptr) {
        if (cur->node == n) return cur;
        cur = cur->next;
    }
    return nullptr;
  }

  /** @brief Frees all DijkstraRecord nodes in the list headed by @p head. */
  void freeRecords(DijkstraRecord* head) const {
    DijkstraRecord* cur = head;
    while (cur != nullptr) {
        DijkstraRecord* tmp = cur;
        cur = cur->next;
        delete tmp;
    }
  }

  /**
   * @brief Computes the predictive edge cost for corridor @p c.
   *
   * cost = traversal_time + capacity_penalty + fire_penalty
   *
   * Returns LLONG_MAX for corridors that are already blocked.
   *
   * @param c The corridor to evaluate.
   * @return Predictive cost, or LLONG_MAX if impassable.
   */
  long long predictiveCost(Corridor* c) const {
    if (c->blocked || c->fire_intensity >= fire_block_threshold_) {
        return std::numeric_limits<long long>::max();
    }

    long long capacity_penalty = 0;
    if (c->capacity > 0 && c->current_load >= c->capacity) {
        capacity_penalty = c->traversal_time;
    }

    long long fire_penalty = 0;
    bool endpoint_burning = (c->from->on_fire || c->to->on_fire ||
                             c->from->fire_intensity > 0.0 ||
                             c->to->fire_intensity > 0.0 ||
                             c->fire_intensity > 0.0);
    if (endpoint_burning) {
        double endpoint_max = c->from->fire_intensity;
        if (c->to->fire_intensity > endpoint_max)   endpoint_max = c->to->fire_intensity;
        if (c->fire_intensity > endpoint_max)        endpoint_max = c->fire_intensity;

        double remaining = fire_block_threshold_ - c->fire_intensity;
        double growth = fire_spread_rate_ *
                        (c->flammability > 0.01 ? c->flammability : 0.01) *
                        (endpoint_max > 0.05 ? endpoint_max : 0.05);
        double ticks_until_block = (growth > 0.0001)
                                      ? (remaining / growth)
                                      : static_cast<double>(prediction_horizon_);

        if (ticks_until_block < static_cast<double>(prediction_horizon_)) {
            double severity = static_cast<double>(prediction_horizon_) - ticks_until_block;
            fire_penalty = static_cast<long long>(
                prediction_penalty_ * severity * static_cast<double>(c->traversal_time));
            if (fire_penalty < 0) fire_penalty = 0;
        }
    }

    long long base = static_cast<long long>(c->traversal_time);
    return base + capacity_penalty + fire_penalty;
  }

  /** @brief Frees all persons, waiting queues, corridors, and nodes. */
  void clearAll() {
    Person* p = people_head_;
    while (p != nullptr) { Person* t = p; p = p->next; delete t; }
    people_head_ = people_tail_ = nullptr;

    WaitingQueue* q = queues_head_;
    while (q != nullptr) { WaitingQueue* t = q; q = q->next; delete t; }
    queues_head_ = nullptr;

    Corridor* c = corridors_head_;
    while (c != nullptr) { Corridor* t = c; c = c->next_global; delete t; }
    corridors_head_ = nullptr;

    CellNode* n = nodes_head_;
    while (n != nullptr) { CellNode* t = n; n = n->next; delete t; }
    nodes_head_ = nodes_tail_ = nullptr;

    node_count_ = corridor_count_ = person_count_ = 0;
  }

  // --------------------------------------------------------------------------
  // Private data members
  // --------------------------------------------------------------------------

  Graph* graph_;            ///< Shared graph updated whenever topology changes.

  CellNode* nodes_head_;    ///< Head of the global CellNode list.
  CellNode* nodes_tail_;    ///< Tail of the global CellNode list.
  Corridor* corridors_head_;///< Head of the global Corridor list.
  int node_count_;          ///< Total CellNodes in the simulator.
  int corridor_count_;      ///< Total Corridor directions in the simulator.

  Person* people_head_;     ///< Head of the global Person list.
  Person* people_tail_;     ///< Tail of the global Person list.
  int person_count_;        ///< Total persons created at simulation start.
  int next_person_id_;      ///< Counter for unique Person IDs.

  WaitingQueue* queues_head_; ///< Head of the per-corridor waiting-queue list.

  int time_step_;             ///< Number of ticks elapsed since simulation start.
  int evacuated_count_;       ///< Persons that have reached an exit.
  int casualty_count_;        ///< Persons that have died.

  double fire_block_threshold_;   ///< Corridor intensity at which it becomes impassable.
  double node_lethal_threshold_;  ///< Node intensity at which persons inside die.
  double fire_spread_rate_;       ///< Per-tick fire growth fraction.
  int    prediction_horizon_;     ///< Look-ahead ticks for the predictive cost model.
  double prediction_penalty_;     ///< Multiplier on the fire-proximity penalty term.
};
