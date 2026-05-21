#pragma once

#include <string>
#include <limits>
#include <cmath>

#include "../data_structures/LinkedList.h"
#include "../data_structures/Queue.h"
#include "../data_structures/priority_queue.h"
#include "../data_structures/Graph.h"
#include "SimEntities.h"

// ---------------------------------------------------------------------------
// FireSimulator
//   - Task 3 (Dinamik Rota Planlayıcı): custom Dijkstra over CellNode/Corridor
//     using Person 1's PriorityQueue, with predictive cost (fire proximity)
//     and capacity-aware penalties.
//   - Task 4 (Yangın Yayılımı): per-tick spread from origin nodes through
//     corridors based on flammability. Edges become "blocked" when intensity
//     crosses threshold (propagated back into Person 1's Graph).
//   - Ortak: bottleneck queueing (Queue<Person*>) and casualty tracking.
//
// Hiçbir STL container kullanılmıyor: yalnızca LinkedList / Queue /
// PriorityQueue / Graph + intrusive pointer listeleri.
// ---------------------------------------------------------------------------

class FireSimulator {
   public:
    struct EvacPath {
        bool reachable;
        long long total_cost;
        LinkedList<std::string> node_sequence;   // start ... exit

        EvacPath() : reachable(false), total_cost(0), node_sequence() {}
    };

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

    ~FireSimulator() { clearAll(); }

    FireSimulator(const FireSimulator&) = delete;
    FireSimulator& operator=(const FireSimulator&) = delete;

    // ===================== SETUP =====================

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

    bool igniteNode(const std::string& id, double initial_intensity = 1.0) {
        CellNode* n = findNode(id);
        if (n == nullptr) return false;
        n->on_fire = true;
        n->fire_intensity = clamp01(initial_intensity);
        return true;
    }

    // ===================== PER-TICK CORE =====================

    void step() {
        propagateFire();        // 1) yangın yayılımı
        processCasualties();    // 2) yangının yuttuğu insanları zayiat say
        releaseStaleQueues();   // 3) bloke olmuş koridorlardaki kuyrukları boşalt
        stepPeople();           // 4) hareket + rota + kuyruk
        ++time_step_;
    }

    void run(int max_steps) {
        int guard = 0;
        while (guard < max_steps && !simulationFinished()) {
            step();
            ++guard;
        }
        // Adım limiti aşıldı: hayatta kalan herkes ulaşamayan = zayiat
        Person* p = people_head_;
        while (p != nullptr) {
            if (!p->evacuated && !p->dead) {
                p->dead = true;
                ++casualty_count_;
            }
            p = p->next;
        }
    }

    // ===================== TASK 4: YANGIN YAYILIMI =====================

