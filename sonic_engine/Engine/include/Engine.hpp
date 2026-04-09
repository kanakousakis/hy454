#pragma once

//core types
#include "Types.hpp"
#include "TileConstants.hpp"
#include "SystemClock.hpp"

//graphics
#include "Bitmap.hpp"

//input
#include "Input.hpp"

//terrain
#include "TileLayer.hpp"
#include "GridLayer.hpp"

//animation
#include "Animation.hpp"
#include "AnimationFilm.hpp"
#include "Animator.hpp"

//sprite system
#include "Sprite.hpp"

//memory management
#include "DestructionManager.hpp"

//game
#include "Game.hpp"

namespace engine {

//initialize the engine
inline bool EngineInit(Dim width, Dim height, const std::string& title = "Sonic Engine") {
    return Graphics::Instance().Init(width, height, title);
}

//shutdown the engine
inline void EngineShutdown() {
    Graphics::Instance().Shutdown();
}

//check if engine should quit
inline bool EngineShouldQuit() {
    return InputManager::Instance().IsWindowClosed() || 
           !Graphics::Instance().IsOpen();
}

}  //namespace engine
