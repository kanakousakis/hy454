#ifndef RING_HPP
#define RING_HPP

#include "Engine.hpp"
#include <functional>
#include <cmath>

namespace app {

class Ring {
public:
    enum class State {
        Static,  //placed in level, waiting to be collected
        Scattered,  //dropped from player, bouncing
        Collected,  //being collected (animation)
        Gone  //ready for deletion
    };

private:
    float posX = 0;
    float posY = 0;
    float velX = 0;
    float velY = 0;
    
    State state = State::Static;
    
//animation
    int currentFrame = 0;
    uint64_t lastFrameTime = 0;
    static constexpr int FRAME_COUNT = 4;
    static constexpr uint64_t FRAME_DELAY = 100;  //ms
    
//scattered state
    uint64_t scatterStartTime = 0;
    static constexpr uint64_t SCATTER_LIFETIME = 4000;  //4 seconds
    static constexpr uint64_t BLINK_START = 2500;  //start blinking at 2.5s
    bool isVisible = true;
    int blinkCounter = 0;
    
//physics for scattered rings
    static constexpr float GRAVITY = 0.25f;
    static constexpr float BOUNCE = -0.75f;
    static constexpr float FLOOR_Y = 1000.0f;  //set by caller
    float floorY = FLOOR_Y;
    
//collision box
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 16;
    
//grid for ground detection
    engine::GridLayer* gridLayer = nullptr;

public:
    Ring() = default;
    Ring(float x, float y, State initialState = State::Static)
        : posX(x), posY(y), state(initialState) {
        lastFrameTime = engine::GetSystemTime();
        if (state == State::Scattered) {
            scatterStartTime = lastFrameTime;
        }
        //randomize initial animation frame to prevent synchronized animations
        currentFrame = rand() % FRAME_COUNT;
        //offset lastFrameTime randomly so frames don't all change together
        lastFrameTime -= (rand() % FRAME_DELAY);
    }
    
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetFloorY(float y) { floorY = y; }
    
//create a scattered ring with velocity
    static Ring CreateScattered(float x, float y, float vx, float vy) {
        Ring r(x, y, State::Scattered);
        r.velX = vx;
        r.velY = vy;
        r.scatterStartTime = engine::GetSystemTime();
        return r;
    }
    
    void Update() {
        uint64_t now = engine::GetSystemTime();
        
//animate
        if (now - lastFrameTime >= FRAME_DELAY) {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            lastFrameTime = now;
        }
        
        if (state == State::Scattered) {
//apply physics
            velY += GRAVITY;
            posX += velX;
            posY += velY;
            
//bounce off floor
            if (gridLayer) {
                engine::Rect box = GetBoundingBox();
                int dy = static_cast<int>(velY);
                int dx = 0;
                
                if (dy > 0) {
                    gridLayer->FilterGridMotion(box, &dx, &dy);
                    if (dy < static_cast<int>(velY)) {
                        posY = posY - velY + dy;
                        velY *= BOUNCE;
                        velX *= 0.9f;  //friction
                        
                        if (std::abs(velY) < 1.0f) {
                            velY = 0;
                        }
                    }
                }
            } else if (posY + HEIGHT >= floorY) {
                posY = floorY - HEIGHT;
                velY *= BOUNCE;
                velX *= 0.9f;
                if (std::abs(velY) < 1.0f) velY = 0;
            }
            
//wall collision
            if (gridLayer) {
                engine::Rect box = GetBoundingBox();
                int dx = static_cast<int>(velX);
                int dy = 0;
                gridLayer->FilterGridMotion(box, &dx, &dy);
                if (dx != static_cast<int>(velX)) {
                    velX = -velX * 0.5f;
                }
            }
            
//blinking when about to disappear
            uint64_t elapsed = now - scatterStartTime;
            if (elapsed >= BLINK_START) {
                blinkCounter++;
                isVisible = (blinkCounter / 3) % 2 == 0;
            }
            
//lifetime expired
            if (elapsed >= SCATTER_LIFETIME) {
                state = State::Gone;
            }
        }
        else if (state == State::Collected) {
//float upward and fade
            posY -= 2.0f;
//after a short delay, mark as gone
            if (now - scatterStartTime >= 300) {
                state = State::Gone;
            }
        }
    }
    
    void Collect() {
        if (state == State::Static || state == State::Scattered) {
            state = State::Collected;
            scatterStartTime = engine::GetSystemTime();
        }
    }
    
    bool CanBeCollected() const {
        if (state == State::Static) return true;
        if (state == State::Scattered) {
//grace period - can't collect scattered rings for first 500ms
            uint64_t elapsed = engine::GetSystemTime() - scatterStartTime;
            return elapsed > 500;
        }
        return false;
    }
    
    bool IsGone() const { return state == State::Gone; }
    bool IsVisible() const { return isVisible; }
    State GetState() const { return state; }
    
    float GetX() const { return posX; }
    float GetY() const { return posY; }
    int GetFrame() const { return currentFrame; }
    
    engine::Rect GetBoundingBox() const {
        return {
            static_cast<int>(posX),
            static_cast<int>(posY),
            WIDTH,
            HEIGHT
        };
    }
    
    bool CollidesWith(const engine::Rect& other) const {
        engine::Rect box = GetBoundingBox();
        return !(other.x + other.w < box.x ||
                 box.x + box.w < other.x ||
                 other.y + other.h < box.y ||
                 box.y + box.h < other.y);
    }
    
