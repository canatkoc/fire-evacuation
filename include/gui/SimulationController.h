#pragma once

#include "../simulation/FireSimulator.h"

// ---------------------------------------------------------------------------
// SimulationController
//
//   GUI'den gelen Start / Pause / Resume / Step / Reset sinyallerini dinler
//   ve FireSimulator motorunu sabit bir tick aralığında ilerletir.
//
//   GUI'den bağımsızdır: ne SDL ne de başka bir kütüphane içerir. main_gui
//   her frame'de update(dt) çağırır; Controller iç biriktiriciyi yönetir.
//
//   STL container yok.
// ---------------------------------------------------------------------------

class SimulationController {
   public:
    enum class State { Idle, Running, Paused, Finished };

    explicit SimulationController(FireSimulator& sim)
        : sim_(&sim),
          state_(State::Idle),
          tick_interval_(0.4),
          accumulator_(0.0) {}

    SimulationController(const SimulationController&) = delete;
    SimulationController& operator=(const SimulationController&) = delete;

    // ===== Sinyaller (GUI butonlarından çağrılır) =====

    void start() {
        if (sim_->simulationFinished()) { state_ = State::Finished; return; }
        state_ = State::Running;
        accumulator_ = 0.0;
    }

    void pause() {
        if (state_ == State::Running) state_ = State::Paused;
    }

    void resume() {
        if (state_ == State::Paused) state_ = State::Running;
    }

    void togglePause() {
        if (state_ == State::Running)      state_ = State::Paused;
        else if (state_ == State::Paused)  state_ = State::Running;
        else if (state_ == State::Idle)    start();
    }

    // Tek bir tick ilerlet (durum ne olursa olsun, simülasyon bitmediyse).
    void singleStep() {
        if (sim_->simulationFinished()) { state_ = State::Finished; return; }
        sim_->step();
        if (sim_->simulationFinished()) state_ = State::Finished;
        else if (state_ == State::Idle)  state_ = State::Paused;
    }

    // Controller durumunu sıfırlar. (Sahnenin kendisini yeniden kurmak
    // çağıranın sorumluluğundadır; FireSimulator'da reset() yoktur.)
    void resetController() {
        state_ = State::Idle;
        accumulator_ = 0.0;
    }

    // ===== Tick yönetimi =====

    // Her frame'de geçen süre (saniye) ile çağır. State==Running iken
    // dahili biriktirici tick_interval_'i aştıkça sim->step() tetiklenir.
    void update(double dt_seconds) {
        if (state_ != State::Running) return;
        if (sim_->simulationFinished()) {
            state_ = State::Finished;
            return;
        }

        accumulator_ += dt_seconds;
        // Bir frame'de birden fazla tick birikebilir (örn. UI takılırsa).
        while (accumulator_ >= tick_interval_ && state_ == State::Running) {
            sim_->step();
            accumulator_ -= tick_interval_;
            if (sim_->simulationFinished()) {
                state_ = State::Finished;
                break;
            }
        }
    }

    // ===== Tuning =====

    void setTickInterval(double seconds) {
        if (seconds < 0.05) seconds = 0.05;
        tick_interval_ = seconds;
    }
    double tickInterval() const { return tick_interval_; }

    // ===== Erişim =====

    State state() const { return state_; }
    const FireSimulator& sim() const { return *sim_; }
    FireSimulator& sim() { return *sim_; }

    const char* stateLabel() const {
        switch (state_) {
            case State::Idle:     return "IDLE";
            case State::Running:  return "RUNNING";
            case State::Paused:   return "PAUSED";
            case State::Finished: return "FINISHED";
        }
        return "?";
    }

   private:
    FireSimulator* sim_;
    State state_;
    double tick_interval_;
    double accumulator_;
};
