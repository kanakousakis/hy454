#ifndef GAME_OBJECTS_HPP
#define GAME_OBJECTS_HPP

#include "Engine.hpp"
#include <vector>
#include <functional>

using namespace engine;

namespace app {

// ============================================================
// SPRING - Bounces Sonic upward
// ============================================================
enum class SpringDirection { Up, Left, Right, DiagonalUpLeft, DiagonalUpRight };

class Spring {
public:
    float x, y;
    int width = 32;
    int height = 32;  // Compressed height, extends when triggered
    SpringDirection direction = SpringDirection::Up;
    float bounceForce = 16.0f;  // How high Sonic bounces
    
    // Animation state
    bool isCompressed = false;
    uint64_t triggerTime = 0;
    static constexpr uint64_t EXTEND_DURATION = 200;  // ms to show extended
    
    // Visual variants
    bool isYellow = true;  // Yellow = strong, Red = weak
    
    Spring(float px, float py, SpringDirection dir = SpringDirection::Up, bool yellow = true)
        : x(px), y(py), direction(dir), isYellow(yellow) {
        bounceForce = yellow ? 16.0f : 10.0f;
    }
    
    Rect GetBoundingBox() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
    // Returns the velocity to apply to Sonic when bounced
    void GetBounceVelocity(float& vx, float& vy) const {
        switch (direction) {
            case SpringDirection::Up:
                vx = 0;
                vy = -bounceForce;
                break;
            case SpringDirection::Left:
                vx = -bounceForce * 0.8f;
                vy = -bounceForce * 0.5f;
                break;
            case SpringDirection::Right:
                vx = bounceForce * 0.8f;
                vy = -bounceForce * 0.5f;
                break;
            case SpringDirection::DiagonalUpLeft:
                vx = -bounceForce * 0.7f;
                vy = -bounceForce * 0.7f;
                break;
            case SpringDirection::DiagonalUpRight:
                vx = bounceForce * 0.7f;
                vy = -bounceForce * 0.7f;
                break;
        }
    }
    
    void Trigger(uint64_t currentTime) {
        isCompressed = true;
        triggerTime = currentTime;
    }
    
    void Update(uint64_t currentTime) {
        if (isCompressed && currentTime - triggerTime > EXTEND_DURATION) {
            isCompressed = false;
        }
    }
    
    int GetCurrentHeight() const {
        return isCompressed ? height / 2 : height;
    }
};

// ============================================================
// SPIKES - Damages Sonic on contact
// ============================================================
enum class SpikeDirection { Up, Down, Left, Right };

class Spike {
public:
    float x, y;
    int width = 32;
    int height = 32;
    SpikeDirection direction = SpikeDirection::Up;
    
    Spike(float px, float py, SpikeDirection dir = SpikeDirection::Up)
        : x(px), y(py), direction(dir) {}
    
    Rect GetBoundingBox() const {
        // Actual damage area is smaller than visual
        int shrink = 4;
        return {
            static_cast<int>(x) + shrink, 
            static_cast<int>(y) + shrink, 
            width - shrink * 2, 
            height - shrink * 2
        };
    }
    
    Rect GetVisualBox() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
    // Check if Sonic should be damaged based on approach direction
    bool ShouldDamage(float sonicVelX, float sonicVelY) const {
        switch (direction) {
            case SpikeDirection::Up:
                return sonicVelY >= 0;  // Falling onto OR standing on spikes
            case SpikeDirection::Down:
                return sonicVelY <= 0;  // Jumping into OR under spikes
            case SpikeDirection::Left:
                return sonicVelX >= 0;  // Running into OR standing next to spikes
            case SpikeDirection::Right:
                return sonicVelX <= 0;  // Running into OR standing next to spikes
        }
        return true;
    }
};

// ============================================================
// CHECKPOINT - Saves player position
// ============================================================
class Checkpoint {
public:
    float x, y;
    int width = 24;
    int height = 64;
    bool activated = false;
    uint64_t activationTime = 0;
    
    // Animation
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    static constexpr int FRAME_COUNT = 4;
    static constexpr uint64_t FRAME_DELAY = 100;
    
    Checkpoint(float px, float py) : x(px), y(py) {}
    
    Rect GetBoundingBox() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
    void Activate(uint64_t currentTime) {
        if (!activated) {
            activated = true;
            activationTime = currentTime;
            animFrame = 0;
        }
    }
    