    void Render(const engine::Rect& viewWindow, engine::AnimationFilm* ringFilm = nullptr,
                engine::AnimationFilm* collectFilm = nullptr, float scale = 1.0f) {
        if (!isVisible && state == State::Scattered) return;
        if (state == State::Gone) return;
        
        auto& gfx = engine::GetGraphics();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
//scaled dimensions
        int scaledW = static_cast<int>(WIDTH * scale);
        int scaledH = static_cast<int>(HEIGHT * scale);
        
//skip if off screen
        if (screenX + scaledW < 0 || screenX > viewWindow.w ||
            screenY + scaledH < 0 || screenY > viewWindow.h) {
            return;
        }
        
//use collect animation for collected state
        if (state == State::Collected && collectFilm && collectFilm->GetTotalFrames() > 0) {
            int frameIdx = currentFrame % collectFilm->GetTotalFrames();
            collectFilm->DisplayFrameScaled({screenX, screenY}, static_cast<engine::byte>(frameIdx), scale);
            return;
        }
        
//try to use sprite if available
        if (ringFilm && ringFilm->GetTotalFrames() > 0) {
            int frameIdx = currentFrame % ringFilm->GetTotalFrames();
            ringFilm->DisplayFrameScaled({screenX, screenY}, static_cast<engine::byte>(frameIdx), scale);
        } else {
//fallback: Draw placeholder ring
            engine::Color ringColor;
            if (state == State::Collected) {
                ringColor = engine::MakeColor(255, 255, 200, 150);
            } else {
                int brightness = 200 + (currentFrame * 15);
                ringColor = engine::MakeColor(255, static_cast<unsigned char>(brightness), 0);
            }
            
//simple ring representation - scaled
            gfx.DrawRect({screenX, screenY, scaledW, scaledH}, ringColor, true);
            gfx.DrawRect({screenX + static_cast<int>(4*scale), screenY + static_cast<int>(4*scale), 
                         scaledW - static_cast<int>(8*scale), scaledH - static_cast<int>(8*scale)}, 
                         engine::MakeColor(135, 206, 235), true);
            gfx.DrawRect({screenX, screenY, scaledW, scaledH}, 
                         engine::MakeColor(200, 150, 0), false);
        }
    }
};

//ring manager to handle multiple rings
class RingManager {
private:
    std::vector<Ring> rings;
    engine::GridLayer* gridLayer = nullptr;
    std::function<void(int)> onRingCollected;
    
public:
    void SetGridLayer(engine::GridLayer* grid) { 
        gridLayer = grid; 
        for (auto& ring : rings) {
            ring.SetGridLayer(grid);
        }
    }
    
    void SetOnRingCollected(std::function<void(int)> callback) {
        onRingCollected = callback;
    }
    
    void AddRing(float x, float y) {
        Ring ring(x, y, Ring::State::Static);
        ring.SetGridLayer(gridLayer);
        rings.push_back(ring);
    }
    
    void ScatterRings(float centerX, float centerY, int count) {
//create rings in double circle pattern
        const float PI = 3.14159265f;
        int innerCount = std::min(count, 16);
        int outerCount = count - innerCount;
        
//inner circle
        for (int i = 0; i < innerCount; ++i) {
            float angle = (2.0f * PI * i) / innerCount;
            float speed = 3.0f + (rand() % 20) / 10.0f;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed - 4.0f;  //upward bias
            
            Ring ring = Ring::CreateScattered(centerX, centerY, vx, vy);
            ring.SetGridLayer(gridLayer);
            rings.push_back(ring);
        }
        
//outer circle (if more than 16 rings)
        for (int i = 0; i < outerCount; ++i) {
            float angle = (2.0f * PI * i) / outerCount + PI / outerCount;
            float speed = 4.0f + (rand() % 20) / 10.0f;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed - 3.0f;
            
            Ring ring = Ring::CreateScattered(centerX, centerY, vx, vy);
            ring.SetGridLayer(gridLayer);
            rings.push_back(ring);
        }
    }
    
    void Update() {
        for (auto& ring : rings) {
            ring.Update();
        }
        
//remove gone rings
        rings.erase(
            std::remove_if(rings.begin(), rings.end(),
                [](const Ring& r) { return r.IsGone(); }),
            rings.end()
        );
    }
    
//check collision with player and collect rings
    int CheckCollision(const engine::Rect& playerBox) {
        int collected = 0;
        for (auto& ring : rings) {
            if (ring.CanBeCollected() && ring.CollidesWith(playerBox)) {
                ring.Collect();
                collected++;
            }
        }
        if (collected > 0 && onRingCollected) {
            onRingCollected(collected);
        }
        return collected;
    }
    
    void Render(const engine::Rect& viewWindow, engine::AnimationFilm* ringFilm = nullptr,
                engine::AnimationFilm* collectFilm = nullptr, float scale = 1.0f) {
        for (auto& ring : rings) {
            float rx = ring.GetX();
            float ry = ring.GetY();
            if (rx >= viewWindow.x - 200 && rx <= viewWindow.x + viewWindow.w + 200 &&
                ry >= viewWindow.y - 200 && ry <= viewWindow.y + viewWindow.h + 200) {
                ring.Render(viewWindow, ringFilm, collectFilm, scale);
            }
        }
    }
    
    void Clear() { rings.clear(); }
    int GetCount() const { return static_cast<int>(rings.size()); }
};

}  //namespace app

#endif  //RING_HPP
