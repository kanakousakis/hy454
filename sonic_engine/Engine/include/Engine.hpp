#pragma once

// Core types
#include "Types.hpp"
#include "TileConstants.hpp"
#include "SystemClock.hpp"

// Graphics
#include "Bitmap.hpp"

// Input
#include "Input.hpp"

// Terrain
#include "TileLayer.hpp"
#include "GridLayer.hpp"

// Animation
#include "Animation.hpp"
#include "AnimationFilm.hpp"
#include "Animator.hpp"

// Sprite system
#include "Sprite.hpp"

// Memory management
#include "DestructionManager.hpp"

// Game
#include "Game.hpp"

namespace engine {

// Initialize the engine
inline bool EngineInit(Dim width, Dim height, const std::string& title = "Sonic Engine") {
    return Graphics::Instance().Init(width, height, title);
}

// Shutdown the engine
inline void EngineShutdown() {
    Graphics::Instance().Shutdown();
}

// Check if engine should quit
inline bool EngineShouldQuit() {
    return InputManager::Instance().IsWindowClosed() || 
           !Graphics::Instance().IsOpen();
}

} // namespace engine
