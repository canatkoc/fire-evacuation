// ---------------------------------------------------------------------------
// main_gui.cpp
//
//   Person 2'nin entegrasyon noktası: Person 1'in Graph + custom DS'leri,
//   Person 3/4'ün FireSimulator motoru ve SDL2 tabanlı Renderer + Controller
//   bir araya getirilir.
//
//   Klavye kısayolları
//     SPACE  : Start / Pause toggle
//     S      : Single step
//     R      : Reset simulation
//     +/-    : Tick interval'ı azalt/artır (hız ayarı)
//     ESC    : Quit
//
//   Derleme (proje kökünden):
//     make gui
//     ./build/sim_gui
//   ya da elle:
//     g++ -std=c++17 -Wall -Wextra -Iinclude `pkg-config --cflags sdl2 SDL2_ttf`
//         src/main_gui.cpp `pkg-config --libs sdl2 SDL2_ttf` -o build/sim_gui
// ---------------------------------------------------------------------------

#include <SDL.h>

#include <cstdio>
#include <new>

#include "../include/data_structures/Graph.h"
#include "../include/simulation/FireSimulator.h"
#include "../include/gui/Layout.h"
#include "../include/gui/Renderer.h"
#include "../include/gui/SimulationController.h"

namespace {

// Kullanıcının yerel font yolu. Yoksa label'lar atlanır; renderlama devam eder.
constexpr const char* FONT_PATH =
    "/System/Library/Fonts/Supplemental/Arial.ttf";

void buildSampleScene(FireSimulator& sim, Layout& layout) {
    // Düğümler
    sim.addNode("ROOM_A",  Graph::NodeType::Room,      0, 4, 0.4);
    sim.addNode("ROOM_B",  Graph::NodeType::Room,      0, 3, 0.5);
    sim.addNode("ROOM_C",  Graph::NodeType::Room,      0, 3, 0.6);
    sim.addNode("HALL_1",  Graph::NodeType::Hallway,   0, 0, 0.3);
    sim.addNode("HALL_2",  Graph::NodeType::Hallway,   0, 0, 0.3);
    sim.addNode("STAIR_S", Graph::NodeType::Stairwell, 0, 0, 0.2);
    sim.addNode("EXIT_N",  Graph::NodeType::Exit,      0, 0, 0.0);
    sim.addNode("EXIT_S",  Graph::NodeType::Exit,      0, 0, 0.0);

    // Koridorlar
    sim.addCorridor("ROOM_A",  "HALL_1",  2, 2, 0.3);
    sim.addCorridor("HALL_1",  "HALL_2",  3, 2, 0.3);
    sim.addCorridor("HALL_2",  "EXIT_N",  2, 3, 0.2);
    sim.addCorridor("HALL_1",  "ROOM_B",  2, 2, 0.5);
    sim.addCorridor("HALL_2",  "ROOM_C",  2, 2, 0.5);
    sim.addCorridor("ROOM_A",  "STAIR_S", 3, 1, 0.4);
    sim.addCorridor("STAIR_S", "EXIT_S",  2, 2, 0.2);

    // Yangın
    sim.igniteNode("ROOM_C", 1.0);

    // Ekran düzeni (px)
    layout.set("ROOM_A",  170, 260);
    layout.set("HALL_1",  340, 260);
    layout.set("HALL_2",  540, 260);
    layout.set("EXIT_N",  720, 260);
    layout.set("ROOM_B",  340, 420);
    layout.set("ROOM_C",  540, 420);
    layout.set("STAIR_S", 170, 420);
    layout.set("EXIT_S",  170, 540);
}

struct UI {
    ButtonRect start  = { 20, 20, 110, 36, "Start"  };
    ButtonRect pause  = {140, 20, 110, 36, "Pause"  };
    ButtonRect step   = {260, 20, 110, 36, "Step"   };
    ButtonRect reset  = {380, 20, 110, 36, "Reset"  };
};

}  // namespace

int main() {
    Graph graph;
    FireSimulator sim(graph);
    Layout layout;
    buildSampleScene(sim, layout);

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
        // ====== Olaylar ======
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
                        if (ui.start.isInside(e.button.x, e.button.y)) controller.start();
                        else if (ui.pause.isInside(e.button.x, e.button.y)) controller.togglePause();
                        else if (ui.step.isInside(e.button.x, e.button.y))  controller.singleStep();
                        else if (ui.reset.isInside(e.button.x, e.button.y)) {
                            // Sahneyi yeniden kur: once destruct, sonra graph clear,
                            // sonra placement-new ile aynı stack adresinde yeniden inşa.
                            sim.~FireSimulator();
                            graph.clear();
                            new (&sim) FireSimulator(graph);
                            buildSampleScene(sim, layout);
                            controller.resetController();
                        }
                    }
                    break;

                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_SPACE:    controller.togglePause(); break;
                        case SDLK_s:        controller.singleStep();  break;
                        case SDLK_r:
                            sim.~FireSimulator();
                            graph.clear();
                            new (&sim) FireSimulator(graph);
                            buildSampleScene(sim, layout);
                            controller.resetController();
                            break;
                        case SDLK_PLUS:
                        case SDLK_EQUALS:
                            controller.setTickInterval(controller.tickInterval() * 0.8);
                            break;
                        case SDLK_MINUS:
                            controller.setTickInterval(controller.tickInterval() * 1.25);
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

        // ====== Tick ======
        Uint32 now = SDL_GetTicks();
        double dt = static_cast<double>(now - last_ticks) / 1000.0;
        last_ticks = now;
        controller.update(dt);

        // ====== Render ======
        renderer.clearFrame();

        renderer.drawScene(sim, layout);

        const auto state = controller.state();
        renderer.drawButton(ui.start, state == SimulationController::State::Running, ui.start.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.pause, state == SimulationController::State::Paused,  ui.pause.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.step,  false, ui.step.isInside(mouse_x, mouse_y));
        renderer.drawButton(ui.reset, false, ui.reset.isInside(mouse_x, mouse_y));

        char status[160];
        std::snprintf(status, sizeof(status),
                      "[%s]  tick=%.2fs  (SPACE play/pause, S step, R reset, +/- speed)",
                      controller.stateLabel(), controller.tickInterval());
        renderer.drawText(status, 20, 70, 200, 200, 220);

        renderer.drawHud(sim, 20, 90);

        renderer.present();
    }

    return 0;
}
