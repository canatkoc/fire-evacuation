/**
 * @file sim_demo.cpp
 * @brief Command-line entry point for the Fire Evacuation simulator.
 *
 * Loads a building scene from a JSON file in the @c data/ directory, prints
 * the initial evacuation plans for every room, then runs the time-step
 * simulation until it finishes. The scene is read from disk so the project
 * satisfies the "read input from data/" requirement.
 *
 * Usage:
 *   ./build/sim_demo [input_file]
 * If no file is given, data/input_default.json is used.
 */

#include <iostream>
#include <string>

#include "../include/data_structures/Graph.h"
#include "../include/io/JsonSceneParser.h"
#include "../include/io/SceneData.h"
#include "../include/io/SceneLoader.h"
#include "../include/simulation/FireSimulator.h"

namespace {

/// Prints a single evacuation path (or an unreachable marker).
void printPath(const FireSimulator::EvacPath& path) {
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

/// Prints the per-tick population snapshot.
void printTick(const FireSimulator& sim) {
    int t, alive, evac, dead;
    sim.snapshot(t, alive, evac, dead);
    std::cout << "[t=" << t << "]  alive=" << alive
              << "  evac=" << evac << "  dead=" << dead << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string input_file = "data/input_default.json";
    if (argc > 1) {
        input_file = argv[1];
    }

    // Parse once; the SceneData lets us enumerate rooms for planning output.
    SceneData scene;
    try {
        JsonSceneParser::loadFromFile(input_file, scene);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    Graph g;
    FireSimulator sim(g);
    SceneLoader::apply(scene, sim);

    std::cout << "=== SCENE LOADED FROM: " << input_file << " ===\n";
    std::cout << "nodes=" << sim.nodeCount()
              << "  corridors=" << sim.corridorCount()
              << "  people=" << sim.totalPeople() << "\n\n";

    std::cout << "=== INITIAL EVAC PLANS ===\n";
    for (SceneNode* n = scene.nodes; n != nullptr; n = n->next) {
        if (n->type == "room" && n->occupants > 0) {
            std::cout << n->id << " ->\n";
            printPath(sim.findSafestExit(n->id));
        }
    }

    std::cout << "\n=== SIMULATION ===\n";
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
