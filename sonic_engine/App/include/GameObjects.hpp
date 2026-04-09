#ifndef GAME_OBJECTS_HPP
#define GAME_OBJECTS_HPP

#include "Engine.hpp"
#include <vector>
#include <functional>

using namespace engine;

namespace app {

//============================================================
//SPRING - Bounces Sonic upward
//============================================================
enum class SpringDirection { Up, Left, Right, DiagonalUpLeft, DiagonalUpRight };

class Spring {
public:
    enum class AnimState { Resting, Compressed, Extended };
    
    float x, y;
    int width = 32;
    int height = 32;
    SpringDirection direction = SpringDirection::Up;
    float bounceForce = 14.0f;  //spring bounce power
    
//3-state animation
    AnimState animState = AnimState::Resting;
    uint64_t stateChangeTime = 0;
    static constexpr uint64_t COMPRESS_DURATION = 50;  //ms compressed
    static constexpr uint64_t EXTEND_DURATION = 300;  //ms extended (doubled)
    
    bool isYellow = true;
    
    Spring(float px, float py, SpringDirection dir = SpringDirection::Up, bool yellow = true)
        : x(px), y(py), direction(dir), isYellow(yellow) {
        bounceForce = yellow ? 14.0f : 10.0f;  //yellow: 14, Red: 10 (middle ground)
    }
    
    Rect GetBoundingBox() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
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
        animState = AnimState::Compressed;
        stateChangeTime = currentTime;
    }
    
    void Update(uint64_t currentTime) {
        uint64_t elapsed = currentTime - stateChangeTime;
        
        switch (animState) {
            case AnimState::Compressed:
                if (elapsed >= COMPRESS_DURATION) {
                    animState = AnimState::Extended;
                    stateChangeTime = currentTime;
                }
                break;
            case AnimState::Extended:
                if (elapsed >= EXTEND_DURATION) {
                    animState = AnimState::Resting;
                }
                break;
            default:
                break;
        }
    }
    
    int GetAnimFrameIndex() const {
        return static_cast<int>(animState);  //0=Resting, 1=Compressed, 2=Extended
    }
};

//============================================================
//SPIKES - Damages Sonic on contact
//============================================================
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
//actual damage area is smaller than visual
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
    
//check if Sonic should be damaged based on approach direction
    bool ShouldDamage(float sonicVelX, float sonicVelY) const {
        switch (direction) {
            case SpikeDirection::Up:
                return sonicVelY >= 0;  //falling onto OR standing on spikes
            case SpikeDirection::Down:
                return sonicVelY <= 0;  //jumping into OR under spikes
            case SpikeDirection::Left:
                return sonicVelX >= 0;  //running into OR standing next to spikes
            case SpikeDirection::Right:
                return sonicVelX <= 0;  //running into OR standing next to spikes
        }
        return true;
    }
};

//============================================================
//CHECKPOINT - Saves player position
//============================================================
class Checkpoint {
public:
    float x, y;
    int width = 40;  //widest frame is 40px
    int height = 64;  //64px tall
    bool activated = false;
    bool animationComplete = false;  //true when spin animation finished
    uint64_t activationTime = 0;
    
//animation - 16 frames, plays once when activated
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    static constexpr int FRAME_COUNT = 16;  
    static constexpr uint64_t FRAME_DELAY = 80;
    
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
//only animate if activated and animation not complete
        if (activated && !animationComplete && currentTime - lastFrameTime > FRAME_DELAY) {
            animFrame++;
            lastFrameTime = currentTime;
            if (animFrame >= FRAME_COUNT) {
                animFrame = FRAME_COUNT - 1;  //stay on last frame (Sonic face)
                animationComplete = true;
            }
        }
    }
};

//============================================================
//MONITOR / ITEM BOX - Breakable containers with power-ups
//============================================================
enum class MonitorType {
    Ring,  //+10 rings
    Shield,  //blue shield, absorbs 1 hit
    Invincibility,  //20 seconds of invincibility
    SpeedShoes,  //10 seconds of increased speed
    ExtraLife,  //+1 life
    Eggman  //damages Sonic (trap)
};