    void propagateFire() {
        // Faz 1: Koridorlar uç düğümlerinden yangın "absorb" eder.
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

        // Faz 2: Düğümler, kendilerine GELEN koridorların yangınından tutuşur.
        // Hesap-uygula sırasını ayırmak için geçici intrusive liste.
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
                // Tamamen yanan düğüm => hem giren hem çıkan tüm koridorlar bloke
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

    // ===================== TASK 3: DİNAMİK ROTA =====================

    // En güvenli/çıkışa-en-yakın yolu, predictive cost ile bulan custom Dijkstra.
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

    // ===================== ORTAK: HAREKET + KUYRUK =====================

    void stepPeople() {
        // -- Faz A: koridor üzerindeki insanları ilerlet, varanları yerleştir
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

        // -- Faz B: bekleme kuyruklarını boşalt (kapasite uygunsa hareket başlat)
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

        // -- Faz C: boştaki insanlara dinamik rota seç
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
            if (!plan.reachable) {
                // Şu an çıkış yok; sonraki tick'te tekrar dene. Hâlâ ulaşılamıyorsa
                // yangın gelene kadar bekler ve casualty olur.
                q = q->next; continue;
            }

            // Yolun ikinci elemanını (next hop) al
            LinkedList<std::string> seq = plan.node_sequence;
            if (seq.get_size() < 2) { q = q->next; continue; }
            seq.pop_front();
            std::string next_id = seq.pop_front();

            CellNode* nxt = findNode(next_id);
            if (nxt == nullptr) { q = q->next; continue; }

            Corridor* c = findCorridor(here, nxt);
            if (c == nullptr || c->blocked) { q = q->next; continue; }

            if (c->capacity > 0 && c->current_load >= c->capacity) {
                // BOTTLENECK: kuyruğa al
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

    void releaseStaleQueues() {
        // Koridor bloklandıysa kuyruktakileri serbest bırak (bir sonraki tick'te
        // yeni rota arayacaklar).
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

    // ===================== STATE / REPORT =====================

    bool simulationFinished() const {
        Person* p = people_head_;
        while (p != nullptr) {
            if (!p->evacuated && !p->dead) return false;
            p = p->next;
        }
        return true;
    }

    int currentTime() const { return time_step_; }
    int evacuatedCount() const { return evacuated_count_; }
    int casualtyCount() const { return casualty_count_; }
    int totalPeople() const { return person_count_; }
    int nodeCount() const { return node_count_; }
    int corridorCount() const { return corridor_count_; }

    // Düğüm-bazlı yangın yoğunluğunu dış kod (örn. GUI) okuyabilsin diye.
    double fireIntensityAt(const std::string& node_id) const {
        CellNode* n = findNode(node_id);
        return (n == nullptr) ? 0.0 : n->fire_intensity;
    }
    bool isCorridorBlocked(const std::string& from_id, const std::string& to_id) const {
        CellNode* a = findNode(from_id);
        CellNode* b = findNode(to_id);
        Corridor* c = (a && b) ? findCorridor(a, b) : nullptr;
        return (c == nullptr) ? true : c->blocked;
    }

    // Anlık özet (iostream gerektirmez): basit getters üzerinden.
    void snapshot(int& out_time, int& out_alive, int& out_evac, int& out_dead) const {
        out_time = time_step_;
        out_evac = evacuated_count_;
        out_dead = casualty_count_;
        out_alive = person_count_ - evacuated_count_ - casualty_count_;
    }

    // Tuning hooks (varsayılanlar makul; istersen değiştir).
    void setFireSpreadRate(double r)        { fire_spread_rate_ = r; }
    void setFireBlockThreshold(double t)    { fire_block_threshold_ = clamp01(t); }
    void setNodeLethalThreshold(double t)   { node_lethal_threshold_ = clamp01(t); }
    void setPredictionHorizon(int h)        { prediction_horizon_ = (h < 1) ? 1 : h; }
    void setPredictionPenalty(double k)     { prediction_penalty_ = (k < 0.0) ? 0.0 : k; }

    // İterasyon yardımcısı (GUI/raporlama için): tüm Person'ları read-only yürür.
    // STL container yok; intrusive listenin head pointer'ı dönülür.
    const Person* peopleHead() const { return people_head_; }
    const CellNode* nodesHead() const { return nodes_head_; }
    const Corridor* corridorsHead() const { return corridors_head_; }

   private:
    // -------- Dijkstra altyapısı --------
    struct DijkstraRecord {
        CellNode* node;
        long long distance;
        CellNode* previous;
        bool visited;
        DijkstraRecord* next;

        explicit DijkstraRecord(CellNode* n)
            : node(n),
              distance(std::numeric_limits<long long>::max()),
              previous(nullptr),
              visited(false),
              next(nullptr) {}
    };

    struct DjItem {
        CellNode* node;
        long long distance;
    };

    struct DjCompare {
        bool operator()(const DjItem& a, const DjItem& b) const {
            return a.distance < b.distance;   // min-heap
        }
    };

    // -------- Helpers --------
    static double clamp01(double x) {
        if (x < 0.0) return 0.0;
        if (x > 1.0) return 1.0;
        return x;
    }

    CellNode* findNode(const std::string& id) const {
        CellNode* cur = nodes_head_;
        while (cur != nullptr) {
            if (cur->id == id) return cur;
            cur = cur->next;
        }
        return nullptr;
    }

    Corridor* findCorridor(CellNode* from, CellNode* to) const {
        Corridor* cur = corridors_head_;
        while (cur != nullptr) {
            if (cur->from == from && cur->to == to) return cur;
            cur = cur->next_global;
        }
        return nullptr;
    }

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

    DijkstraRecord* buildRecords() const {
        DijkstraRecord* head = nullptr;
        DijkstraRecord* tail = nullptr;
        CellNode* n = nodes_head_;
        while (n != nullptr) {
            DijkstraRecord* r = new DijkstraRecord(n);
            if (head == nullptr) {
                head = r; tail = r;
            } else {
                tail->next = r; tail = r;
            }
            n = n->next;
        }
        return head;
    }

    DijkstraRecord* recFor(DijkstraRecord* head, CellNode* n) const {
        DijkstraRecord* cur = head;
        while (cur != nullptr) {
            if (cur->node == n) return cur;
            cur = cur->next;
        }
        return nullptr;
    }

    void freeRecords(DijkstraRecord* head) const {
        DijkstraRecord* cur = head;
        while (cur != nullptr) {
            DijkstraRecord* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
    }

    // Predictive routing maliyeti:
    //   base = traversal_time
    // + capacity_penalty (load >= capacity ise +traversal_time)
    // + fire_penalty     (yakında yanacak koridorlar pahalı)
    long long predictiveCost(Corridor* c) const {
        if (c->blocked || c->fire_intensity >= fire_block_threshold_) {
            return std::numeric_limits<long long>::max();
        }

        long long capacity_penalty = 0;
        if (c->capacity > 0 && c->current_load >= c->capacity) {
            capacity_penalty = c->traversal_time;       // ortalama bir bekleme
        }

        long long fire_penalty = 0;
        bool endpoint_burning = (c->from->on_fire || c->to->on_fire ||
                                 c->from->fire_intensity > 0.0 ||
                                 c->to->fire_intensity > 0.0 ||
                                 c->fire_intensity > 0.0);
        if (endpoint_burning) {
            double endpoint_max = c->from->fire_intensity;
            if (c->to->fire_intensity > endpoint_max) endpoint_max = c->to->fire_intensity;
            if (c->fire_intensity > endpoint_max)    endpoint_max = c->fire_intensity;

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

    void clearAll() {
        Person* p = people_head_;
        while (p != nullptr) {
            Person* t = p; p = p->next; delete t;
        }
        people_head_ = people_tail_ = nullptr;

        WaitingQueue* q = queues_head_;
        while (q != nullptr) {
            WaitingQueue* t = q; q = q->next; delete t;
        }
        queues_head_ = nullptr;

        Corridor* c = corridors_head_;
        while (c != nullptr) {
            Corridor* t = c; c = c->next_global; delete t;
        }
        corridors_head_ = nullptr;

        CellNode* n = nodes_head_;
        while (n != nullptr) {
            CellNode* t = n; n = n->next; delete t;
        }
        nodes_head_ = nodes_tail_ = nullptr;

        node_count_ = corridor_count_ = person_count_ = 0;
    }

    // ===================== STATE =====================
    Graph* graph_;

    CellNode* nodes_head_;
    CellNode* nodes_tail_;
    Corridor* corridors_head_;
    int node_count_;
    int corridor_count_;

    Person* people_head_;
    Person* people_tail_;
    int person_count_;
    int next_person_id_;

    WaitingQueue* queues_head_;

    int time_step_;
    int evacuated_count_;
    int casualty_count_;

    double fire_block_threshold_;
    double node_lethal_threshold_;
    double fire_spread_rate_;
    int    prediction_horizon_;
    double prediction_penalty_;
};
