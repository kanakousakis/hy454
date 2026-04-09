#include "Game.hpp"
#include <iostream>

namespace engine {

Game* Game::instance = nullptr;
static int frameCount = 0;

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
    frameCount++;
    
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

//enable freeze debugging from outside (no longer used but keep for compatibility)
void EnableFreezeDebug() {
    std::cout << "=== DEBUG MODE ===" << std::endl;
}

}  //namespace engine
