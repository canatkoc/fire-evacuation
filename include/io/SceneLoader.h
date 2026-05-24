#pragma once

/**
 * @file SceneLoader.h
 * @brief Builds a FireSimulator (and optional GUI Layout) from a SceneData.
 *
 * Separates file parsing (@ref JsonSceneParser) from domain object
 * construction. The CLI demo and the GUI both call into this loader so the
 * scene-building logic exists in exactly one place (Single Responsibility).
 */

#include <string>

#include "../data_structures/Graph.h"
#include "../simulation/FireSimulator.h"
#include "JsonSceneParser.h"
#include "SceneData.h"

/**
 * @brief Translates parsed scene data into live simulator objects.
 */
class SceneLoader {
 public:
    /**
     * @brief Maps a textual node type to the Graph::NodeType enum.
     * @param type Lowercase type string ("room", "hallway", ...).
     * @return Corresponding enum value; Room is used as the fallback.
     */
    static Graph::NodeType toNodeType(const std::string& type) {
        if (type == "exit")      return Graph::NodeType::Exit;
        if (type == "stairwell") return Graph::NodeType::Stairwell;
        if (type == "hallway")   return Graph::NodeType::Hallway;
        if (type == "room")      return Graph::NodeType::Room;
        return Graph::NodeType::Room;
    }

    /**
     * @brief Populates a simulator from an already-parsed scene.
     *
     * @pre @p sim is freshly constructed and empty.
     * @post Every node, corridor and the fire origin from @p scene exist in
     *       @p sim; the fire spread rate is applied.
     * @param scene Parsed input data.
     * @param sim   Target simulator to fill.
     */
    static void apply(const SceneData& scene, FireSimulator& sim) {
        sim.setFireSpreadRate(scene.fire_spread_rate);

        for (SceneNode* n = scene.nodes; n != nullptr; n = n->next) {
            sim.addNode(n->id, toNodeType(n->type), n->floor,
                        n->occupants, n->flammability);
        }
        for (SceneEdge* e = scene.edges; e != nullptr; e = e->next) {
            sim.addCorridor(e->from, e->to, e->traversal_time,
                            e->capacity, e->flammability);
        }
        if (!scene.fire_origin.empty()) {
            sim.igniteNode(scene.fire_origin, 1.0);
        }
    }

    /**
     * @brief Convenience: parse a file and fill a simulator in one call.
     *
     * @param path Path to a JSON scene file under @c data/.
     * @param sim  Target simulator to fill.
     * @throws std::runtime_error if the file is missing or malformed.
     */
    static void loadInto(const std::string& path, FireSimulator& sim) {
        SceneData scene;
        JsonSceneParser::loadFromFile(path, scene);
        apply(scene, sim);
    }

    /**
     * @brief Assigns screen coordinates to every node of a parsed scene.
     *
     * Uses the explicit @c x / @c y fields from the file when present;
     * otherwise lays nodes out on an automatic grid so the GUI always has
     * usable positions regardless of the input file.
     *
     * @tparam LayoutT Any type exposing @c set(id, x, y).
     * @param scene  Parsed scene data.
     * @param layout Target layout to populate.
     * @param width  Drawable area width in pixels.
     * @param height Drawable area height in pixels.
     */
    template <typename LayoutT>
    static void applyLayout(const SceneData& scene, LayoutT& layout,
                            int width = 960, int height = 640) {
        int count = 0;
        for (SceneNode* n = scene.nodes; n != nullptr; n = n->next) ++count;
        if (count == 0) return;

        // Grid dimensions: roughly square.
        int cols = 1;
        while (cols * cols < count) ++cols;
        const int margin_x = 120;
        const int margin_y = 160;
        const int span_x = (width  - 2 * margin_x);
        const int span_y = (height - margin_y - 60);

        int index = 0;
        for (SceneNode* n = scene.nodes; n != nullptr; n = n->next, ++index) {
            if (n->hasCoordinates()) {
                layout.set(n->id, n->x, n->y);
                continue;
            }
            int row = index / cols;
            int col = index % cols;
            int rows = (count + cols - 1) / cols;
            int gx = margin_x +
                     (cols <= 1 ? span_x / 2 : col * span_x / (cols - 1));
            int gy = margin_y +
                     (rows <= 1 ? span_y / 2 : row * span_y / (rows - 1));
            layout.set(n->id, gx, gy);
        }
    }
};
