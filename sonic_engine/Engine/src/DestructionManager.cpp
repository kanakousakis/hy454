#include "DestructionManager.hpp"

namespace engine {

DestructionManager* DestructionManager::instance = nullptr;

DestructionManager& DestructionManager::Instance() {
    if (!instance) {
        instance = new DestructionManager();
    }
    return *instance;
}

void LatelyDestroyable::Destroy() {
    if (alive) {
        alive = false;
        DestructionManager::Instance().Register(this);
    }
}

}  //namespace engine
