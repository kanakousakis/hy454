#include "Game.hpp"

namespace engine {

Game* Game::instance = nullptr;

Game& Game::Instance() {
    if (!instance) {
        instance = new Game();
    }
    return *instance;
}

void Game::MainLoop() {
    while (!IsFinished()) {
        MainLoopIteration();
    }
}

void Game::MainLoopIteration() {
    Render();
    Input();
    
    if (!IsPaused()) {
        ProgressAnimations();
        AI();
        Physics();
        CollisionChecking();
        CommitDestructions();
        UserCode();
    }
}

} // namespace engine