enum class MonitorState {
    Idle,  //normal, waiting to be hit
    Breaking,  //being destroyed
    Destroyed  //gone, power-up released
};

class Monitor {
public:
    float x, y;
    int width = 32;
    int height = 32;
    MonitorType type;
    MonitorState state = MonitorState::Idle;
    
//animation
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    uint64_t breakTime = 0;
    static constexpr uint64_t ICON_FLASH_DELAY = 150;  //icon flickers
    static constexpr uint64_t BREAK_DURATION = 1000;  //broken sprite visible for 1 second
    static constexpr uint64_t POPUP_DURATION = 800;  //icon popup visible for 800ms
    
//power-up popup (rises after break)
    float popupY = 0;
    float popupVelY = 0;
    float popupTargetY = 0;
    bool showPopup = false;
    bool popupFloating = false;
    uint64_t floatStartTime = 0;
    
    Monitor(float px, float py, MonitorType t)
        : x(px), y(py), type(t), popupY(py), popupTargetY(py - 12) {}
    
    Rect GetBoundingBox() const {
        if (state == MonitorState::Destroyed) {
            return {0, 0, 0, 0};  //no collision when destroyed
        }
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
    
//returns true if Sonic can break this
//MUST be in ball state (jumping, rolling, or spindash) to break
    bool CanBreak(bool sonicInBallState, float /*sonicVelY*/) const {
        if (state != MonitorState::Idle) return false;
//require ball state to break (jumping, rolling, or spindash)
        return sonicInBallState;
    }
    
    void Break(uint64_t currentTime) {
        if (state == MonitorState::Idle) {
            state = MonitorState::Breaking;
            breakTime = currentTime;
            showPopup = true;
            popupVelY = -0.75f;  //power-up icon rises slowly
            popupY = y;
            popupFloating = false;
            floatStartTime = 0;
        }
    }
    
    void Update(uint64_t currentTime) {
//icon flicker animation
        if (state == MonitorState::Idle) {
            if (currentTime - lastFrameTime > ICON_FLASH_DELAY) {
                animFrame = (animFrame + 1) % 2;
                lastFrameTime = currentTime;
            }
        }
        
//breaking animation
        if (state == MonitorState::Breaking) {
            if (currentTime - breakTime > BREAK_DURATION) {
                state = MonitorState::Destroyed;
            }
        }
        
//power-up popup animation: rise, float, disappear
        if (showPopup) {
            if (!popupFloating) {
//stage 1: rise to target height
                popupY += popupVelY;
                if (popupY <= popupTargetY) {
//reached target, start floating
                    popupY = popupTargetY;
                    popupVelY = 0;
                    popupFloating = true;
                    floatStartTime = currentTime;
                }
            } else {
//stage 2 & 3: float then disappear
                if (currentTime - floatStartTime > 900) {
                    showPopup = false;
                }
            }
        }
    }
    
//get the color for placeholder rendering
    Color GetIconColor() const {
        switch (type) {
            case MonitorType::Ring:         return MakeColor(255, 215, 0);  //gold
            case MonitorType::Shield:       return MakeColor(0, 150, 255);  //blue
            case MonitorType::Invincibility: return MakeColor(255, 255, 0);  //yellow
            case MonitorType::SpeedShoes:   return MakeColor(255, 100, 0);  //orange
            case MonitorType::ExtraLife:    return MakeColor(0, 255, 0);  //green
            case MonitorType::Eggman:       return MakeColor(255, 0, 0);  //red
        }
        return MakeColor(255, 255, 255);
    }
};

//============================================================
//MANAGERS - Handle collections of game objects
//============================================================

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
    
//check collision and apply bounce. Returns true if bounced.
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
    
//check collision. Returns true if Sonic should be damaged.
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
    
//check collision. Returns true if a new checkpoint was activated.
    bool CheckCollision(const Rect& sonicBox, uint64_t currentTime) {
        for (size_t i = 0; i < checkpoints.size(); ++i) {
            auto& cp = checkpoints[i];
            if (!cp.activated && RectsOverlap(sonicBox, cp.GetBoundingBox())) {
                cp.Activate(currentTime);
                lastActivatedIndex = static_cast<int>(i);
                respawnX = cp.x;
                respawnY = cp.y - 40;  //spawn above checkpoint
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
    
//check collision. Returns the MonitorType if broken, or -1 if no collision.
//sonicInBallState: true if Sonic is in ball/jump state
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

//============================================================
//FLOWER - Decorative animated background element
//============================================================
enum class FlowerType { Tall, Short };

class Flower {
public:
    float x, y;
    int width, height;
    FlowerType type;
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    uint64_t frameDelay;
    int totalFrames;
    
    Flower(float px, float py, FlowerType t)
        : x(px), y(py), type(t) {
        if (type == FlowerType::Tall) {
            width = 32;
            height = 40;
            totalFrames = 3;
            frameDelay = 250;
        } else {
            width = 32;
            height = 32;
            totalFrames = 2;
            frameDelay = 300;
        }
        //randomize initial animation to prevent synchronized animations
        animFrame = rand() % totalFrames;
        lastFrameTime = engine::GetSystemTime() - (rand() % frameDelay);
    }
    
    void Update(uint64_t currentTime) {
        if (currentTime - lastFrameTime > frameDelay) {
            animFrame = (animFrame + 1) % totalFrames;
            lastFrameTime = currentTime;
        }
    }
};

class FlowerManager {
    std::vector<Flower> flowers;
    
public:
    void Add(float x, float y, FlowerType type = FlowerType::Tall) {
        flowers.emplace_back(x, y, type);
    }
    
    void Clear() { flowers.clear(); }
    
    void Update(uint64_t currentTime) {
        for (auto& f : flowers) {
            f.Update(currentTime);
        }
    }
    
    const std::vector<Flower>& GetFlowers() const { return flowers; }
};

//============================================================
//BIG RING - Level complete goal ring
//============================================================
class BigRing {
public:
    float x, y;
    int width = 64;
    int height = 64;
    float scale = 2.0f;  //1.2x previous size (1.7 * 1.2 ≈ 2.0)
    bool active = true;
    bool collected = false;
    
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    static constexpr int totalFrames = 4;
    static constexpr uint64_t frameDelay = 150;  //slower spin (was 60)
    
    BigRing(float px, float py, float s = 2.0f) : x(px), y(py), scale(s) {
        //randomize initial animation to prevent synchronized animations
        animFrame = rand() % totalFrames;
        lastFrameTime = engine::GetSystemTime() - (rand() % frameDelay);
    }
    
    Rect GetBoundingBox() const {
        int scaledW = static_cast<int>(width * scale);
        int scaledH = static_cast<int>(height * scale);
        return {static_cast<int>(x), static_cast<int>(y), scaledW, scaledH};
    }
    
    void Update(uint64_t currentTime) {
        if (!active) return;
        if (currentTime - lastFrameTime > frameDelay) {
            animFrame = (animFrame + 1) % totalFrames;
            lastFrameTime = currentTime;
        }
    }
    
    bool CheckCollision(const Rect& playerBox) {
        if (!active || collected) return false;
        auto box = GetBoundingBox();
        bool hit = !(playerBox.x + playerBox.w < box.x ||
                    playerBox.x > box.x + box.w ||
                    playerBox.y + playerBox.h < box.y ||
                    playerBox.y > box.y + box.h);
        if (hit) {
            collected = true;
            return true;
        }
        return false;
    }
};

class BigRingManager {
    std::vector<BigRing> rings;
    
public:
    void Add(float x, float y, float scale = 2.0f) {
        rings.emplace_back(x, y, scale);
    }
    
    void Clear() { rings.clear(); }
    
    void Update(uint64_t currentTime) {
        for (auto& r : rings) {
            r.Update(currentTime);
        }
    }
    
//returns true if any ring was collected
    bool CheckCollision(const Rect& playerBox) {
        for (auto& r : rings) {
            if (r.CheckCollision(playerBox)) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<BigRing>& GetRings() { return rings; }
    const std::vector<BigRing>& GetRings() const { return rings; }
};

}  //namespace app

#endif  //GAME_OBJECTS_HPP
