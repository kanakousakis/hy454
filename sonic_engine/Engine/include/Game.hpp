#pragma once

#include "Types.hpp"
#include "SystemClock.hpp"
#include <functional>

namespace engine {

class Game {
public:
    using Action = std::function<void()>;
    using Predicate = std::function<bool()>;

private:
    Action render;
    Action progressAnimations;
    Action input;
    Action ai;
    Action physics;
    Action collisions;
    Action destruct;
    Action userCode;
    Predicate isDone;
    
//pause system
    Action onPauseResume;
    bool paused = false;
    timestamp_t pauseTime = 0;
    
    static Game* instance;
    Game() = default;
    
    void Invoke(const Action& f) { if (f) f(); }

public:
    static Game& Instance();
    
//setters
    void SetRender(const Action& f) { render = f; }
    void SetProgressAnimations(const Action& f) { progressAnimations = f; }
    void SetInput(const Action& f) { input = f; }
    void SetAI(const Action& f) { ai = f; }
    void SetPhysics(const Action& f) { physics = f; }
    void SetCollisionChecking(const Action& f) { collisions = f; }
    void SetCommitDestructions(const Action& f) { destruct = f; }
    void SetUserCode(const Action& f) { userCode = f; }
    void SetDone(const Predicate& f) { isDone = f; }
    void SetOnPauseResume(const Action& f) { onPauseResume = f; }
    
//actions
    void Render() { Invoke(render); }
    void ProgressAnimations() { Invoke(progressAnimations); }
    void Input() { Invoke(input); }
    void AI() { Invoke(ai); }
    void Physics() { Invoke(physics); }
    void CollisionChecking() { Invoke(collisions); }
    void CommitDestructions() { Invoke(destruct); }
    void UserCode() { Invoke(userCode); }
    
//state
    bool IsFinished() const { return isDone && isDone(); }
    bool IsPaused() const { return paused; }
    timestamp_t GetPauseTime() const { return pauseTime; }
    
//pause/Resume
    void Pause(timestamp_t t) {
        if (!paused) {
            paused = true;
            pauseTime = t;
            Invoke(onPauseResume);
        }
    }
    
    void Resume() {
        if (paused) {
            paused = false;
            Invoke(onPauseResume);
            pauseTime = 0;
        }
    }
    
    void TogglePause() {
        if (paused) Resume();
        else Pause(GetSystemTime());
    }
    
//main loop
    void MainLoop();
    void MainLoopIteration();
};

//convenience function
inline Game& GetGame() { return Game::Instance(); }

//debug function to enable freeze debugging
void EnableFreezeDebug();

}  //namespace engine