    void Update(uint64_t currentTime) {
        if (activated && currentTime - lastFrameTime > FRAME_DELAY) {
            animFrame = (animFrame + 1) % FRAME_COUNT;
            lastFrameTime = currentTime;
        }
    }
};

// ============================================================
// MONITOR / ITEM BOX - Breakable containers with power-ups
// ============================================================
enum class MonitorType {
    Ring,           // +10 rings
    Shield,         // Blue shield, absorbs 1 hit
    Invincibility,  // 20 seconds of invincibility
    SpeedShoes,     // 10 seconds of increased speed
    ExtraLife,      // +1 life
    Eggman          // Damages Sonic (trap)
};

enum class MonitorState {
    Idle,           // Normal, waiting to be hit
    Breaking,       // Being destroyed
    Destroyed       // Gone, power-up released
};

class Monitor {
public:
    float x, y;
    int width = 30;
    int height = 30;
    MonitorType type;
    MonitorState state = MonitorState::Idle;
    
    // Animation
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    uint64_t breakTime = 0;
    static constexpr uint64_t ICON_FLASH_DELAY = 150;  // Icon flickers
    static constexpr uint64_t BREAK_DURATION = 300;
    
    // Power-up popup (rises after break)
    float popupY = 0;
    float popupVelY = 0;
    bool showPopup = false;
    
    Monitor(float px, float py, MonitorType t)
        : x(px), y(py), type(t), popupY(py) {}
    
    Rect GetBoundingBox() const {
        if (state == MonitorState::Destroyed) {
            return {0, 0, 0, 0};  // No collision when destroyed
        }
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
    // Returns true if Sonic can break this
    // MUST be in ball state (jumping, rolling, or spindash) to break
    bool CanBreak(bool sonicInBallState, float sonicVelY) const {
        if (state != MonitorState::Idle) return false;
        // Require ball state to break (jumping, rolling, or spindash)
        return sonicInBallState;
    }
    
    void Break(uint64_t currentTime) {
        if (state == MonitorState::Idle) {
            state = MonitorState::Breaking;
            breakTime = currentTime;
            showPopup = true;
            popupVelY = -4.0f;  // Power-up icon rises
        }
    }
    
    void Update(uint64_t currentTime) {
        // Icon flicker animation
        if (state == MonitorState::Idle) {
            if (currentTime - lastFrameTime > ICON_FLASH_DELAY) {
                animFrame = (animFrame + 1) % 2;
                lastFrameTime = currentTime;
            }
        }
        
        // Breaking animation
        if (state == MonitorState::Breaking) {
            if (currentTime - breakTime > BREAK_DURATION) {
                state = MonitorState::Destroyed;
            }
        }
        
        // Power-up popup rises and falls
        if (showPopup) {
            popupY += popupVelY;
            popupVelY += 0.2f;  // Gravity
            if (popupY > y) {
                showPopup = false;  // Done showing popup
            }
        }
    }
    
    // Get the color for placeholder rendering
    Color GetIconColor() const {
        switch (type) {
            case MonitorType::Ring:         return MakeColor(255, 215, 0);    // Gold
            case MonitorType::Shield:       return MakeColor(0, 150, 255);    // Blue
            case MonitorType::Invincibility: return MakeColor(255, 255, 0);   // Yellow
            case MonitorType::SpeedShoes:   return MakeColor(255, 100, 0);    // Orange
            case MonitorType::ExtraLife:    return MakeColor(0, 255, 0);      // Green
            case MonitorType::Eggman:       return MakeColor(255, 0, 0);      // Red
        }
        return MakeColor(255, 255, 255);
    }
};

// ============================================================
// MANAGERS - Handle collections of game objects
// ============================================================

class SpringManager {
    std::vector<Spring> springs;
    
public:
    void Add(float x, float y, SpringDirection dir = SpringDirection::Up, bool yellow = true) {
        springs.emplace_back(x, y, dir, yellow);
    }
    
    void Clear() { springs.clear(); }
    
    void Update(uint64_t currentTime) {
        for (auto& s : springs) {
            s.Update(currentTime);
        }
    }
    
    // Check collision and apply bounce. Returns true if bounced.
    bool CheckCollision(const Rect& sonicBox, float& outVelX, float& outVelY, uint64_t currentTime) {
        for (auto& spring : springs) {
            Rect springBox = spring.GetBoundingBox();
            if (RectsOverlap(sonicBox, springBox)) {
                spring.Trigger(currentTime);
                spring.GetBounceVelocity(outVelX, outVelY);
                return true;
            }
        }
        return false;
    }
    
