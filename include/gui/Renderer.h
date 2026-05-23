#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

#include "../simulation/FireSimulator.h"
#include "../simulation/SimEntities.h"
#include "Layout.h"

// ---------------------------------------------------------------------------
// Renderer
//   SDL2 + SDL2_ttf tabanlı 2D çizim katmanı. Bina topolojisini, koridor
//   yangın yoğunluğunu, insan pozisyonlarını (kenar üzerinde lineer
//   interpolasyon dahil) ve UI butonlarını çizer.
//
//   STL container kullanılmaz: SDL ham C API'si üzerinden çalışır.
// ---------------------------------------------------------------------------

struct ButtonRect {
    int x;
    int y;
    int w;
    int h;
    const char* label;

    bool isInside(int px, int py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class Renderer {
   public:
    Renderer()
        : window_(nullptr),
          sdl_renderer_(nullptr),
          font_(nullptr),
          window_w_(0),
          window_h_(0) {}

    ~Renderer() { shutdown(); }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(int width, int height, const char* title, const char* font_path) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return false;
        if (TTF_Init() != 0) {
            SDL_Quit();
            return false;
        }

        window_w_ = width;
        window_h_ = height;

        window_ = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   width, height,
                                   SDL_WINDOW_SHOWN);
        if (window_ == nullptr) {
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        sdl_renderer_ = SDL_CreateRenderer(window_, -1,
                                           SDL_RENDERER_ACCELERATED |
                                           SDL_RENDERER_PRESENTVSYNC);
        if (sdl_renderer_ == nullptr) {
            SDL_DestroyWindow(window_);
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        if (font_path != nullptr) {
            font_ = TTF_OpenFont(font_path, 14);
            // Font opsiyoneldir: yoksa labellar atlanır.
        }
        return true;
    }

    void shutdown() {
        if (font_ != nullptr)        { TTF_CloseFont(font_); font_ = nullptr; }
        if (sdl_renderer_ != nullptr) { SDL_DestroyRenderer(sdl_renderer_); sdl_renderer_ = nullptr; }
        if (window_ != nullptr)       { SDL_DestroyWindow(window_); window_ = nullptr; }
        TTF_Quit();
        SDL_Quit();
    }

    void clearFrame() {
        SDL_SetRenderDrawColor(sdl_renderer_, 18, 18, 24, 255);
        SDL_RenderClear(sdl_renderer_);
    }

    void present() { SDL_RenderPresent(sdl_renderer_); }

    int windowW() const { return window_w_; }
    int windowH() const { return window_h_; }

    // ===== Sahne =====

    void drawScene(const FireSimulator& sim, const Layout& layout) {
        drawCorridors(sim, layout);
        drawNodes(sim, layout);
        drawPeople(sim, layout);
    }

    // ===== UI =====

    void drawButton(const ButtonRect& btn, bool active, bool hover) {
        SDL_Rect r{btn.x, btn.y, btn.w, btn.h};
        if (active) {
            SDL_SetRenderDrawColor(sdl_renderer_, 90, 160, 90, 255);
        } else if (hover) {
            SDL_SetRenderDrawColor(sdl_renderer_, 80, 80, 110, 255);
        } else {
            SDL_SetRenderDrawColor(sdl_renderer_, 60, 60, 80, 255);
        }
        SDL_RenderFillRect(sdl_renderer_, &r);

        SDL_SetRenderDrawColor(sdl_renderer_, 200, 200, 220, 255);
        SDL_RenderDrawRect(sdl_renderer_, &r);

        if (font_ != nullptr && btn.label != nullptr) {
            drawText(btn.label, btn.x + 10, btn.y + (btn.h - 18) / 2,
                     230, 230, 240);
        }
    }

    void drawHud(const FireSimulator& sim, int x, int y) {
        if (font_ == nullptr) return;

        int t, alive, evac, dead;
        sim.snapshot(t, alive, evac, dead);

        char line[128];
        std::snprintf(line, sizeof(line), "t=%d  alive=%d  evac=%d  dead=%d",
                      t, alive, evac, dead);
        drawText(line, x, y, 230, 230, 240);
    }

    void drawText(const char* text, int x, int y, int r, int g, int b) {
        if (font_ == nullptr || text == nullptr) return;
        SDL_Color color{(Uint8)r, (Uint8)g, (Uint8)b, 255};
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font_, text, color);
        if (surf == nullptr) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(sdl_renderer_, surf);
        SDL_Rect dst{x, y, surf->w, surf->h};
        SDL_FreeSurface(surf);
        if (tex != nullptr) {
            SDL_RenderCopy(sdl_renderer_, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
    }

   private:
    void drawCorridors(const FireSimulator& sim, const Layout& layout) {
        const Corridor* c = sim.corridorsHead();
        while (c != nullptr) {
            // Çift yönlü topoloji: aynı kenarı iki kez çizmemek için
            // yalnız adress sıralaması tutarsa atlamak yerine yarı saydam
            // çizip üst üste bindirmeye izin veriyoruz. Performansa zararsız.
            int x1, y1, x2, y2;
            if (layout.get(c->from->id, x1, y1) &&
                layout.get(c->to->id, x2, y2)) {
                if (c->blocked) {
                    SDL_SetRenderDrawColor(sdl_renderer_, 90, 30, 30, 255);
                } else {
                    const Uint8 r = (Uint8)(120 + 120 * c->fire_intensity);
                    const Uint8 g = (Uint8)(120 - 80  * c->fire_intensity);
                    const Uint8 b = (Uint8)(120 - 80  * c->fire_intensity);
                    SDL_SetRenderDrawColor(sdl_renderer_, r, g, b, 255);
                }
                // Kalın çizgi: paralel iki-üç çizgi
                for (int off = -1; off <= 1; ++off) {
                    SDL_RenderDrawLine(sdl_renderer_,
                                       x1, y1 + off, x2, y2 + off);
                    SDL_RenderDrawLine(sdl_renderer_,
                                       x1 + off, y1, x2 + off, y2);
                }
            }
            c = c->next_global;
        }
    }

    void drawNodes(const FireSimulator& sim, const Layout& layout) {
        const CellNode* n = sim.nodesHead();
        while (n != nullptr) {
            int x, y;
            if (layout.get(n->id, x, y)) {
                SDL_Rect r{x - 26, y - 18, 52, 36};
                fillNode(r, n);
                drawNodeBorder(r);
                if (font_ != nullptr) {
                    drawText(n->id.c_str(), r.x + 4, r.y - 16, 230, 230, 240);
                }
            }
            n = n->next;
        }
    }

    void fillNode(const SDL_Rect& r, const CellNode* n) {
        Uint8 base_r = 70, base_g = 110, base_b = 180;
        switch (n->type) {
            case Graph::NodeType::Room:      base_r=70;  base_g=110; base_b=180; break;
            case Graph::NodeType::Hallway:   base_r=120; base_g=120; base_b=130; break;
            case Graph::NodeType::Stairwell: base_r=210; base_g=180; base_b=80;  break;
            case Graph::NodeType::Exit:      base_r=60;  base_g=180; base_b=90;  break;
            default:                          base_r=100; base_g=100; base_b=100; break;
        }
        // Yangın overlay (kırmızı bias)
        double t = n->fire_intensity;
        Uint8 rr = (Uint8)(base_r * (1.0 - t) + 230 * t);
        Uint8 gg = (Uint8)(base_g * (1.0 - t) +  60 * t);
        Uint8 bb = (Uint8)(base_b * (1.0 - t) +  30 * t);
        SDL_SetRenderDrawColor(sdl_renderer_, rr, gg, bb, 255);
        SDL_RenderFillRect(sdl_renderer_, &r);
    }

    void drawNodeBorder(const SDL_Rect& r) {
        SDL_SetRenderDrawColor(sdl_renderer_, 220, 220, 235, 255);
        SDL_RenderDrawRect(sdl_renderer_, &r);
    }

    void drawPeople(const FireSimulator& sim, const Layout& layout) {
        const Person* p = sim.peopleHead();
        while (p != nullptr) {
            if (!p->dead && !p->evacuated) {
                int cx = 0, cy = 0;
                if (computePersonScreenPos(p, layout, cx, cy)) {
                    SDL_SetRenderDrawColor(sdl_renderer_, 245, 245, 250, 255);
                    fillCircle(cx, cy, 4);
                }
            }
            p = p->next;
        }
    }

    bool computePersonScreenPos(const Person* p, const Layout& layout,
                                int& out_x, int& out_y) {
        int sx, sy;
        if (!layout.get(p->current_location, sx, sy)) return false;

        if (p->on_edge) {
            int tx, ty;
            if (!layout.get(p->next_location, tx, ty)) {
                out_x = sx; out_y = sy; return true;
            }
            double frac = (p->edge_traversal_time > 0)
                              ? static_cast<double>(p->progress_on_edge) /
                                static_cast<double>(p->edge_traversal_time)
                              : 0.0;
            if (frac < 0.0) frac = 0.0;
            if (frac > 1.0) frac = 1.0;
            out_x = static_cast<int>(sx + (tx - sx) * frac);
            out_y = static_cast<int>(sy + (ty - sy) * frac);
        } else {
            out_x = sx;
            out_y = sy;
        }
        return true;
    }

    void fillCircle(int cx, int cy, int radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (dx * dx + dy * dy <= radius * radius) {
                    SDL_RenderDrawPoint(sdl_renderer_, cx + dx, cy + dy);
                }
            }
        }
    }

    SDL_Window* window_;
    SDL_Renderer* sdl_renderer_;
    TTF_Font* font_;
    int window_w_;
    int window_h_;
};
