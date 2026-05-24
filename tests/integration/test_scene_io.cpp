/**
 * @file test_scene_io.cpp
 * @brief Unit and integration tests for the data/ input pipeline.
 *
 * Covers JsonSceneParser (string + file parsing, edge cases, malformed input)
 * and SceneLoader (parsed scene -> live FireSimulator).
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "data_structures/Graph.h"
#include "io/JsonSceneParser.h"
#include "io/SceneData.h"
#include "io/SceneLoader.h"
#include "simulation/FireSimulator.h"

namespace {

void printTestPassed(const std::string& test_name) {
    std::cout << test_name << " ........ PASSED\n";
}

void runAndReport(const std::string& test_name, void (*test_func)()) {
    test_func();
    printTestPassed(test_name);
}

/// Counts the nodes in a parsed scene's intrusive list.
int countNodes(const SceneData& s) {
    int n = 0;
    for (SceneNode* p = s.nodes; p != nullptr; p = p->next) ++n;
    return n;
}

/// Counts the edges in a parsed scene's intrusive list.
int countEdges(const SceneData& s) {
    int n = 0;
    for (SceneEdge* p = s.edges; p != nullptr; p = p->next) ++n;
    return n;
}

// --------------------------------------------------------------------
// Parser unit tests
// --------------------------------------------------------------------

void test_parse_minimal_scene_from_string() {
    const char* json =
        "{ \"building\": {\"floors\": 1, \"fire_origin\": \"R1\"},"
        "  \"nodes\": [ {\"id\":\"R1\",\"type\":\"room\",\"floor\":1,"
        "                \"occupants\":2,\"flammability\":0.5} ],"
        "  \"edges\": [],"
        "  \"fire_spread_rate\": 0.3 }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);

    assert(scene.floors == 1);
    assert(scene.fire_origin == "R1");
    assert(countNodes(scene) == 1);
    assert(countEdges(scene) == 0);
    assert(scene.nodes->id == "R1");
    assert(scene.nodes->occupants == 2);
}

void test_parse_nodes_and_edges_counts() {
    const char* json =
        "{ \"nodes\": ["
        "    {\"id\":\"A\",\"type\":\"room\",\"occupants\":1},"
        "    {\"id\":\"B\",\"type\":\"hallway\"},"
        "    {\"id\":\"E\",\"type\":\"exit\"} ],"
        "  \"edges\": ["
        "    {\"from\":\"A\",\"to\":\"B\",\"capacity\":2,\"traversal_time\":1},"
        "    {\"from\":\"B\",\"to\":\"E\",\"capacity\":3,\"traversal_time\":2} ] }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);

    assert(countNodes(scene) == 3);
    assert(countEdges(scene) == 2);
    assert(scene.edges->from == "A" && scene.edges->to == "B");
    assert(scene.edges->capacity == 2);
}

void test_parse_optional_coordinates() {
    const char* json =
        "{ \"nodes\": ["
        "  {\"id\":\"A\",\"type\":\"room\",\"x\":100,\"y\":200},"
        "  {\"id\":\"B\",\"type\":\"exit\"} ], \"edges\": [] }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);

    assert(scene.nodes->hasCoordinates());
    assert(scene.nodes->x == 100 && scene.nodes->y == 200);
    assert(!scene.nodes->next->hasCoordinates());
}

void test_parse_empty_scene() {
    const char* json = "{ \"nodes\": [], \"edges\": [] }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);
    assert(countNodes(scene) == 0);
    assert(countEdges(scene) == 0);
}

void test_parse_malformed_input_throws() {
    bool threw = false;
    try {
        SceneData scene;
        JsonSceneParser::loadFromString("{ \"nodes\": [ {\"id\": ", scene);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_parse_missing_file_throws() {
    bool threw = false;
    try {
        SceneData scene;
        JsonSceneParser::loadFromFile("data/__no_such_file__.json", scene);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

void test_parser_ignores_unknown_keys() {
    const char* json =
        "{ \"unknown_block\": {\"a\":1,\"b\":[1,2,3]},"
        "  \"nodes\": [ {\"id\":\"A\",\"type\":\"exit\",\"extra\":true} ],"
        "  \"edges\": [] }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);
    assert(countNodes(scene) == 1);
    assert(scene.nodes->id == "A");
}

// --------------------------------------------------------------------
// Loader integration tests
// --------------------------------------------------------------------

void test_loader_populates_simulator() {
    const char* json =
        "{ \"building\": {\"floors\":1,\"fire_origin\":\"R1\"},"
        "  \"nodes\": ["
        "    {\"id\":\"R1\",\"type\":\"room\",\"occupants\":3},"
        "    {\"id\":\"H1\",\"type\":\"hallway\"},"
        "    {\"id\":\"E1\",\"type\":\"exit\"} ],"
        "  \"edges\": ["
        "    {\"from\":\"R1\",\"to\":\"H1\",\"capacity\":2,\"traversal_time\":1},"
        "    {\"from\":\"H1\",\"to\":\"E1\",\"capacity\":2,\"traversal_time\":1} ],"
        "  \"fire_spread_rate\": 0.2 }";
    SceneData scene;
    JsonSceneParser::loadFromString(json, scene);

    Graph g;
    FireSimulator sim(g);
    SceneLoader::apply(scene, sim);

    assert(sim.nodeCount() == 3);
    assert(sim.totalPeople() == 3);
    assert(sim.corridorCount() >= 2);
}

void test_loader_node_type_mapping() {
    assert(SceneLoader::toNodeType("exit") == Graph::NodeType::Exit);
    assert(SceneLoader::toNodeType("room") == Graph::NodeType::Room);
    assert(SceneLoader::toNodeType("hallway") == Graph::NodeType::Hallway);
    assert(SceneLoader::toNodeType("stairwell") == Graph::NodeType::Stairwell);
    assert(SceneLoader::toNodeType("???") == Graph::NodeType::Room);
}

void test_loader_default_file_runs_end_to_end() {
    Graph g;
    FireSimulator sim(g);
    bool ok = true;
    try {
        SceneLoader::loadInto("data/input_default.json", sim);
    } catch (const std::exception&) {
        ok = false;
    }
    assert(ok);
    assert(sim.nodeCount() == 8);
    assert(sim.totalPeople() == 10);

    // The scene should make progress and finish within a tick budget.
    for (int i = 0; i < 300 && !sim.simulationFinished(); ++i) {
        sim.step();
    }
    int t, alive, evac, dead;
    sim.snapshot(t, alive, evac, dead);
    assert(evac + dead == sim.totalPeople());
}

}  // namespace

/// Entry point invoked by the global test runner.
void run_scene_io_tests() {
    std::cout << "\n[UNIT] JsonSceneParser\n";
    runAndReport("test_parse_minimal_scene_from_string",
                 test_parse_minimal_scene_from_string);
    runAndReport("test_parse_nodes_and_edges_counts",
                 test_parse_nodes_and_edges_counts);
    runAndReport("test_parse_optional_coordinates",
                 test_parse_optional_coordinates);
    runAndReport("test_parse_empty_scene", test_parse_empty_scene);
    runAndReport("test_parse_malformed_input_throws",
                 test_parse_malformed_input_throws);
    runAndReport("test_parse_missing_file_throws",
                 test_parse_missing_file_throws);
    runAndReport("test_parser_ignores_unknown_keys",
                 test_parser_ignores_unknown_keys);

    std::cout << "\n[INTEGRATION] SceneLoader\n";
    runAndReport("test_loader_populates_simulator",
                 test_loader_populates_simulator);
    runAndReport("test_loader_node_type_mapping",
                 test_loader_node_type_mapping);
    runAndReport("test_loader_default_file_runs_end_to_end",
                 test_loader_default_file_runs_end_to_end);
}
