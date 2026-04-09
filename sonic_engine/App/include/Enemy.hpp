#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Engine.hpp"
#include "ResourceManager.hpp"
#include "SpriteSheetConfig.hpp"
#include <functional>
#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

namespace app {

struct SpriteFrame {
    int x, y, w, h;
};

namespace EnemyFrames {
//=== EXISTING ENEMIES ===
    inline SpriteFrame GetCrabmeatWalk(int index) {
        auto anims = EnemySpriteConfig::GetCrabmeatAnimations();
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetCrabmeatAttack(int index) {
        auto anims = EnemySpriteConfig::GetCrabmeatAnimations();
        auto& frames = anims[1].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetBuzzBomberFly(int index) {
        auto anims = EnemySpriteConfig::GetBuzzBomberAnimations();
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetBuzzBomberShoot() {
        auto anims = EnemySpriteConfig::GetBuzzBomberAnimations();
        auto& f = anims[1].frames[0];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetMotobugBody(int index) {
        auto anims = EnemySpriteConfig::GetMotobugAnimations();
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetMasher(int index) {
        auto anims = EnemySpriteConfig::GetMasherAnimations();
        auto& frames = anims[1].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//=== NEW ENEMIES ===
    
//newtron Blue (camouflage enemy)
    inline SpriteFrame GetNewtronBlue(int index) {
        auto anims = EnemySpriteConfig::GetNewtronBlueAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//newtron Green (rocket launcher)
    inline SpriteFrame GetNewtronGreen(int index) {
        auto anims = EnemySpriteConfig::GetNewtronGreenAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//bomb (walking bomb enemy)
    inline SpriteFrame GetBombWalk(int index) {
        auto anims = EnemySpriteConfig::GetBombAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,24,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetBombExplode(int index) {
        auto anims = EnemySpriteConfig::GetBombAnimations();
        if (anims.size() < 2 || anims[1].frames.empty()) return {0,0,32,32};
        auto& frames = anims[1].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//caterkiller (segmented worm)
    inline SpriteFrame GetCaterkillerHead(int index) {
        auto anims = EnemySpriteConfig::GetCaterkillerAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,16,16};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
    inline SpriteFrame GetCaterkillerBody(int index) {
        auto anims = EnemySpriteConfig::GetCaterkillerAnimations();
        if (anims.size() < 2 || anims[1].frames.empty()) return {0,0,16,16};
        auto& frames = anims[1].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//batbrain (flying bat)
    inline SpriteFrame GetBatbrain(int index) {
        auto anims = EnemySpriteConfig::GetBatbrainAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,24};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//burrobot (digging robot)
    inline SpriteFrame GetBurrobot(int index) {
        auto anims = EnemySpriteConfig::GetBurrobotAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//roller (rolling armadillo)
    inline SpriteFrame GetRoller(int index) {
        auto anims = EnemySpriteConfig::GetRollerAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//jaws (fish enemy)
    inline SpriteFrame GetJaws(int index) {
        auto anims = EnemySpriteConfig::GetJawsAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,24};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//orbinaut (spiked ball shooter)
    inline SpriteFrame GetOrbinaut(int index) {
        auto anims = EnemySpriteConfig::GetOrbinautAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//ballHog (bomb thrower)
    inline SpriteFrame GetBallHog(int index) {
        auto anims = EnemySpriteConfig::GetBallHogAnimations();
        if (anims.empty() || anims[0].frames.empty()) return {0,0,32,32};
        auto& frames = anims[0].frames;
        auto& f = frames[index % frames.size()];
        return {f.x, f.y, f.w, f.h};
    }
    
//projectile frames
    constexpr SpriteFrame CRABMEAT_PROJECTILE = {282, 35, 12, 12};
    constexpr SpriteFrame BUZZBOMBER_PROJECTILE = {120, 150, 16, 16};
    constexpr SpriteFrame BOMB_PROJECTILE = {145, 590, 16, 16};  //ballHog cannonball
    
    constexpr SpriteFrame MOTOBUG_EXHAUST[] = {
        {270, 105, 6, 6},
        {268, 121, 10, 10},
        {266, 139, 14, 12},
    };
    constexpr int MOTOBUG_EXHAUST_COUNT = 3;
    constexpr int MOTOBUG_BODY_COUNT = 2;
    constexpr int BUZZBOMBER_FLY_COUNT = 2;
}

//============================================================================
//PROJECTILE SYSTEM
//============================================================================

class Projectile {
public:
    float x, y;
    float velX, velY;
    float gravity;
    
    enum class Type { Crabmeat, BuzzBomber };
    Type type;
    
    int width, height;
    bool active = true;
    bool facingRight = false;
    uint64_t spawnTime;
    
    Projectile(float px, float py, float vx, float vy, float g, Type t, int w, int h, bool facing = false)
        : x(px), y(py), velX(vx), velY(vy), gravity(g), type(t), width(w), height(h), facingRight(facing) {
        spawnTime = engine::GetSystemTime();
    }
    
    void Update() {
        if (!active) return;
        velY += gravity;
        x += velX;
        y += velY;
        if (engine::GetSystemTime() - spawnTime > 3000 || y > 2000 || y < -100) {
            active = false;
        }
    }
    
    engine::Rect GetBounds() const {
        return {static_cast<int>(x), static_cast<int>(y), width, height};
    }
};

class ProjectileManager {
private:
    std::vector<Projectile> projectiles;
    static constexpr size_t MAX_PROJECTILES = 30;
    ProjectileManager() = default;
    
public:
    static ProjectileManager& Instance() {
        static ProjectileManager instance;
        return instance;
    }
    
//crabmeat shoots TWO projectiles - one from each claw
    void SpawnCrabmeatBullets(float x, float y, int width, bool /*facingRight*/) {
        if (projectiles.size() >= MAX_PROJECTILES - 1) return;
        
//left projectile goes left-up arc
//right projectile goes right-up arc
//these always shoot in both directions regardless of facing
        projectiles.emplace_back(x - 4, y + 4, -2.5f, -4.5f, 0.18f, 
                                 Projectile::Type::Crabmeat, 12, 12);
        projectiles.emplace_back(x + width - 8, y + 4, 2.5f, -4.5f, 0.18f, 
                                 Projectile::Type::Crabmeat, 12, 12);
    }
    
//buzzBomber shoots ONE projectile toward player
    void SpawnBuzzBomberBullet(float x, float y, float targetX, float targetY, bool facingRight) {
        if (projectiles.size() >= MAX_PROJECTILES) return;
        
        float dx = targetX - x;
        float dy = (targetY + 16) - y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist < 1.0f) dist = 1.0f;
        
        float speed = 3.5f;
//spawn from front of bee based on facing direction
        float spawnX = facingRight ? (x + 40) : (x + 8);
        projectiles.emplace_back(spawnX, y + 24, (dx/dist)*speed, (dy/dist)*speed, 0.0f, 
                                 Projectile::Type::BuzzBomber, 16, 16, facingRight);
    }
    
    void Update() {
        for (auto& p : projectiles) p.Update();
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
            [](const Projectile& p) { return !p.active; }), projectiles.end());
    }
    
    void Render(const engine::Rect& viewWindow) {
        auto& gfx = engine::GetGraphics();
        auto& rm = ResourceManager::Instance();
        auto sheet = rm.GetEnemiesSheet();
        uint64_t now = engine::GetSystemTime();
        
        for (const auto& p : projectiles) {
            if (!p.active) continue;
            
            int screenX = static_cast<int>(p.x) - viewWindow.x;
            int screenY = static_cast<int>(p.y) - viewWindow.y;
            
            if (screenX < -30 || screenX > viewWindow.w + 30 ||
                screenY < -30 || screenY > viewWindow.h + 30) continue;
            
            SpriteFrame frame = (p.type == Projectile::Type::Crabmeat) 
                ? EnemyFrames::CRABMEAT_PROJECTILE 
                : EnemyFrames::BUZZBOMBER_PROJECTILE;
            
            if (sheet) {
                engine::Rect srcRect = {frame.x, frame.y, frame.w, frame.h};
                engine::Rect destRect = {screenX, screenY, frame.w, frame.h};
                gfx.DrawTextureScaled(sheet->GetTexture(), srcRect, destRect);
            } else {
//enhanced fallback with pulsing
                uint32_t color = (p.type == Projectile::Type::Crabmeat) 
                    ? engine::MakeColor(255, 60, 0)
                    : engine::MakeColor(255, 255, 100);
                
//pulsing size for visibility
                int pulse = (now / 100) % 6;
                int size = p.width + (pulse > 3 ? (6 - pulse) : pulse) - 2;
                int offset = (p.width - size) / 2;
                
                gfx.DrawRect({screenX + offset, screenY + offset, size, size}, color, true);
            }
        }
    }
    
    bool CheckPlayerCollision(const engine::Rect& playerBox) {
        for (auto& p : projectiles) {
            if (!p.active) continue;
            engine::Rect pBox = p.GetBounds();
            if (!(playerBox.x + playerBox.w < pBox.x ||
                  pBox.x + pBox.w < playerBox.x ||
                  playerBox.y + playerBox.h < pBox.y ||
                  pBox.y + pBox.h < playerBox.y)) {
                p.active = false;
                return true;
            }
        }
        return false;
    }
    
    void Clear() { projectiles.clear(); }
};

//============================================================================
//BASE ENEMY CLASS
//============================================================================

class Enemy {
public:
    enum class State { Active, Dying, Dead };
    enum class Type { Motobug, Crabmeat, BuzzBomber, Masher };

protected:
    float posX, posY;
    float velX = 0, velY = 0;
    int width, height;
    Type type;
    State state = State::Active;
    bool facingRight = false;  //false = facing left (sprite default)
    
    uint64_t lastFrameTime = 0;
    uint64_t deathTime = 0;
    int scoreValue = 100;
    
    engine::GridLayer* gridLayer = nullptr;
    std::function<void(float, float)> onDeath;
    
public:
    Enemy(Type t, float x, float y, int w, int h)
        : posX(x), posY(y), width(w), height(h), type(t) {
        lastFrameTime = engine::GetSystemTime();
    }
    virtual ~Enemy() = default;
    
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetOnDeath(std::function<void(float, float)> cb) { onDeath = cb; }
    
    virtual void Update() = 0;
    virtual void Render(const engine::Rect& viewWindow) = 0;
    virtual void SetPlayerPosition(float, float) {}
    
    void Kill() {
        if (state == State::Active) {
            state = State::Dying;
            deathTime = engine::GetSystemTime();
            velY = -5.0f;
            if (onDeath) onDeath(posX + width/2, posY);
        }
    }
    
    bool IsDead() const { return state == State::Dead; }
    bool IsActive() const { return state == State::Active; }
    int GetScore() const { return scoreValue; }
    Type GetType() const { return type; }
    float GetX() const { return posX; }
    float GetY() const { return posY; }
    
    engine::Rect GetBoundingBox() const {
        return {static_cast<int>(posX), static_cast<int>(posY), width, height};
    }
    
    bool CollidesWith(const engine::Rect& other) const {
        engine::Rect box = GetBoundingBox();
        return !(other.x + other.w < box.x || box.x + box.w < other.x ||
                 other.y + other.h < box.y || box.y + box.h < other.y);
    }
    
    bool CanBeDamagedBy(const engine::Rect& playerBox, bool playerInBallState) {
        if (state != State::Active || !playerInBallState) return false;
        return CollidesWith(playerBox);
    }
    
    bool DamagesPlayer(const engine::Rect& playerBox, bool playerInBallState) {
        if (state != State::Active || playerInBallState) return false;
        return CollidesWith(playerBox);
    }

protected:
    void RenderFrame(const engine::Rect& viewWindow, const SpriteFrame& frame, bool flip) {
        auto& gfx = engine::GetGraphics();
        auto sheet = ResourceManager::Instance().GetEnemiesSheet();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (screenX < -100 || screenX > viewWindow.w + 100 ||
            screenY < -100 || screenY > viewWindow.h + 100) return;
        
        if (sheet) {
            if (flip) {
                sf::Sprite sprite(sheet->GetTexture());
                sprite.setTextureRect(sf::IntRect(frame.x, frame.y, frame.w, frame.h));
                sprite.setScale(-1.f, 1.f);
                sprite.setPosition(static_cast<float>(screenX + frame.w), static_cast<float>(screenY));
                gfx.DrawSprite(sprite);
            } else {
                engine::Rect srcRect = {frame.x, frame.y, frame.w, frame.h};
                engine::Rect destRect = {screenX, screenY, frame.w, frame.h};
                gfx.DrawTextureScaled(sheet->GetTexture(), srcRect, destRect);
            }
        } else {
            gfx.DrawRect({screenX, screenY, frame.w, frame.h}, engine::MakeColor(255, 0, 255), true);
        }
    }
    
    void RenderFrameAt(const engine::Rect& viewWindow, const SpriteFrame& frame, 
                       int atX, int atY, bool /*flip*/) {
        auto& gfx = engine::GetGraphics();
        auto sheet = ResourceManager::Instance().GetEnemiesSheet();
        
        int screenX = atX - viewWindow.x;
        int screenY = atY - viewWindow.y;
        
        if (sheet) {
            engine::Rect srcRect = {frame.x, frame.y, frame.w, frame.h};
            engine::Rect destRect = {screenX, screenY, frame.w, frame.h};
            gfx.DrawTextureScaled(sheet->GetTexture(), srcRect, destRect);
        }
    }
    
    void UpdateDeathAnimation() {
        velY += 0.3f;
        posY += velY;
        if (engine::GetSystemTime() - deathTime > 2000 || posY > 2000) {
            state = State::Dead;
        }
    }
    
    bool WouldHitWall(int direction) {
        if (!gridLayer) return false;
//check for wall at MID-BODY height - above any ground variations
//only detect actual walls, not small bumps or terrain variations
        engine::Rect box = GetBoundingBox();
        engine::Rect checkBox;
        checkBox.x = box.x + (direction > 0 ? box.w : -8);  //check ahead
        checkBox.y = box.y + (box.h / 3);  //check at 1/3 height from top (well above feet)
        checkBox.w = 8;
        checkBox.h = box.h / 3;  //check a decent vertical slice
        int dx = direction * 12, dy = 0;  //try to move 12 pixels
        gridLayer->FilterGridMotion(checkBox, &dx, &dy);
//only consider it a wall if we can't move at all (truly blocked)
        return (std::abs(dx) < 2);  //wall if almost no movement possible
    }
    
    bool WouldFallOff(int direction) {
        if (!gridLayer) return false;
//check if there's ground ahead at foot level
//IMPORTANT: Allow stepping down small heights (like 8x8 blocks)
//only return true for actual cliffs
        engine::Rect box = GetBoundingBox();
        engine::Rect checkBox;
//position ahead of enemy at foot level
        checkBox.x = box.x + (direction > 0 ? box.w + 4 : -12);
        checkBox.y = box.y + box.h;  //at feet level
        checkBox.w = 8;
        checkBox.h = 2;
        
//first check: is there ground within step-down distance (16 pixels)?
        int dx = 0, dy = 16;  //check up to 16 pixels down (2 grid cells)
        gridLayer->FilterGridMotion(checkBox, &dx, &dy);
        
//if ground within 16 pixels, it's just a small step - NOT a cliff
        if (dy < 16) {
            return false;  //can walk down small steps
        }
        
//check further down for actual cliffs
        dy = 48;  //check 48 pixels down
        gridLayer->FilterGridMotion(checkBox, &dx, &dy);
        
//if no ground within 48 pixels, it's a real cliff
        return (dy >= 48);
    }
    
    void ApplyGravity(float amount = 0.3f) {
        velY += amount;
        if (velY > 12.0f) velY = 12.0f;
    }
    
    void ApplyMovement() {
        if (!gridLayer) {
            posX += velX;
            posY += velY;
            return;
        }
        
//boundary check - turn around at map edges instead of falling into void
//map boundaries: 0 to MAP_WIDTH (4992 pixels)
        constexpr float MAP_LEFT = 16.0f;  //small margin from left edge
        constexpr float MAP_RIGHT = 4992.0f - 16.0f;  //small margin from right edge
        constexpr float MAX_FALL_Y = 1280.0f + 100.0f;  //below map = death
        
//check if about to go off map horizontally
        float nextX = posX + velX;
        if (nextX < MAP_LEFT || nextX + width > MAP_RIGHT) {
            velX = -velX;  //turn around
            facingRight = !facingRight;
            nextX = posX;  //don't move past edge
        }
        posX = nextX;
        
//handle vertical movement with grid collision (for gravity/landing)
        if (velY != 0) {
            engine::Rect box = GetBoundingBox();
            int dx = 0;
            int dy = static_cast<int>(velY);
            gridLayer->FilterGridMotion(box, &dx, &dy);
            posY += dy;
            if (dy == 0 && velY > 0) velY = 0;  //hit ground
        }
        
//ground snap: if enemy is on ground (velY==0), check if ground moved down (slope)
//this makes enemies follow terrain properly when walking down slopes
        if (velY == 0) {
            engine::Rect box = GetBoundingBox();
//probe downward to find ground
            int probeX = 0;
            int probeY = 8;  //check up to 8 pixels down for slope
            gridLayer->FilterGridMotion(box, &probeX, &probeY);
            if (probeY > 0 && probeY <= 8) {
//there's ground within 8 pixels below - snap to it
                posY += probeY;
            } else if (probeY > 8) {
//no ground within 8 pixels - check further for ledge vs pit
                int farProbe = 64;  //check 64 pixels down
                gridLayer->FilterGridMotion(box, &probeX, &farProbe);
                if (farProbe >= 64) {
//no ground for 64 pixels - this is a cliff/pit, turn around
                    velX = -velX;
                    facingRight = !facingRight;
                } else {
//ground found below - start falling
                    velY = 0.1f;
                }
            }
        }
        
//kill enemy if fallen too far
        if (posY > MAX_FALL_Y) {
            state = State::Dead;
        }
    }
};

//============================================================================
//MOTOBUG
//============================================================================

class Motobug : public Enemy {
private:
    float speed = 1.5f;
    bool paused = false;
    uint64_t pauseStart = 0;
    int bodyFrame = 0;
    int exhaustFrame = 0;
    uint64_t lastBodyTime = 0;
    uint64_t lastExhaustTime = 0;
    
public:
    Motobug(float x, float y) : Enemy(Type::Motobug, x, y, 40, 33) {
        velX = -speed;
        facingRight = false;
        lastBodyTime = lastExhaustTime = engine::GetSystemTime();
    }
    
    void Update() override {
        if (state == State::Dying) { UpdateDeathAnimation(); return; }
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
        if (paused) {
            if (now - pauseStart >= 500) {
                paused = false;
                facingRight = !facingRight;
                velX = facingRight ? speed : -speed;
            }
            return;
        }
        
        int dir = facingRight ? 1 : -1;
//only check for walls, not ledges (WouldFallOff was too sensitive)
        if (WouldHitWall(dir)) {
            velX = 0;
            paused = true;
            pauseStart = now;
            return;
        }
        
        ApplyGravity();
        ApplyMovement();
        
//animate body (wheel rotation)
        if (now - lastBodyTime >= 80) {
            bodyFrame = (bodyFrame + 1) % EnemyFrames::MOTOBUG_BODY_COUNT;
            lastBodyTime = now;
        }
        
//animate exhaust
        if (now - lastExhaustTime >= 100) {
            exhaustFrame = (exhaustFrame + 1) % EnemyFrames::MOTOBUG_EXHAUST_COUNT;
            lastExhaustTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        if (state == State::Dead) return;
        
        bool flip = facingRight;
        
        SpriteFrame exhaustFr = EnemyFrames::MOTOBUG_EXHAUST[exhaustFrame];
        int exhaustX, exhaustY;
        if (facingRight) {
            exhaustX = static_cast<int>(posX) - exhaustFr.w - 2;
        } else {
            exhaustX = static_cast<int>(posX) + width + 2;
        }
        exhaustY = static_cast<int>(posY) + height - exhaustFr.h - 10;
        RenderFrameAt(viewWindow, exhaustFr, exhaustX, exhaustY, false);
        
        SpriteFrame bodyFr = EnemyFrames::GetMotobugBody(bodyFrame);
        RenderFrame(viewWindow, bodyFr, flip);
    }
};

//============================================================================
//CRABMEAT - Smooth transitions
//============================================================================

class Crabmeat : public Enemy {
private:
    enum class CrabState { Walking, PreAttack, Attacking, PostAttack, Cooldown };
    CrabState crabState = CrabState::Walking;
    
    float speed = 0.8f;
    float playerX = 0, playerY = 0;
    
    uint64_t stateStart = 0;
    int walkFrame = 0;
    int attackFrame = 0;
    bool walkForward = true;
    bool hasShot = false;
    uint64_t lastWalkTime = 0;
    
//check if there's a cliff/void ahead (5 tiles down with no ground)
    bool IsCliffAhead(int direction) {
        if (!gridLayer) return false;
        
        engine::Rect box = GetBoundingBox();
//check position ahead of crab at foot level
        int checkX = (direction > 0) ? (box.x + box.w + 4) : (box.x - 12);
        int checkY = box.y + box.h;  //at feet
        
//convert to grid coordinates
        int gridCol = checkX / engine::GRID_ELEMENT_WIDTH;
        int gridRow = checkY / engine::GRID_ELEMENT_HEIGHT;
        
//check 5 tiles downward - if all are empty (0 or -1), it's a cliff
        int emptyCount = 0;
        for (int i = 0; i < 5; i++) {
            int tileType = gridLayer->GetTile(gridCol, gridRow + i);
            if (tileType <= 0) {  //empty or invalid
                emptyCount++;
            }
        }
        
//if all 5 tiles are empty, it's a cliff
        return (emptyCount >= 5);
    }
    
//check if there's a wall ahead (tiles that are solid)
    bool IsWallAhead(int direction) {
        if (!gridLayer) return false;
        
        engine::Rect box = GetBoundingBox();
//check position ahead of crab at body level
        int checkX = (direction > 0) ? (box.x + box.w + 4) : (box.x - 12);
        int checkY = box.y + box.h / 2;  //mid body
        
//convert to grid coordinates
        int gridCol = checkX / engine::GRID_ELEMENT_WIDTH;
        int gridRow = checkY / engine::GRID_ELEMENT_HEIGHT;
        
//check 3 tiles upward - if solid, it's a wall
        int solidCount = 0;
        for (int i = 0; i < 3; i++) {
            int tileType = gridLayer->GetTile(gridCol, gridRow - i);
            if (tileType > 0) {  //solid
                solidCount++;
            }
        }
        
//if 2+ tiles are solid, it's a wall
        return (solidCount >= 2);
    }
    
public:
    Crabmeat(float x, float y) : Enemy(Type::Crabmeat, x, y, 48, 32) {
        velX = -speed;
        facingRight = false;
        lastWalkTime = engine::GetSystemTime();
    }
    
    void SetPlayerPosition(float px, float py) override {
        playerX = px;
        playerY = py;
    }
    
    void DoWalkMovement() {
        int dir = facingRight ? 1 : -1;
        
//check for cliff or wall ahead
        if (IsCliffAhead(dir) || IsWallAhead(dir)) {
//turn around
            facingRight = !facingRight;
            velX = facingRight ? speed : -speed;
        } else {
//move forward
            velX = facingRight ? speed : -speed;
            posX += velX;
        }
        
//map boundary check
        if (posX < 50.0f) {
            posX = 50.0f;
            facingRight = true;
            velX = speed;
        } else if (posX > 9900.0f) {
            posX = 9900.0f;
            facingRight = false;
            velX = -speed;
        }
        
//ground following - use FindGroundBelow to stick to terrain
        if (gridLayer) {
            engine::Rect box = GetBoundingBox();
            int footX = box.x + box.w / 2;  //center of crab
            int footY = box.y + box.h;  //bottom of crab
            
//find ground below current position (search up to 24 pixels down)
            int groundY = gridLayer->FindGroundBelow(footX, footY - 4, 24);
            
            if (groundY > 0) {
//ground found - snap to it
                float targetY = static_cast<float>(groundY - box.h);
                float diff = targetY - posY;
                
                if (diff > 0 && diff <= 16) {
//ground is below us (walking down slope) - snap down
                    posY = targetY;
                    velY = 0;
                } else if (diff < 0 && diff >= -8) {
//ground is above us (walking up slope) - snap up
                    posY = targetY;
                    velY = 0;
                } else if (diff > 16) {
//too far down - apply gravity
                    ApplyGravity();
                    engine::Rect gBox = GetBoundingBox();
                    int dx = 0;
                    int dy = static_cast<int>(velY);
                    gridLayer->FilterGridMotion(gBox, &dx, &dy);
                    posY += dy;
                    if (dy == 0 && velY > 0) velY = 0;
                }
            } else {
//no ground found - apply gravity
                ApplyGravity();
                engine::Rect gBox = GetBoundingBox();
                int dx = 0;
                int dy = static_cast<int>(velY);
                gridLayer->FilterGridMotion(gBox, &dx, &dy);
                posY += dy;
                if (dy == 0 && velY > 0) velY = 0;
            }
        }
    }
    
    void DoWalkAnimation(uint64_t now) {
        if (now - lastWalkTime >= 150) {
            if (walkForward) {
                walkFrame++;
                if (walkFrame >= 2) walkForward = false;
            } else {
                walkFrame--;
                if (walkFrame <= 0) walkForward = true;
            }
            lastWalkTime = now;
        }
    }
    
    void Update() override {
        if (state == State::Dying) { UpdateDeathAnimation(); return; }
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
        switch (crabState) {
            case CrabState::Walking: {
//check if player is close for attack
                float dx = playerX - posX;
                float dy = playerY - posY;
                float dist = std::sqrt(dx*dx + dy*dy);
                
                if (dist < 150.0f && std::abs(dy) < 50) {
                    crabState = CrabState::PreAttack;
                    stateStart = now;
                    velX = 0;
                    facingRight = (dx > 0);
                    attackFrame = 0;
                    return;
                }
                
                DoWalkMovement();
                DoWalkAnimation(now);
                break;
            }
            case CrabState::PreAttack:
                attackFrame = 0;
                if (now - stateStart >= 250) {
                    crabState = CrabState::Attacking;
                    stateStart = now;
                    hasShot = false;
                }
                break;
            case CrabState::Attacking:
                attackFrame = 1;
                if (!hasShot && now - stateStart > 200) {
                    ProjectileManager::Instance().SpawnCrabmeatBullets(posX, posY, width, facingRight);
                    hasShot = true;
                }
                if (now - stateStart >= 500) {
                    crabState = CrabState::PostAttack;
                    stateStart = now;
                }
                break;
            case CrabState::PostAttack:
                attackFrame = 0;
                if (now - stateStart >= 250) {
                    crabState = CrabState::Cooldown;
                    stateStart = now;
                }
                break;
            case CrabState::Cooldown:
                DoWalkMovement();
                DoWalkAnimation(now);
                
                if (now - stateStart >= 2000) {
                    crabState = CrabState::Walking;
                }
                break;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        if (state == State::Dead) return;
        
        bool flip = facingRight;
        
        if (crabState == CrabState::PreAttack || 
            crabState == CrabState::Attacking || 
            crabState == CrabState::PostAttack) {
            SpriteFrame frame = EnemyFrames::GetCrabmeatAttack(attackFrame);
            float oldY = posY;
            posY -= 8;
            RenderFrame(viewWindow, frame, flip);
            posY = oldY;
        } else {
            SpriteFrame frame = EnemyFrames::GetCrabmeatWalk(walkFrame);
            RenderFrame(viewWindow, frame, flip);
        }
    }
};

//============================================================================
//BUZZ BOMBER - SMOOTH animation with only 2 frames
//============================================================================

class BuzzBomber : public Enemy {
private:
    enum class BuzzState { Patrol, Approach, Hover, Shoot, Retreat };
    BuzzState buzzState = BuzzState::Patrol;
    
    float homeX, homeY;
    float playerX = 0, playerY = 0;
    float speed = 2.0f;
    float patrolLeft, patrolRight;
    
//screen bounds for off-screen detection
    float screenLeft = 0, screenRight = 0, screenTop = 0, screenBottom = 0;
    
//trigger/reset distance - about 1 tile (256 pixels) from home
    static constexpr float RESET_DISTANCE = 300.0f;
    
    uint64_t stateStart = 0;
    uint64_t lastFrameTime = 0;
    int animFrame = 0;
    bool hasShot = false;
    bool showStingFrame = false;
    uint64_t stingStartTime = 0;
    bool shootingFacingRight = false;  //store direction when shooting
    
public:
    BuzzBomber(float x, float y) : Enemy(Type::BuzzBomber, x, y, 48, 32) {
        homeX = x;
        homeY = y;
        patrolLeft = x - 100;
        patrolRight = x + 100;
        velX = -speed;
        facingRight = false;
        lastFrameTime = engine::GetSystemTime();
    }
    
    void SetPlayerPosition(float px, float py) override {
        playerX = px;
        playerY = py;
    }
    
//called to set current screen bounds for off-screen detection
    void SetScreenBounds(float left, float top, float right, float bottom) {
        screenLeft = left;
        screenTop = top;
        screenRight = right;
        screenBottom = bottom;
    }
    
    bool IsOffScreen() const {
//check if completely off screen (with margin)
        const float margin = 50.0f;
        return posX + width < screenLeft - margin || 
               posX > screenRight + margin ||
               posY + height < screenTop - margin ||
               posY > screenBottom + margin;
    }
    
//check if Sonic is far from BuzzBomber's home (1 tile away = reset)
    bool IsSonicFarFromHome() const {
        float dx = playerX - homeX;
        float dy = playerY - homeY;
        return std::sqrt(dx*dx + dy*dy) > RESET_DISTANCE;
    }
    
    void ResetToHome() {
        buzzState = BuzzState::Patrol;
        posX = homeX;
        posY = homeY;
        facingRight = false;
        hasShot = false;
    }
    
    void Update() override {
        if (state == State::Dying) { UpdateDeathAnimation(); return; }
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
//reset ONLY when Sonic has left the area - NOT when BuzzBomber goes off-screen
//this prevents the teleporting appearance - BuzzBomber should fly off naturally
        if (buzzState != BuzzState::Patrol) {
            if (IsSonicFarFromHome()) {
                ResetToHome();
                return;
            }
        }
        
//wing animation - use all 4 frames for smoother animation
        unsigned frameDelay = (buzzState == BuzzState::Approach) ? 50 : 100;
        if (now - lastFrameTime >= frameDelay) {
            animFrame = (animFrame + 1) % 4;  //use all 4 frames
            lastFrameTime = now;
        }
        
        switch (buzzState) {
            case BuzzState::Patrol: {
                float dx = playerX - posX;
                float dy = playerY - posY;
                float dist = std::sqrt(dx*dx + dy*dy);
                
//only approach if Sonic is nearby (within detection range)
//don't check if on-screen - let BuzzBomber approach naturally
                if (dist < 200.0f && dist > 40.0f) {
                    buzzState = BuzzState::Approach;
                    stateStart = now;
                    facingRight = (dx > 0);
                    return;
                }
                
//patrol back and forth
                if (posX <= patrolLeft) {
                    facingRight = true;
                } else if (posX >= patrolRight) {
                    facingRight = false;
                }
                
                posX += facingRight ? speed : -speed;
                posY = homeY + std::sin(now / 300.0f) * 4.0f;
                break;
            }
            case BuzzState::Approach: {
                float dx = playerX - posX;
                float dy = playerY - posY;
                float dist = std::sqrt(dx*dx + dy*dy);
                
                if (dist > 1.0f) {
//move toward player smoothly
                    float moveX = (dx / dist) * speed * 1.8f;
                    float targetY = playerY - 50;
                    float moveY = (posY > targetY) ? -1.5f : ((posY < targetY) ? 0.8f : 0);
                    
                    posX += moveX;
                    posY += moveY;
                }
                
                facingRight = (dx > 0);
                
                if (dist < 90.0f || now - stateStart > 1500) {
                    buzzState = BuzzState::Hover;
                    stateStart = now;
                }
                break;
            }
            case BuzzState::Hover:
                posY += std::sin(now / 100.0f) * 0.5f;
                facingRight = (playerX > posX);
                
                if (now - stateStart >= 300) {
//SHOOT INSTANTLY and switch to retreat immediately
                    ProjectileManager::Instance().SpawnBuzzBomberBullet(
                        posX, posY, playerX, playerY, facingRight);
                    hasShot = true;
                    showStingFrame = true;
                    stingStartTime = now;
                    shootingFacingRight = facingRight;  //remember direction when shooting
                    buzzState = BuzzState::Retreat;
                    stateStart = now;
//retreat away from player
                    facingRight = (posX > playerX);
                }
                break;
            case BuzzState::Shoot:
//this state is no longer used - shoot happens instantly in Hover
                buzzState = BuzzState::Retreat;
                break;
            case BuzzState::Retreat:
//move away smoothly - keep flying until off screen
                posX += facingRight ? speed * 2.0f : -speed * 2.0f;
                posY -= 1.0f;
                
//reset to home when going off screen OR when Sonic leaves area
                if (IsOffScreen()) {
                    ResetToHome();
                }
                break;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
//hold shooting frame for 225ms
        if (showStingFrame && now - stingStartTime > 225) {
            showStingFrame = false;
        }
        
        SpriteFrame frame;
        bool flipDirection;
        
        if (showStingFrame) {
            frame = EnemyFrames::GetBuzzBomberShoot();
            flipDirection = shootingFacingRight;  //use direction when shot was fired
        } else {
            frame = EnemyFrames::GetBuzzBomberFly(animFrame);
            flipDirection = facingRight;  //use current direction for flying
        }
        
        RenderFrame(viewWindow, frame, flipDirection);
    }
};

//============================================================================
//MASHER
//============================================================================

class Masher : public Enemy {
private:
    enum class MasherState { Waiting, Jumping, Falling };
    MasherState masherState = MasherState::Waiting;
    
    float homeY;
    float playerX = 0, playerY = 0;
    uint64_t waitStart = 0;
    int currentFrame = 0;
    
public:
    Masher(float x, float y) : Enemy(Type::Masher, x, y, 32, 33) {
        homeY = y;
        waitStart = engine::GetSystemTime();
    }
    
    void SetPlayerPosition(float px, float py) override {
        playerX = px;
        playerY = py;
    }
    
    void Update() override {
        if (state == State::Dying) { UpdateDeathAnimation(); return; }
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
        switch (masherState) {
            case MasherState::Waiting:
                posY = homeY;
                currentFrame = 0;
                if (now - waitStart >= 400) {
                    float dx = std::abs(playerX - posX);
                    if (dx < 70.0f && playerY < homeY) {
                        masherState = MasherState::Jumping;
                        velY = -9.0f;
                        facingRight = (playerX > posX);
                    }
                }
                break;
            case MasherState::Jumping:
                velY += 0.28f;
                posY += velY;
                currentFrame = 1;
                facingRight = (playerX > posX);
                if (velY >= 0 || homeY - posY > 96) {
                    masherState = MasherState::Falling;
                }
                break;
            case MasherState::Falling:
                velY += 0.32f;
                posY += velY;
                currentFrame = 0;
                facingRight = (playerX > posX);
                if (posY >= homeY) {
                    posY = homeY;
                    velY = 0;
                    masherState = MasherState::Waiting;
                    waitStart = now;
                }
                break;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        if (state == State::Dead) return;
        if (masherState == MasherState::Waiting) return;
        
        SpriteFrame frame = EnemyFrames::GetMasher(currentFrame);
        RenderFrame(viewWindow, frame, facingRight);
    }
};

//============================================================================
//BATBRAIN - Hangs from ceiling, swoops down when player approaches
//============================================================================

class Batbrain : public Enemy {
private:
    enum class BatState { Hanging, Swooping, Flying };
    BatState batState = BatState::Hanging;
    
    float homeX, homeY;
    float playerX = 0, playerY = 0;
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    
public:
    Batbrain(float x, float y) : Enemy(Type::Masher, x, y, 32, 24) {  //reuse Masher type
        homeX = x;
        homeY = y;
        lastFrameTime = engine::GetSystemTime();
    }
    
    void SetPlayerPosition(float px, float py) override {
        playerX = px;
        playerY = py;
    }
    
    void Update() override {
        if (state == State::Dying) { UpdateDeathAnimation(); return; }
        if (state == State::Dead) return;
        
        uint64_t now = engine::GetSystemTime();
        
//animate wings
        if (now - lastFrameTime >= 100) {
            animFrame = (animFrame + 1) % 2;
            lastFrameTime = now;
        }
        
        switch (batState) {
            case BatState::Hanging:
                {
                    float dx = playerX - posX;
                    float dy = playerY - posY;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    
                    if (dist < 120.0f && playerY > posY) {
//player is below and close - swoop!
                        batState = BatState::Swooping;
                        facingRight = (dx > 0);
                    }
                }
                break;
                
            case BatState::Swooping:
//dive toward player
                {
                    float dx = playerX - posX;
                    float dy = playerY - posY;
                    float dist = std::sqrt(dx*dx + dy*dy);
                    
                    if (dist > 1.0f) {
                        posX += (dx / dist) * 3.0f;
                        posY += (dy / dist) * 2.5f;
                    }
                    
                    facingRight = (dx > 0);
                    
//after diving past player Y or timeout, start flying
                    if (posY > playerY + 30 || posY > homeY + 200) {
                        batState = BatState::Flying;
                    }
                }
                break;
                
            case BatState::Flying:
//fly in a wave pattern away
                posX += facingRight ? 2.0f : -2.0f;
                posY += std::sin(now / 150.0f) * 1.5f;
                
//return home if far away
                if (std::abs(posX - homeX) > 300) {
                    posX = homeX;
                    posY = homeY;
                    batState = BatState::Hanging;
                }
                break;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        if (state == State::Dead) return;
        
        SpriteFrame frame = EnemyFrames::GetBatbrain(animFrame);
        RenderFrame(viewWindow, frame, facingRight);
    }
};

//============================================================================
//SUPPORT CLASSES
//============================================================================

//=== ANIMAL TYPES ===
enum class AnimalType {
    Flicky,  //blue bird - most common
    Pocky,  //tan rabbit
    Cucky,  //yellow chicken
    Pecky,  //blue penguin
    Rocky,  //gray seal
    Picky,  //pink pig
    Ricky  //brown squirrel
};

class Animal {
public:
    float x, y, velX, velY;
    bool active = true;
    uint64_t spawnTime;
    AnimalType type;
    bool onGround = false;
    int bounceCount = 0;
    static constexpr float GRAVITY = 800.0f;  //pixels per second^2
    static constexpr float INITIAL_VEL_Y = -300.0f;  //initial upward pop
    static constexpr float HORIZONTAL_SPEED = 100.0f;  //leftward movement
    static constexpr float BOUNCE_DAMPENING = 0.9f;
    static constexpr int MAX_BOUNCES = 3;
    static constexpr uint64_t LIFETIME_MS = 8000;  //8 seconds
    
    Animal(float px, float py) : x(px), y(py) {
        spawnTime = engine::GetSystemTime();
        velY = INITIAL_VEL_Y;
        velX = -HORIZONTAL_SPEED;  //always move left initially
//randomly pick an animal type
        type = static_cast<AnimalType>(rand() % 7);
    }
    
    Animal(float px, float py, AnimalType animalType) : x(px), y(py), type(animalType) {
        spawnTime = engine::GetSystemTime();
        velY = INITIAL_VEL_Y;
        velX = -HORIZONTAL_SPEED;
    }
    
    void Update() {
        if (!active) return;
        
//calculate delta time (assuming ~60fps, use 1/60 if no clock available)
        float dt = 1.0f / 60.0f;
        
//apply gravity
        velY += GRAVITY * dt;
        
        x += velX * dt;
        y += velY * dt;
        
//simple ground collision - bounce at a fixed Y level or use grid
//for now, bounce at a reasonable ground level
        float groundY = 500.0f;  //this should be determined from grid
        if (y > groundY && velY > 0) {
            y = groundY;
            bounceCount++;
            if (bounceCount < MAX_BOUNCES) {
                velY = -velY * BOUNCE_DAMPENING;
//possibly reverse horizontal direction on bounce
                if (rand() % 2 == 0) velX = -velX;
            } else {
//stop bouncing, just hop along
                velY = 0;
                onGround = true;
            }
        }
        
//deactivate after lifetime
        if (engine::GetSystemTime() - spawnTime > LIFETIME_MS) {
            active = false;
        }
    }
    
    void Render(const engine::Rect& viewWindow) {
        if (!active) return;
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(x) - viewWindow.x;
        int screenY = static_cast<int>(y) - viewWindow.y;
        
//get the appropriate animal film based on type
        const char* filmName = GetFilmName();
        auto* animalFilm = ResourceManager::Instance().GetFilm(filmName);
        
        if (animalFilm && animalFilm->GetTotalFrames() >= 3) {
            uint64_t now = engine::GetSystemTime();
            uint64_t elapsed = now - spawnTime;
            
            int frame;
            if (elapsed < 200) {
//first 200ms: show spawn frame (frame 0)
                frame = 0;
            } else {
//after spawn: alternate between frames 1 and 2 only (running animation)
                frame = 1 + ((elapsed / 100) % 2);  //alternates between 1 and 2
            }
            
//flip sprite based on movement direction
//if moving left (velX < 0), flip horizontally (facing left)
//if moving right (velX > 0), show normal sprite (facing right)
            if (velX < 0) {
                animalFilm->DisplayFrameFlipped({screenX, screenY}, static_cast<engine::byte>(frame));
            } else {
                animalFilm->DisplayFrame({screenX, screenY}, static_cast<engine::byte>(frame));
            }
        } else {
//fallback: colored box based on type
            engine::Color color = GetFallbackColor();
            gfx.DrawRect({screenX, screenY, 16, 24}, color, true);
        }
    }
    
private:
    const char* GetFilmName() const {
        switch (type) {
            case AnimalType::Flicky: return "flicky";
            case AnimalType::Pocky:  return "pocky";
            case AnimalType::Cucky:  return "cucky";
            case AnimalType::Pecky:  return "pecky";
            case AnimalType::Rocky:  return "rocky";
            case AnimalType::Picky:  return "picky";
            case AnimalType::Ricky:  return "ricky";
            default: return "flicky";
        }
    }
    
    engine::Color GetFallbackColor() const {
        switch (type) {
            case AnimalType::Flicky: return engine::MakeColor(0, 100, 255);  //blue bird
            case AnimalType::Pocky:  return engine::MakeColor(210, 180, 140);  //tan rabbit
            case AnimalType::Cucky:  return engine::MakeColor(255, 255, 0);  //yellow chicken
            case AnimalType::Pecky:  return engine::MakeColor(100, 149, 237);  //blue penguin
            case AnimalType::Rocky:  return engine::MakeColor(128, 128, 128);  //gray seal
            case AnimalType::Picky:  return engine::MakeColor(255, 182, 193);  //pink pig
            case AnimalType::Ricky:  return engine::MakeColor(139, 90, 43);  //brown squirrel
            default: return engine::MakeColor(0, 100, 255);
        }
    }
};

class AnimalManager {
    std::vector<Animal> animals;
    int currentZone = 1;  //default to Green Hill Zone
    engine::GridLayer* gridLayer = nullptr;
    static constexpr size_t MAX_ANIMALS = 20;  //limit particle count for performance
    
public:
    AnimalManager() = default;
    static AnimalManager& Instance() { static AnimalManager instance; return instance; }
    
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetZone(int zone) { currentZone = zone; }
    
//get zone-appropriate animal type
    AnimalType GetZoneAnimal() {
        switch (currentZone) {
            case 1:  //green Hill Zone
                return (rand() % 2 == 0) ? AnimalType::Flicky : AnimalType::Pocky;
            case 2:  //marble Zone
                return (rand() % 2 == 0) ? AnimalType::Cucky : AnimalType::Ricky;
            case 3:  //spring Yard Zone
                return (rand() % 2 == 0) ? AnimalType::Ricky : AnimalType::Flicky;
            case 4:  //labyrinth Zone
                return (rand() % 2 == 0) ? AnimalType::Pecky : AnimalType::Rocky;
            case 5:  //star Light Zone
                return (rand() % 2 == 0) ? AnimalType::Flicky : AnimalType::Cucky;
            case 6:  //scrap Brain Zone
                return (rand() % 2 == 0) ? AnimalType::Picky : AnimalType::Ricky;
            default:
                return AnimalType::Flicky;
        }
    }
    
    void Spawn(float x, float y) { 
        if (animals.size() >= MAX_ANIMALS) return;
        animals.emplace_back(x, y, GetZoneAnimal());
    }
    
    void SpawnAnimal(float x, float y) { Spawn(x, y); }
    
    void SpawnSpecific(float x, float y, AnimalType type) {
        if (animals.size() >= MAX_ANIMALS) return;
        animals.emplace_back(x, y, type);
    }
    
    void Update() {
        for (auto& a : animals) {
            if (gridLayer) {
                UpdateWithGrid(a);
            } else {
                a.Update();
            }
        }
//remove inactive animals
        animals.erase(std::remove_if(animals.begin(), animals.end(),
            [](const Animal& a) { return !a.active; }), animals.end());
    }
    
    void UpdateWithGrid(Animal& a) {
        if (!a.active) return;
        
        float dt = 1.0f / 60.0f;
        
//apply gravity
        a.velY += Animal::GRAVITY * dt;
        
        float newX = a.x + a.velX * dt;
        float newY = a.y + a.velY * dt;
        
//use a proper Rect for ground check (x, y, w, h)
        engine::Rect feetBox = {static_cast<int>(newX), static_cast<int>(newY + 24), 16, 1};
        if (gridLayer->IsOnSolidGround(feetBox)) {
            if (a.velY > 0) {  //falling
                a.bounceCount++;
                if (a.bounceCount < Animal::MAX_BOUNCES) {
                    a.velY = -a.velY * Animal::BOUNCE_DAMPENING;
                    if (rand() % 2 == 0) a.velX = -a.velX;
                } else {
                    a.velY = 0;
                    a.onGround = true;
                }
                newY = a.y;  //don't go through ground
            }
        }
        
        a.x = newX;
        a.y = newY;
        
//deactivate after lifetime
        if (engine::GetSystemTime() - a.spawnTime > Animal::LIFETIME_MS) {
            a.active = false;
        }
    }
    
    void Render(const engine::Rect& viewWindow) { 
        for (auto& a : animals) {
            if (a.x >= viewWindow.x - 200 && a.x <= viewWindow.x + viewWindow.w + 200 &&
                a.y >= viewWindow.y - 200 && a.y <= viewWindow.y + viewWindow.h + 200) {
                a.Render(viewWindow);
            }
        }
    }
    
    void Clear() { animals.clear(); }
    size_t Count() const { return animals.size(); }
};

class EnemyManager {
    std::vector<std::unique_ptr<Enemy>> enemies;
    engine::GridLayer* gridLayer = nullptr;
    std::function<void(int)> onEnemyKilled;
    
public:
    void SetGridLayer(engine::GridLayer* grid) {
        gridLayer = grid;
        for (auto& e : enemies) e->SetGridLayer(grid);
    }
    
    void AddEnemy(std::unique_ptr<Enemy> e) {
        if (gridLayer) e->SetGridLayer(gridLayer);
        e->SetOnDeath([](float x, float y) { AnimalManager::Instance().Spawn(x, y); });
        enemies.push_back(std::move(e));
    }
    
    void AddMotobug(float x, float y) { AddEnemy(std::make_unique<Motobug>(x, y)); }
    void AddCrabmeat(float x, float y) { AddEnemy(std::make_unique<Crabmeat>(x, y)); }
    void AddBuzzBomber(float x, float y) { AddEnemy(std::make_unique<BuzzBomber>(x, y)); }
    void AddBuzzBomber(float x, float y, float /*range*/) { AddBuzzBomber(x, y);  }
    void AddMasher(float x, float y) { AddEnemy(std::make_unique<Masher>(x, y)); }
    void AddMasher(float x, float y, int /*interval*/) { AddMasher(x, y);  }
    void AddBatbrain(float x, float y) { AddEnemy(std::make_unique<Batbrain>(x, y)); }
    
    void SetOnEnemyKilled(std::function<void(int)> cb) { onEnemyKilled = cb; }
    void SetOnAnimalFreed(std::function<void(float, float)>) {}
    
    void Update(float playerX, float playerY, const engine::Rect& viewWindow) {
        for (auto& e : enemies) {
            float ex = e->GetX();
            float ey = e->GetY();
            if (ex >= viewWindow.x - 300 && ex <= viewWindow.x + viewWindow.w + 300 &&
                ey >= viewWindow.y - 300 && ey <= viewWindow.y + viewWindow.h + 300) {
                e->SetPlayerPosition(playerX, playerY);
//pass screen bounds to BuzzBombers for off-screen detection
                if (auto* buzz = dynamic_cast<BuzzBomber*>(e.get())) {
                    buzz->SetScreenBounds(
                        static_cast<float>(viewWindow.x),
                        static_cast<float>(viewWindow.y),
                        static_cast<float>(viewWindow.x + viewWindow.w),
                        static_cast<float>(viewWindow.y + viewWindow.h)
                    );
                }
                e->Update();
            }
        }
        ProjectileManager::Instance().Update();
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const std::unique_ptr<Enemy>& e) { return e->IsDead(); }), enemies.end());
    }
    
//overload for backward compatibility
    void Update(float playerX, float playerY) {
        engine::Rect defaultView = {0, 0, 320, 224};
        Update(playerX, playerY, defaultView);
    }
    
    void Render(const engine::Rect& viewWindow) {
        for (auto& e : enemies) {
            float ex = e->GetX();
            float ey = e->GetY();
            if (ex >= viewWindow.x - 200 && ex <= viewWindow.x + viewWindow.w + 200 &&
                ey >= viewWindow.y - 200 && ey <= viewWindow.y + viewWindow.h + 200) {
                e->Render(viewWindow);
            }
        }
        ProjectileManager::Instance().Render(viewWindow);
    }
    
    int CheckCollision(const engine::Rect& playerBox, bool canKillInBall, bool canKillOnContact, bool damageImmune) {
        bool canKill = canKillInBall || canKillOnContact;
        for (auto& e : enemies) {
            if (!e->IsActive()) continue;
            if (e->CollidesWith(playerBox)) {
                if (canKill) {
                    int score = e->GetScore();
                    e->Kill();
                    if (onEnemyKilled) onEnemyKilled(score);
                    return score;
                } else if (!damageImmune) {
                    return -1;
                }
            }
        }
        if (!damageImmune && ProjectileManager::Instance().CheckPlayerCollision(playerBox)) return -1;
        return 0;
    }
    
    bool CheckPlayerAttack(const engine::Rect& playerBox, bool playerInBallState, int& score) {
        for (auto& e : enemies) {
            if (e->CanBeDamagedBy(playerBox, playerInBallState)) {
                score = e->GetScore();
                e->Kill();
                if (onEnemyKilled) onEnemyKilled(score);
                return true;
            }
        }
        return false;
    }
    
    bool CheckPlayerHurt(const engine::Rect& playerBox, bool playerInBallState) {
        for (auto& e : enemies) {
            if (e->DamagesPlayer(playerBox, playerInBallState)) return true;
        }
        return ProjectileManager::Instance().CheckPlayerCollision(playerBox);
    }
    
    void Clear() { enemies.clear(); ProjectileManager::Instance().Clear(); }
    size_t GetEnemyCount() const { return enemies.size(); }
};

}  //namespace app

#endif  //ENEMY_HPP
