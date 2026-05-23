#pragma once

#include <string>

// ---------------------------------------------------------------------------
// Layout
//   Düğüm ID'lerine ekran koordinatlarını (px, py) atayan basit intrusive
//   linked-list haritası. STL container yok.
// ---------------------------------------------------------------------------

class Layout {
   public:
    struct Pos {
        std::string id;
        int x;
        int y;
        Pos* next;

        Pos(const std::string& node_id, int px, int py, Pos* nxt = nullptr)
            : id(node_id), x(px), y(py), next(nxt) {}
    };

    Layout() : head_(nullptr) {}
    ~Layout() { clear(); }

    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;

    void set(const std::string& id, int x, int y) {
        Pos* existing = find(id);
        if (existing != nullptr) {
            existing->x = x;
            existing->y = y;
            return;
        }
        Pos* node = new Pos(id, x, y, head_);
        head_ = node;
    }

    bool get(const std::string& id, int& out_x, int& out_y) const {
        const Pos* p = find(id);
        if (p == nullptr) return false;
        out_x = p->x;
        out_y = p->y;
        return true;
    }

    const Pos* head() const { return head_; }

   private:
    Pos* head_;

    Pos* find(const std::string& id) const {
        Pos* cur = head_;
        while (cur != nullptr) {
            if (cur->id == id) return cur;
            cur = cur->next;
        }
        return nullptr;
    }

    void clear() {
        Pos* cur = head_;
        while (cur != nullptr) {
            Pos* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
        head_ = nullptr;
    }
};