    const std::vector<Spring>& GetSprings() const { return springs; }
    
private:
    bool RectsOverlap(const Rect& a, const Rect& b) const {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
                 a.y + a.h <= b.y || b.y + b.h <= a.y);
    }
};

class SpikeManager {
    std::vector<Spike> spikes;
    
public:
    void Add(float x, float y, SpikeDirection dir = SpikeDirection::Up) {
        spikes.emplace_back(x, y, dir);
    }
    
    void Clear() { spikes.clear(); }
    
    // Check collision. Returns true if Sonic should be damaged.
    bool CheckCollision(const Rect& sonicBox, float sonicVelX, float sonicVelY) const {
        for (const auto& spike : spikes) {
            Rect spikeBox = spike.GetBoundingBox();
            if (RectsOverlap(sonicBox, spikeBox)) {
                if (spike.ShouldDamage(sonicVelX, sonicVelY)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    const std::vector<Spike>& GetSpikes() const { return spikes; }
    
private:
    bool RectsOverlap(const Rect& a, const Rect& b) const {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
                 a.y + a.h <= b.y || b.y + b.h <= a.y);
    }
};

class CheckpointManager {
    std::vector<Checkpoint> checkpoints;
    int lastActivatedIndex = -1;
    float respawnX = 0;
    float respawnY = 0;
    
public:
    void Add(float x, float y) {
        checkpoints.emplace_back(x, y);
    }
    
    void Clear() { 
        checkpoints.clear(); 
        lastActivatedIndex = -1;
    }
    
    void SetInitialSpawn(float x, float y) {
        respawnX = x;
        respawnY = y;
    }
    
    void Update(uint64_t currentTime) {
        for (auto& cp : checkpoints) {
            cp.Update(currentTime);
        }
    }
    
    // Check collision. Returns true if a new checkpoint was activated.
    bool CheckCollision(const Rect& sonicBox, uint64_t currentTime) {
        for (size_t i = 0; i < checkpoints.size(); ++i) {
            auto& cp = checkpoints[i];
            if (!cp.activated && RectsOverlap(sonicBox, cp.GetBoundingBox())) {
                cp.Activate(currentTime);
                lastActivatedIndex = static_cast<int>(i);
                respawnX = cp.x;
                respawnY = cp.y - 40;  // Spawn above checkpoint
                return true;
            }
        }
        return false;
    }
    
    void GetRespawnPosition(float& x, float& y) const {
        x = respawnX;
        y = respawnY;
    }
    
    const std::vector<Checkpoint>& GetCheckpoints() const { return checkpoints; }
    
private:
    bool RectsOverlap(const Rect& a, const Rect& b) const {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
                 a.y + a.h <= b.y || b.y + b.h <= a.y);
    }
};

class MonitorManager {
    std::vector<Monitor> monitors;
    
public:
    void Add(float x, float y, MonitorType type) {
        monitors.emplace_back(x, y, type);
    }
    
    void Clear() { monitors.clear(); }
    
    void Update(uint64_t currentTime) {
        for (auto& m : monitors) {
            m.Update(currentTime);
        }
    }
    
    // Check collision. Returns the MonitorType if broken, or -1 if no collision.
    // sonicInBallState: true if Sonic is in ball/jump state
    int CheckCollision(const Rect& sonicBox, bool sonicInBallState, float sonicVelY, uint64_t currentTime) {
        for (auto& monitor : monitors) {
            if (monitor.state == MonitorState::Idle) {
                Rect monitorBox = monitor.GetBoundingBox();
                if (RectsOverlap(sonicBox, monitorBox)) {
                    if (monitor.CanBreak(sonicInBallState, sonicVelY)) {
                        monitor.Break(currentTime);
                        return static_cast<int>(monitor.type);
                    }
                }
            }
        }
        return -1;
    }
    
    std::vector<Monitor>& GetMonitors() { return monitors; }
    const std::vector<Monitor>& GetMonitors() const { return monitors; }
    
private:
    bool RectsOverlap(const Rect& a, const Rect& b) const {
        return !(a.x + a.w <= b.x || b.x + b.w <= a.x ||
                 a.y + a.h <= b.y || b.y + b.h <= a.y);
    }
};

} // namespace app

#endif // GAME_OBJECTS_HPP
