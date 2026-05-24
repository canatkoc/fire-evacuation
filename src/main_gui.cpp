/**
 * @file main_gui.cpp
 * @brief SDL2 graphical frontend for the Fire Evacuation simulator.
 *
 * Wires together the custom data structures + Graph, the FireSimulator engine
 * and the SDL2-based Renderer / SimulationController. The building scene is
 * loaded from a JSON file under @c data/ (no longer hardcoded).
 *
 * Keyboard shortcuts:
 *   SPACE  : Start / Pause toggle
 *   S      : Single step
 *   R      : Reset simulation
 *   +/-    : Decrease / increase tick interval (speed)
 *   ESC    : Quit
 *
 * Usage:
 *   make gui && ./build/sim_gui [input_file]
 * If no file is given, data/input_default.json is used.
 */

#include <SDL.h>

#include <cstdio>
#include <cstring>
#include <new>
#include <string>

#include "../include/data_structures/Graph.h"
#include "../include/gui/Layout.h"
#include "../include/gui/Renderer.h"
#include "../include/gui/SimulationController.h"
#include "../include/io/JsonSceneParser.h"
#include "../include/io/SceneData.h"
#include "../include/io/SceneLoader.h"
#include "../include/simulation/FireSimulator.h"

namespace {

/// Local font path. If absent, labels are skipped and rendering continues.
constexpr const char* FONT_PATH =
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

/// Default scene file used when no command-line argument is given.
constexpr const char* DEFAULT_SCENE = "data/input_default.json";

/// On-screen control buttons.
struct UI {
    ButtonRect start = {20, 20, 110, 36, "Start"};
    ButtonRect pause = {140, 20, 110, 36, "Pause"};
    ButtonRect step  = {260, 20, 110, 36, "Step"};
    ButtonRect reset = {380, 20, 110, 36, "Reset"};
};

}  // namespace

int main(int argc, char* argv[]) {
    const std::string scene_path =
        (argc > 1) ? std::string(argv[1]) : std::string(DEFAULT_SCENE);

    // Parse the scene once; reused on every reset.
    SceneData scene;
    try {
        JsonSceneParser::loadFromFile(scene_path, scene);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error loading scene: %s\n", e.what());
        return 1;
    }

    Graph graph;
    FireSimulator sim(graph);
    Layout layout;
    SceneLoader::apply(scene, sim);
    SceneLoader::applyLayout(scene, layout, 960, 640);

    SimulationController controller(sim);

    Renderer renderer;
    if (!renderer.init(960, 640, "Fire Evacuation Simulator", FONT_PATH)) {
        std::fprintf(stderr, "SDL/TTF init failed\n");
        return 1;
    }

    UI ui;
    bool quit = false;
    int mouse_x = 0, mouse_y = 0;
    Uint32 last_ticks = SDL_GetTicks();

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_MOUSEMOTION:
                    mouse_x = e.motion.x;
                    mouse_y = e.motion.y;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (ui.start.isInside(e.button.x, e.button.y)) {
                            controller.start();
                        } else if (ui.pause.isInside(e.button.x, e.button.y)) {
                            controller.togglePause();
                        } else if (ui.step.isInside(e.button.x, e.button.y)) {
                            controller.singleStep();
                        } else if (ui.reset.isInside(e.button.x, e.button.y)) {
                            // Rebuild the scene in place from the same data.
                            sim.~FireSimulator();
                            graph.clear();
                            new (&sim) FireSimulator(graph);
                            SceneLoader::apply(scene, sim);
                            controller.resetController();
                        }
                    }
                    break;

                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_SPACE: controller.togglePause(); break;
                        case SDLK_s:     controller.singleStep();  break;
                        case SDLK_r:
                            sim.~FireSimulator();
                            graph.clear();
                            new (&sim) FireSimulator(graph);
                            SceneLoader::apply(scene, sim);
                            controller.resetController();
                            break;
                        case SDLK_PLUS:
                        case SDLK_EQUALS:
                            controller.setTickInterval(
                                controller.tickInterval() * 0.8);
                            break;
                        case SDLK_MINUS:
                            controller.setTickInterval(
                                controller.tickInterval() * 1.25);
                            break;
                        case SDLK_ESCAPE:
                            quit = true;
                            break;
                        default: break;
                    }
                    break;

                default: break;
            }
        }

        Uint32 now = SDL_GetTicks();
        double dt = static_cast<double>(now - last_ticks) / 1000.0;
        last_ticks = now;
        controller.update(dt);

        renderer.clearFrame();
        renderer.drawScene(sim, layout);

        const auto state = controller.state();
        renderer.drawButton(ui.start,
                            state == SimulationController::State::Running,
                            ui.start.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.pause,
                            state == SimulationController::State::Paused,
                            ui.pause.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.step, false,
                            ui.step.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.reset, false,
                            ui.reset.isInside(mouse_x, mouse_y));

        char status[200];
        std::snprintf(status, sizeof(status),
                      "[%s]  tick=%.2fs  scene=%s  "
                      "(SPACE play/pause, S step, R reset, +/- speed)",
                      controller.stateLabel(), controller.tickInterval(),
                      scene_path.c_str());
        renderer.drawText(status, 20, 70, 200, 200, 220);

        renderer.drawHud(sim, 20, 90);
        renderer.present();
    }

    return 0;
}
