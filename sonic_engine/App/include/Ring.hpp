#ifndef RING_HPP
#define RING_HPP

#include "Engine.hpp"
#include <functional>
#include <cmath>

namespace app {

class Ring {
public:
    enum class State {
        Static,     // Placed in level, waiting to be collected
        Scattered,  // Dropped from player, bouncing
        Collected,  // Being collected (animation)
        Gone        // Ready for deletion
    };

private:
    float posX = 0;
    float posY = 0;
    float velX = 0;
    float velY = 0;
    
    State state = State::Static;
    
    // Animation
    int currentFrame = 0;
    uint64_t lastFrameTime = 0;
    static constexpr int FRAME_COUNT = 4;
    static constexpr uint64_t FRAME_DELAY = 100;  // ms
    
    // Scattered state
    uint64_t scatterStartTime = 0;
    static constexpr uint64_t SCATTER_LIFETIME = 4000;  // 4 seconds
    static constexpr uint64_t BLINK_START = 2500;       // Start blinking at 2.5s
    bool isVisible = true;
    int blinkCounter = 0;
    
    // Physics for scattered rings
    static constexpr float GRAVITY = 0.25f;
    static constexpr float BOUNCE = -0.75f;
    static constexpr float FLOOR_Y = 1000.0f;  // Set by caller
    float floorY = FLOOR_Y;
    
    // Collision box
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 16;
    
    // Grid for ground detection
    engine::GridLayer* gridLayer = nullptr;

public:
    Ring() = default;
    Ring(float x, float y, State initialState = State::Static)
        : posX(x), posY(y), state(initialState) {
        lastFrameTime = engine::GetSystemTime();
        if (state == State::Scattered) {
            scatterStartTime = lastFrameTime;
        }
    }
    
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetFloorY(float y) { floorY = y; }
    
    // Create a scattered ring with velocity
    static Ring CreateScattered(float x, float y, float vx, float vy) {
        Ring r(x, y, State::Scattered);
        r.velX = vx;
        r.velY = vy;
        r.scatterStartTime = engine::GetSystemTime();
        return r;
    }
    
    void Update() {
        uint64_t now = engine::GetSystemTime();
        
        // Animate
        if (now - lastFrameTime >= FRAME_DELAY) {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            lastFrameTime = now;
        }
        
        if (state == State::Scattered) {
            // Apply physics
            velY += GRAVITY;
            posX += velX;
            posY += velY;
            
            // Bounce off floor
            if (gridLayer) {
                engine::Rect box = GetBoundingBox();
                int dy = static_cast<int>(velY);
                int dx = 0;
                
                if (dy > 0) {
                    gridLayer->FilterGridMotion(box, &dx, &dy);
                    if (dy < static_cast<int>(velY)) {
                        posY = posY - velY + dy;
                        velY *= BOUNCE;
                        velX *= 0.9f;  // Friction
                        
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
            
            // Wall collision
            if (gridLayer) {
                engine::Rect box = GetBoundingBox();
                int dx = static_cast<int>(velX);
                int dy = 0;
                gridLayer->FilterGridMotion(box, &dx, &dy);
                if (dx != static_cast<int>(velX)) {
                    velX = -velX * 0.5f;
                }
            }
            
            // Blinking when about to disappear
            uint64_t elapsed = now - scatterStartTime;
            if (elapsed >= BLINK_START) {
                blinkCounter++;
                isVisible = (blinkCounter / 3) % 2 == 0;
            }
            
            // Lifetime expired
            if (elapsed >= SCATTER_LIFETIME) {
                state = State::Gone;
            }
        }
        else if (state == State::Collected) {
            // Float upward and fade
            posY -= 2.0f;
            // After a short delay, mark as gone
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
            // Grace period - can't collect scattered rings for first 500ms
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
                engine::AnimationFilm* collectFilm = nullptr) {
        if (!isVisible && state == State::Scattered) return;
        if (state == State::Gone) return;
        
        auto& gfx = engine::GetGraphics();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        // Skip if off screen
        if (screenX + WIDTH < 0 || screenX > viewWindow.w ||
            screenY + HEIGHT < 0 || screenY > viewWindow.h) {
            return;
        }
        
        // Use collect animation for collected state
        if (state == State::Collected && collectFilm && collectFilm->GetTotalFrames() > 0) {
            int frameIdx = currentFrame % collectFilm->GetTotalFrames();
            collectFilm->DisplayFrame({screenX, screenY}, static_cast<engine::byte>(frameIdx));
            return;
        }
        
        // Try to use sprite if available
        if (ringFilm && ringFilm->GetTotalFrames() > 0) {
            int frameIdx = currentFrame % ringFilm->GetTotalFrames();
            ringFilm->DisplayFrame({screenX, screenY}, static_cast<engine::byte>(frameIdx));
        } else {
            // Fallback: Draw placeholder ring
            engine::Color ringColor;
            if (state == State::Collected) {
                ringColor = engine::MakeColor(255, 255, 200, 150);
            } else {
                int brightness = 200 + (currentFrame * 15);
                ringColor = engine::MakeColor(255, static_cast<unsigned char>(brightness), 0);
            }
            
            // Simple ring representation
            gfx.DrawRect({screenX, screenY, WIDTH, HEIGHT}, ringColor, true);
            gfx.DrawRect({screenX + 4, screenY + 4, WIDTH - 8, HEIGHT - 8}, 
                         engine::MakeColor(135, 206, 235), true);
            gfx.DrawRect({screenX, screenY, WIDTH, HEIGHT}, 
                         engine::MakeColor(200, 150, 0), false);
        }
    }
};

// Ring manager to handle multiple rings
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
        // Create rings in double circle pattern
        const float PI = 3.14159265f;
        int innerCount = std::min(count, 16);
        int outerCount = count - innerCount;
        
        // Inner circle
        for (int i = 0; i < innerCount; ++i) {
            float angle = (2.0f * PI * i) / innerCount;
            float speed = 3.0f + (rand() % 20) / 10.0f;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed - 4.0f;  // Upward bias
            
            Ring ring = Ring::CreateScattered(centerX, centerY, vx, vy);
            ring.SetGridLayer(gridLayer);
            rings.push_back(ring);
        }
        
        // Outer circle (if more than 16 rings)
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
        
        // Remove gone rings
        rings.erase(
            std::remove_if(rings.begin(), rings.end(),
                [](const Ring& r) { return r.IsGone(); }),
            rings.end()
        );
    }
    
    // Check collision with player and collect rings
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
                engine::AnimationFilm* collectFilm = nullptr) {
        for (auto& ring : rings) {
            ring.Render(viewWindow, ringFilm, collectFilm);
        }
    }
    
    void Clear() { rings.clear(); }
    int GetCount() const { return static_cast<int>(rings.size()); }
};

} // namespace app

#endif // RING_HPP
