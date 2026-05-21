// ---------------------------------------------------------------------------
// sim_demo.cpp
//
// Küçük bir bina senaryosu kurar ve FireSimulator'ı çalıştırır.
// Person 1'in Graph + custom DS'leri ile çalışır; STL container yok.
//
// Derleme (proje kök dizininden):
//   g++ -std=c++17 -Wall -Wextra -Iinclude src/sim_demo.cpp -o build/sim_demo
//   ./build/sim_demo
// ---------------------------------------------------------------------------

#include <iostream>
#include <string>

#include "../include/data_structures/Graph.h"
#include "../include/simulation/FireSimulator.h"

static void printPath(const FireSimulator::EvacPath& path) {
    if (!path.reachable) {
        std::cout << "  [UNREACHABLE]\n";
        return;
    }
    std::cout << "  cost=" << path.total_cost << "  :  ";
    LinkedList<std::string> copy = path.node_sequence;
    bool first = true;
    while (!copy.is_empty()) {
        std::string n = copy.pop_front();
        if (!first) std::cout << " -> ";
        std::cout << n;
        first = false;
    }
    std::cout << "\n";
}

static void printTick(const FireSimulator& sim) {
    int t, alive, evac, dead;
    sim.snapshot(t, alive, evac, dead);
    std::cout << "[t=" << t
              << "]  alive=" << alive
              << "  evac=" << evac
              << "  dead=" << dead
              << "\n";
}

int main() {
    Graph g;
    FireSimulator sim(g);

    // ====== Bina topolojisi ======
    //
    //   ROOM_A (4 kişi) --- HALL_1 --- HALL_2 --- EXIT_N
    //         |                |          |
    //       STAIR_S       ROOM_B(3)   ROOM_C(3)
    //         |
    //       EXIT_S
    //
    // Flammability (0..1): yüksek = kolay yanan
    sim.addNode("ROOM_A",  Graph::NodeType::Room,      0, 4, /*flam*/0.4);
    sim.addNode("ROOM_B",  Graph::NodeType::Room,      0, 3, 0.5);
    sim.addNode("ROOM_C",  Graph::NodeType::Room,      0, 3, 0.6);
    sim.addNode("HALL_1",  Graph::NodeType::Hallway,   0, 0, 0.3);
    sim.addNode("HALL_2",  Graph::NodeType::Hallway,   0, 0, 0.3);
    sim.addNode("STAIR_S", Graph::NodeType::Stairwell, 0, 0, 0.2);
    sim.addNode("EXIT_N",  Graph::NodeType::Exit,      0, 0, 0.0);
    sim.addNode("EXIT_S",  Graph::NodeType::Exit,      0, 0, 0.0);

    // (from, to, traversal_time, capacity, flammability, bidirectional)
    sim.addCorridor("ROOM_A",  "HALL_1",  2, 2, 0.3);
    sim.addCorridor("HALL_1",  "HALL_2",  3, 2, 0.3);
    sim.addCorridor("HALL_2",  "EXIT_N",  2, 3, 0.2);
    sim.addCorridor("HALL_1",  "ROOM_B",  2, 2, 0.5);
    sim.addCorridor("HALL_2",  "ROOM_C",  2, 2, 0.5);
    sim.addCorridor("ROOM_A",  "STAIR_S", 3, 1, 0.4);    // kapasitesi düşük (bottleneck)
    sim.addCorridor("STAIR_S", "EXIT_S",  2, 2, 0.2);

    // ====== Yangın başlangıcı ======
    sim.igniteNode("ROOM_C", 1.0);  // ROOM_C yanıyor

    // ====== İlk rota planları ======
    std::cout << "=== INITIAL EVAC PLANS ===\n";
    std::cout << "ROOM_A ->\n"; printPath(sim.findSafestExit("ROOM_A"));
    std::cout << "ROOM_B ->\n"; printPath(sim.findSafestExit("ROOM_B"));
    std::cout << "ROOM_C ->\n"; printPath(sim.findSafestExit("ROOM_C"));
    std::cout << "\n=== SIMULATION ===\n";

    // ====== Simülasyonu çalıştır ======
    const int MAX_TICKS = 200;
    printTick(sim);
    for (int i = 0; i < MAX_TICKS && !sim.simulationFinished(); ++i) {
        sim.step();
        printTick(sim);
    }

    int t, alive, evac, dead;
    sim.snapshot(t, alive, evac, dead);
    std::cout << "\n=== FINAL ===\n"
              << "Total people : " << sim.totalPeople() << "\n"
              << "Evacuated    : " << evac << "\n"
              << "Casualties   : " << dead << "\n"
              << "Time steps   : " << t << "\n";

    return 0;
}
