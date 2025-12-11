#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Engine.hpp"
#include <functional>
#include <cmath>

namespace app {

// Forward declaration
class SonicPlayer;

// Base enemy class
class Enemy {
public:
    enum class State {
        Active,
        Dying,      // Death animation
        Dead        // Ready for removal
    };
    
    enum class Type {
        Motobug,
        Crabmeat,
        BuzzBomber,
        Masher,
        // New enemy types from sprite sheet
        NewtronBlue,
        NewtronGreen,
        Bomb,
        Caterkiller,
        Batbrain,
        Burrobot,
        Roller,
        Jaws,
        BallHog,
        Orbinaut
    };

protected:
    float posX = 0;
    float posY = 0;
    float velX = 0;
    float velY = 0;
    
    int width = 32;
    int height = 32;
    
    Type type;
    State state = State::Active;
    bool facingRight = false;
    
    int currentFrame = 0;
    uint64_t lastFrameTime = 0;
    uint64_t deathTime = 0;
    
    int scoreValue = 100;
    
    engine::GridLayer* gridLayer = nullptr;
    
    // Death animation callback
    std::function<void(float, float)> onDeath;  // Spawn animal at position

public:
    Enemy(Type t, float x, float y, int w, int h)
        : posX(x), posY(y), width(w), height(h), type(t) {
        lastFrameTime = engine::GetSystemTime();
    }
    
    virtual ~Enemy() = default;
    
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetOnDeath(std::function<void(float, float)> callback) { onDeath = callback; }
    
    virtual void Update() = 0;
    virtual void Render(const engine::Rect& viewWindow) = 0;
    
    // Check if player can damage this enemy (player in ball state)
    virtual bool CanBeDamagedBy(const engine::Rect& playerBox, bool playerInBallState) {
        if (state != State::Active) return false;
        if (!playerInBallState) return false;
        return CollidesWith(playerBox);
    }
    
    // Check if this enemy damages the player
    virtual bool DamagesPlayer(const engine::Rect& playerBox, bool playerInBallState) {
        if (state != State::Active) return false;
        if (playerInBallState) return false;  // Ball state kills enemy, not player
        return CollidesWith(playerBox);
    }
    
    void Kill() {
        if (state == State::Active) {
            state = State::Dying;
            deathTime = engine::GetSystemTime();
            velY = -5.0f;  // Pop up
            if (onDeath) {
                onDeath(posX + width/2, posY);
            }
        }
    }
    
    bool IsDead() const { return state == State::Dead; }
    bool IsActive() const { return state == State::Active; }
    int GetScore() const { return scoreValue; }
    Type GetType() const { return type; }
    
    float GetX() const { return posX; }
    float GetY() const { return posY; }
    
    engine::Rect GetBoundingBox() const {
        return {
            static_cast<int>(posX),
            static_cast<int>(posY),
            width,
            height
        };
    }
    
    bool CollidesWith(const engine::Rect& other) const {
        engine::Rect box = GetBoundingBox();
        return !(other.x + other.w < box.x ||
                 box.x + box.w < other.x ||
                 other.y + other.h < box.y ||
                 box.y + box.h < other.y);
    }
    
protected:
    void UpdateDeathAnimation() {
        if (state == State::Dying) {
            velY += 0.3f;
            posY += velY;
            
            // Remove after falling off screen or timeout
            uint64_t elapsed = engine::GetSystemTime() - deathTime;
            if (elapsed > 2000 || posY > 2000) {
                state = State::Dead;
            }
        }
    }
    
    bool IsOnGround() {
        if (!gridLayer) return true;
        
        engine::Rect box = GetBoundingBox();
        int dy = 1;
        int dx = 0;
        gridLayer->FilterGridMotion(box, &dx, &dy);
        return dy == 0;
    }
    
    bool WouldFallOff(float direction) {
        if (!gridLayer) return false;
        
        // Check a bit ahead and below
        int checkX = static_cast<int>(posX + (direction > 0 ? width + 4 : -4));
        int checkY = static_cast<int>(posY + height + 4);
        
        engine::Rect checkBox = {checkX, checkY, 4, 4};
        int dy = 1;
        int dx = 0;
        gridLayer->FilterGridMotion(checkBox, &dx, &dy);
        return dy != 0;  // Would fall
    }
    
    bool WouldHitWall(float direction) {
        if (!gridLayer) return false;
        
        int dx = static_cast<int>(direction);
        int dy = 0;
        engine::Rect box = GetBoundingBox();
        gridLayer->FilterGridMotion(box, &dx, &dy);
        return dx == 0;
    }
};

// Motobug / Motora - ground patrol enemy
// PDF: "A bug on a motor that goes left and right. Stops for a while and turns around
//       to continue the loop if it meets an obstacle or a cliff."
class Motobug : public Enemy {
private:
    float speed = 1.5f;
    uint64_t pauseTime = 0;
    bool isPaused = false;
    static constexpr uint64_t PAUSE_DURATION = 500;
    static constexpr int FRAME_COUNT = 2;
    static constexpr uint64_t FRAME_DELAY = 150;

public:
    Motobug(float x, float y) 
        : Enemy(Type::Motobug, x, y, 40, 32) {
        facingRight = false;
        velX = -speed;
        scoreValue = 100;
    }
    
    void Update() override {
        if (state == State::Dying) {
            UpdateDeathAnimation();
            return;
        }
        
        if (state != State::Active) return;
        
        uint64_t now = engine::GetSystemTime();
        
        // Animate
        if (now - lastFrameTime >= FRAME_DELAY) {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            lastFrameTime = now;
        }
        
        // Pause logic
        if (isPaused) {
            if (now - pauseTime >= PAUSE_DURATION) {
                isPaused = false;
                // Turn around
                facingRight = !facingRight;
                velX = facingRight ? speed : -speed;
            }
            return;
        }
        
        // Movement
        posX += velX;
        
        // Check for obstacles or cliffs
        if (WouldHitWall(velX) || WouldFallOff(velX)) {
            isPaused = true;
            pauseTime = now;
            velX = 0;
        }
        
        // Apply gravity
        if (!IsOnGround()) {
            velY += 0.3f;
            posY += velY;
        } else {
            velY = 0;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        // Skip if off screen
        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) {
            return;
        }
        
        engine::Color bodyColor;
        if (state == State::Dying) {
            bodyColor = engine::MakeColor(255, 100, 100);  // Flash red
        } else {
            bodyColor = engine::MakeColor(200, 50, 50);  // Red body
        }
        
        // Draw Motobug body
        gfx.DrawRect({screenX + 5, screenY, width - 10, height - 8}, bodyColor, true);
        
        // Wheel
        int wheelOffset = (currentFrame == 0) ? 0 : 2;
        gfx.DrawRect({screenX + 8, screenY + height - 12 + wheelOffset, 10, 10}, 
                     engine::MakeColor(50, 50, 50), true);
        gfx.DrawRect({screenX + width - 18, screenY + height - 12 - wheelOffset, 10, 10}, 
                     engine::MakeColor(50, 50, 50), true);
        
        // Eye
        int eyeX = facingRight ? screenX + width - 12 : screenX + 6;
        gfx.DrawRect({eyeX, screenY + 6, 6, 6}, engine::MakeColor(255, 255, 255), true);
        
        // Antenna
        gfx.DrawRect({screenX + width/2 - 1, screenY - 6, 2, 8}, 
                     engine::MakeColor(100, 100, 100), true);
        
        // Border
        gfx.DrawRect({screenX, screenY, width, height}, engine::MakeColor(0, 0, 0), false);
    }
};

// Crabmeat - shoots projectiles
// PDF: "Move like crabs left/right till obstacle/cliff, then turn. If Sonic is close,
//       throw two projectiles, one from each hand that follow a curved path (not targeting Sonic)"
class Crabmeat : public Enemy {
private:
    float speed = 0.8f;
    uint64_t lastShootTime = 0;
    static constexpr uint64_t SHOOT_COOLDOWN = 3000;
    bool canSeePlayer = false;
    float playerX = 0;

    // Projectile system - two curved projectiles (one from each claw)
    struct Projectile {
        float x, y;
        float velX, velY;
        bool active = false;
    };
    Projectile projectileLeft;   // From left claw
    Projectile projectileRight;  // From right claw

    static constexpr float PROJ_GRAVITY = 0.15f;  // For curved path
    static constexpr float PROJ_SPEED_X = 3.0f;
    static constexpr float PROJ_SPEED_Y = -4.0f;  // Initial upward velocity for arc

public:
    Crabmeat(float x, float y)
        : Enemy(Type::Crabmeat, x, y, 48, 32) {
        facingRight = false;
        velX = -speed;
        scoreValue = 100;
    }

    void SetPlayerPosition(float px) {
        playerX = px;
        canSeePlayer = std::abs(px - posX) < 200;
    }

    void Update() override {
        if (state == State::Dying) {
            UpdateDeathAnimation();
            // Deactivate projectiles on death
            projectileLeft.active = false;
            projectileRight.active = false;
            return;
        }

        if (state != State::Active) return;

        uint64_t now = engine::GetSystemTime();

        // Animate
        if (now - lastFrameTime >= 200) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }

        // Movement - left/right until obstacle or cliff
        posX += velX;

        // Check for obstacles or cliffs - then turn
        if (WouldHitWall(velX) || WouldFallOff(velX)) {
            facingRight = !facingRight;
            velX = facingRight ? speed : -speed;
        }

        // PDF: "If Sonic stands close to them they throw two projectiles"
        // Curved path projectiles (not targeting Sonic)
        if (canSeePlayer && now - lastShootTime >= SHOOT_COOLDOWN) {
            lastShootTime = now;

            // Spawn left claw projectile (goes left with arc)
            projectileLeft.active = true;
            projectileLeft.x = posX - 4;
            projectileLeft.y = posY + height / 2;
            projectileLeft.velX = -PROJ_SPEED_X;
            projectileLeft.velY = PROJ_SPEED_Y;

            // Spawn right claw projectile (goes right with arc)
            projectileRight.active = true;
            projectileRight.x = posX + width + 4;
            projectileRight.y = posY + height / 2;
            projectileRight.velX = PROJ_SPEED_X;
            projectileRight.velY = PROJ_SPEED_Y;
        }

        // Update projectiles - curved path (parabolic arc)
        auto updateProjectile = [&](Projectile& proj) {
            if (!proj.active) return;
            proj.velY += PROJ_GRAVITY;  // Gravity creates curved path
            proj.x += proj.velX;
            proj.y += proj.velY;
            // Deactivate if too far or below ground
            if (std::abs(proj.x - posX) > 250 || proj.y > posY + 200) {
                proj.active = false;
            }
        };
        updateProjectile(projectileLeft);
        updateProjectile(projectileRight);
    }

    bool DamagesPlayer(const engine::Rect& playerBox, bool inBall) override {
        if (state != State::Active) return false;
        if (inBall) return false;

        // Body collision
        if (CollidesWith(playerBox)) return true;

        // Check projectile collisions
        auto checkProjectileHit = [&](const Projectile& proj) -> bool {
            if (!proj.active) return false;
            engine::Rect projBox = {static_cast<int>(proj.x) - 4, static_cast<int>(proj.y) - 4, 8, 8};
            return playerBox.x < projBox.x + projBox.w && playerBox.x + playerBox.w > projBox.x &&
                   playerBox.y < projBox.y + projBox.h && playerBox.y + playerBox.h > projBox.y;
        };

        if (checkProjectileHit(projectileLeft)) {
            projectileLeft.active = false;
            return true;
        }
        if (checkProjectileHit(projectileRight)) {
            projectileRight.active = false;
            return true;
        }
        return false;
    }

    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();

        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;

        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) {
            return;
        }

        engine::Color bodyColor = state == State::Dying ?
            engine::MakeColor(255, 150, 150) : engine::MakeColor(255, 100, 50);

        // Body
        gfx.DrawRect({screenX + 8, screenY + 8, width - 16, height - 8}, bodyColor, true);

        // Claws (animate open/close)
        int clawOffset = (currentFrame == 0) ? 0 : 4;
        gfx.DrawRect({screenX, screenY + 10 - clawOffset, 12, 16}, bodyColor, true);
        gfx.DrawRect({screenX + width - 12, screenY + 10 + clawOffset, 12, 16}, bodyColor, true);

        // Eyes
        gfx.DrawRect({screenX + 14, screenY + 4, 6, 6}, engine::MakeColor(255, 255, 255), true);
        gfx.DrawRect({screenX + width - 20, screenY + 4, 6, 6}, engine::MakeColor(255, 255, 255), true);

        // Draw projectiles (energy balls)
        auto drawProjectile = [&](const Projectile& proj) {
            if (!proj.active) return;
            int px = static_cast<int>(proj.x) - viewWindow.x;
            int py = static_cast<int>(proj.y) - viewWindow.y;
            // Yellow energy ball
            gfx.DrawRect({px - 4, py - 4, 8, 8}, engine::MakeColor(255, 200, 0), true);
            gfx.DrawRect({px - 2, py - 2, 4, 4}, engine::MakeColor(255, 255, 200), true);
        };
        drawProjectile(projectileLeft);
        drawProjectile(projectileRight);

        // Border
        gfx.DrawRect({screenX, screenY, width, height}, engine::MakeColor(0, 0, 0), false);
    }
};

// BuzzBomber - Flying bee enemy
// PDF: "Fly toward Sonic, fire projectile at Sonic's feet. If not killed, fly off screen then flip.
//       If Sonic skips them (1 tile away), reset. Don't chase if Sonic moves too fast."
class BuzzBomber : public Enemy {
private:
    enum class BuzzState { Patrol, Attacking, FlyingAway, Resetting };
    BuzzState buzzState = BuzzState::Patrol;

    float speed = 2.0f;
    float homeX = 0;  // Spawn position for reset
    float homeY = 0;
    float hoverOffset = 0;
    float hoverSpeed = 0.05f;

    float playerX = 0;
    float playerY = 0;
    bool canSeePlayer = false;

    uint64_t lastShootTime = 0;
    static constexpr uint64_t SHOOT_COOLDOWN = 2500;
    static constexpr int FRAME_COUNT = 2;
    static constexpr uint64_t FRAME_DELAY = 80;

    // Projectile that targets Sonic's feet
    struct Projectile {
        float x, y;
        float velX, velY;
        float targetX, targetY;  // Target position (Sonic's feet)
        bool active = false;
    };
    Projectile projectile;

    // Reset distance - PDF says "1 tile away" (tile = ~128px in this engine)
    static constexpr float RESET_DISTANCE = 256.0f;  // 2 tiles for better gameplay

    // Patrol bounds
    float patrolLeft = 0;
    float patrolRight = 0;

public:
    BuzzBomber(float x, float y, float patrolRange = 200.0f)
        : Enemy(Type::BuzzBomber, x, y, 46, 32) {
        homeX = x;
        homeY = y;
        facingRight = false;
        velX = -speed;
        scoreValue = 100;

        patrolLeft = x - patrolRange / 2;
        patrolRight = x + patrolRange / 2;
    }

    void SetPlayerPosition(float px, float py) {
        playerX = px;
        playerY = py;
        float dist = std::sqrt((px - posX) * (px - posX) + (py - posY) * (py - posY));
        canSeePlayer = dist < 250;

        // PDF: "If Sonic skips them by moving too fast... they reset"
        float distFromHome = std::abs(px - homeX);
        if (distFromHome > RESET_DISTANCE && buzzState != BuzzState::Resetting) {
            buzzState = BuzzState::Resetting;
        }
    }

    void Update() override {
        if (state == State::Dying) {
            UpdateDeathAnimation();
            projectile.active = false;
            return;
        }

        if (state != State::Active) return;

        uint64_t now = engine::GetSystemTime();

        // Wing animation
        if (now - lastFrameTime >= FRAME_DELAY) {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            lastFrameTime = now;
        }

        switch (buzzState) {
            case BuzzState::Patrol:
                // Normal patrol behavior
                hoverOffset += hoverSpeed;
                posY = homeY + std::sin(hoverOffset) * 10.0f;
                posX += velX;

                // Turn at patrol bounds
                if (posX <= patrolLeft) {
                    posX = patrolLeft;
                    facingRight = true;
                    velX = speed;
                } else if (posX >= patrolRight) {
                    posX = patrolRight;
                    facingRight = false;
                    velX = -speed;
                }

                // PDF: "fly toward Sonic and fire projectile at Sonic's feet"
                if (canSeePlayer && playerY > posY) {
                    buzzState = BuzzState::Attacking;
                    facingRight = (playerX > posX);
                }
                break;

            case BuzzState::Attacking:
                // Fly toward player
                facingRight = (playerX > posX);
                velX = facingRight ? speed : -speed;
                posX += velX;

                // Slight hover
                hoverOffset += hoverSpeed;
                posY = homeY + std::sin(hoverOffset) * 5.0f;

                // Fire at Sonic's feet when close
                if (std::abs(playerX - posX) < 80 && now - lastShootTime >= SHOOT_COOLDOWN) {
                    lastShootTime = now;

                    // PDF: "fire a projectile that is targeted at sonic's feet"
                    projectile.active = true;
                    projectile.x = posX + width / 2;
                    projectile.y = posY + height;
                    projectile.targetX = playerX;
                    projectile.targetY = playerY + 32;  // Target feet area

                    // Calculate velocity toward target
                    float dx = projectile.targetX - projectile.x;
                    float dy = projectile.targetY - projectile.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist > 0) {
                        float projSpeed = 4.0f;
                        projectile.velX = (dx / dist) * projSpeed;
                        projectile.velY = (dy / dist) * projSpeed;
                    }

                    // After shooting, fly away
                    buzzState = BuzzState::FlyingAway;
                }
                break;

            case BuzzState::FlyingAway:
                // PDF: "fly till they go off screen then they flip"
                posX += velX * 1.5f;  // Fly faster when leaving

                // Check if off screen (simplified check)
                if (posX < homeX - 400 || posX > homeX + 400) {
                    // Flip and try again
                    facingRight = !facingRight;
                    velX = facingRight ? speed : -speed;
                    buzzState = BuzzState::Patrol;
                    posY = homeY;
                }
                break;

            case BuzzState::Resetting:
                // PDF: "they don't try to chase him, instead they reset"
                // Return to home position
                float dx = homeX - posX;
                float dy = homeY - posY;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist < 5.0f) {
                    posX = homeX;
                    posY = homeY;
                    buzzState = BuzzState::Patrol;
                    facingRight = false;
                    velX = -speed;
                } else {
                    // Fly back home
                    posX += (dx / dist) * speed;
                    posY += (dy / dist) * speed;
                }
                break;
        }

        // Update projectile
        if (projectile.active) {
            projectile.x += projectile.velX;
            projectile.y += projectile.velY;
            // Deactivate if past target or too far
            if (projectile.y > playerY + 100 || std::abs(projectile.x - homeX) > 400) {
                projectile.active = false;
            }
        }
    }

    bool DamagesPlayer(const engine::Rect& playerBox, bool inBall) override {
        if (state != State::Active) return false;
        if (inBall) return false;

        // Body collision
        if (CollidesWith(playerBox)) return true;

        // Projectile collision
        if (projectile.active) {
            engine::Rect projBox = {static_cast<int>(projectile.x) - 4, static_cast<int>(projectile.y) - 4, 8, 8};
            if (playerBox.x < projBox.x + projBox.w && playerBox.x + playerBox.w > projBox.x &&
                playerBox.y < projBox.y + projBox.h && playerBox.y + playerBox.h > projBox.y) {
                projectile.active = false;
                return true;
            }
        }
        return false;
    }

    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();

        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;

        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) {
            return;
        }

        engine::Color bodyColor = state == State::Dying ?
            engine::MakeColor(255, 200, 100) : engine::MakeColor(255, 200, 0);
        engine::Color stripeColor = engine::MakeColor(40, 40, 40);

        // Body
        gfx.DrawRect({screenX + 8, screenY + 10, width - 16, height - 14}, bodyColor, true);

        // Stripes
        gfx.DrawRect({screenX + 14, screenY + 12, width - 28, 4}, stripeColor, true);
        gfx.DrawRect({screenX + 14, screenY + 20, width - 28, 4}, stripeColor, true);

        // Head
        int headX = facingRight ? screenX + width - 16 : screenX;
        gfx.DrawRect({headX, screenY + 8, 16, 16}, bodyColor, true);

        // Eyes
        int eyeX = facingRight ? headX + 8 : headX + 2;
        gfx.DrawRect({eyeX, screenY + 10, 6, 6}, engine::MakeColor(255, 0, 0), true);

        // Wings (animate)
        int wingY = (currentFrame == 0) ? screenY : screenY + 4;
        gfx.DrawRect({screenX + 12, wingY - 6, 22, 8},
                     engine::MakeColor(200, 200, 255, 180), true);

        // Stinger
        int stingerX = facingRight ? screenX - 4 : screenX + width;
        gfx.DrawRect({stingerX, screenY + height/2, 6, 3}, stripeColor, true);

        // Projectile (targeting Sonic's feet)
        if (projectile.active) {
            int px = static_cast<int>(projectile.x) - viewWindow.x;
            int py = static_cast<int>(projectile.y) - viewWindow.y;
            gfx.DrawRect({px - 4, py - 4, 8, 8}, engine::MakeColor(255, 100, 0), true);
            gfx.DrawRect({px - 2, py - 2, 4, 4}, engine::MakeColor(255, 200, 100), true);
        }

        // Border
        gfx.DrawRect({screenX, screenY, width, height}, engine::MakeColor(0, 0, 0), false);
    }
};

// Masher - Jumping piranha enemy (jumps from water/bridges)
// PDF: "go up and down, max height 64 pixels above bridge, lowest just beneath screen"
class Masher : public Enemy {
private:
    float jumpForce = -8.0f;  // Adjusted for 64px max height
    float gravity = 0.5f;
    float homeY = 0;  // Starting position (water/bridge surface)

    bool isJumping = false;
    uint64_t lastJumpTime = 0;
    uint64_t jumpCooldown = 2000;  // Time between jumps

    float playerX = 0;
    bool playerNearby = false;

    // PDF constraint: max 64 pixels above bridge
    static constexpr float MAX_JUMP_HEIGHT = 64.0f;
    float maxY = 0;  // Maximum Y position (minimum screen position since Y increases downward)

    static constexpr int FRAME_COUNT = 2;
    static constexpr uint64_t FRAME_DELAY = 100;

public:
    Masher(float x, float y, uint64_t cooldown = 2000)
        : Enemy(Type::Masher, x, y, 24, 40) {
        homeY = y;
        maxY = homeY - MAX_JUMP_HEIGHT;  // 64 pixels above home position
        jumpCooldown = cooldown;
        scoreValue = 100;
        velY = 0;
    }

    void SetPlayerPosition(float px) {
        playerX = px;
        playerNearby = std::abs(px - posX) < 150;
    }

    void Update() override {
        if (state == State::Dying) {
            UpdateDeathAnimation();
            return;
        }

        if (state != State::Active) return;

        uint64_t now = engine::GetSystemTime();

        // Animate
        if (now - lastFrameTime >= FRAME_DELAY) {
            currentFrame = (currentFrame + 1) % FRAME_COUNT;
            lastFrameTime = now;
        }

        if (isJumping) {
            // Apply gravity
            velY += gravity;
            posY += velY;

            // PDF: Constrain to max 64 pixels above bridge
            if (posY < maxY) {
                posY = maxY;
                velY = 0;  // Stop upward momentum at peak
            }

            // Face toward player while jumping
            facingRight = playerX > posX;

            // Back to home position (lowest point - just beneath screen/bridge)
            if (posY >= homeY) {
                posY = homeY;
                velY = 0;
                isJumping = false;
                lastJumpTime = now;
            }
        } else {
            // Wait underwater, jump when player is near and cooldown elapsed
            if (playerNearby && now - lastJumpTime >= jumpCooldown) {
                isJumping = true;
                velY = jumpForce;
            }
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        // Only draw when jumping (underwater otherwise)
        if (!isJumping && state == State::Active) {
            // Draw just a small indicator at water level
            gfx.DrawRect({screenX + width/2 - 4, screenY - 4, 8, 4}, 
                         engine::MakeColor(100, 100, 255), true);
            return;
        }
        
        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) {
            return;
        }
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(150, 50, 150);  // Purple
        
        // Body (fish shape)
        gfx.DrawRect({screenX + 2, screenY + 8, width - 4, height - 16}, bodyColor, true);
        
        // Head/mouth
        int mouthY = screenY + 4;
        int mouthOpen = (currentFrame == 0) ? 8 : 12;
        gfx.DrawRect({screenX, mouthY, width, mouthOpen}, bodyColor, true);
        
        // Teeth
        gfx.DrawRect({screenX + 4, mouthY + mouthOpen - 4, 4, 4}, 
                     engine::MakeColor(255, 255, 255), true);
        gfx.DrawRect({screenX + width - 8, mouthY + mouthOpen - 4, 4, 4}, 
                     engine::MakeColor(255, 255, 255), true);
        
        // Eye
        int eyeX = facingRight ? screenX + width - 10 : screenX + 4;
        gfx.DrawRect({eyeX, screenY + 10, 6, 6}, engine::MakeColor(255, 255, 255), true);
        gfx.DrawRect({eyeX + 2, screenY + 12, 2, 2}, engine::MakeColor(0, 0, 0), true);
        
        // Tail fin
        int tailX = facingRight ? screenX - 6 : screenX + width;
        gfx.DrawRect({tailX, screenY + height/2 - 6, 8, 12}, bodyColor, true);
        
        // Dorsal fin
        gfx.DrawRect({screenX + width/2 - 4, screenY, 8, 10}, bodyColor, true);
        
        // Border
        gfx.DrawRect({screenX, screenY, width, height}, engine::MakeColor(0, 0, 0), false);
    }
};

// ============================================================
// NEWTRON (BLUE) - Chameleon that appears and shoots
// Appears when Sonic approaches, fires projectile
// ============================================================
class NewtronBlue : public Enemy {
private:
    enum class NewtronState { Hidden, Appearing, Visible, Shooting };
    NewtronState nState = NewtronState::Hidden;
    float playerPosX = 0;
    float playerPosY = 0;
    float detectRange = 150.0f;
    uint64_t stateTime = 0;
    uint64_t shootCooldown = 2000;
    uint64_t lastShot = 0;
    
    // Projectile
    bool hasProjectile = false;
    float projX = 0, projY = 0;
    float projVelX = 0;
    
public:
    NewtronBlue(float x, float y) : Enemy(Type::NewtronBlue, x, y, 48, 32) {}
    
    void SetPlayerPosition(float px, float py) { playerPosX = px; playerPosY = py; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        float dist = std::abs(playerPosX - posX);
        
        switch (nState) {
            case NewtronState::Hidden:
                if (dist < detectRange) {
                    nState = NewtronState::Appearing;
                    stateTime = now;
                }
                break;
                
            case NewtronState::Appearing:
                if (now - stateTime > 500) {
                    nState = NewtronState::Visible;
                    facingRight = (playerPosX > posX);
                }
                break;
                
            case NewtronState::Visible:
                if (now - lastShot > shootCooldown && dist < detectRange * 2) {
                    nState = NewtronState::Shooting;
                    stateTime = now;
                    // Fire projectile
                    hasProjectile = true;
                    projX = facingRight ? posX + width : posX;
                    projY = posY + height/2;
                    projVelX = facingRight ? 4.0f : -4.0f;
                    lastShot = now;
                }
                break;
                
            case NewtronState::Shooting:
                if (now - stateTime > 300) {
                    nState = NewtronState::Visible;
                }
                break;
        }
        
        // Update projectile
        if (hasProjectile) {
            projX += projVelX;
            if (std::abs(projX - posX) > 300) hasProjectile = false;
        }
        
        // Animation
        if (now - lastFrameTime > 150) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }
    }
    
    bool DamagesPlayer(const engine::Rect& playerBox, bool inBall) override {
        if (state != State::Active) return false;
        if (nState == NewtronState::Hidden) return false;
        if (inBall) return false;
        
        // Check body collision
        if (CollidesWith(playerBox)) return true;
        
        // Check projectile collision
        if (hasProjectile) {
            engine::Rect projBox = {static_cast<int>(projX), static_cast<int>(projY), 8, 8};
            if (playerBox.x < projBox.x + projBox.w && playerBox.x + playerBox.w > projBox.x &&
                playerBox.y < projBox.y + projBox.h && playerBox.y + playerBox.h > projBox.y) {
                hasProjectile = false;
                return true;
            }
        }
        return false;
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (nState == NewtronState::Hidden && state == State::Active) return;
        
        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) return;
        
        engine::Color bodyColor = engine::MakeColor(50, 100, 200);  // Blue
        if (state == State::Dying) bodyColor = engine::MakeColor(255, 150, 150);
        
        // Body
        gfx.DrawRect({screenX + 8, screenY + 4, width - 16, height - 8}, bodyColor, true);
        // Head
        gfx.DrawRect({screenX + (facingRight ? width - 16 : 0), screenY, 16, 16}, bodyColor, true);
        // Tail
        gfx.DrawRect({screenX + (facingRight ? 0 : width - 12), screenY + 8, 12, 8}, bodyColor, true);
        // Eye
        int eyeX = screenX + (facingRight ? width - 12 : 4);
        gfx.DrawRect({eyeX, screenY + 4, 6, 6}, engine::MakeColor(255, 255, 255), true);
        
        // Projectile
        if (hasProjectile) {
            int px = static_cast<int>(projX) - viewWindow.x;
            int py = static_cast<int>(projY) - viewWindow.y;
            gfx.DrawRect({px - 4, py - 4, 8, 8}, engine::MakeColor(255, 200, 0), true);
        }
    }
};

// ============================================================
// NEWTRON (GREEN) - Chameleon that flies toward player
// Appears when approached, then flies at player
// ============================================================
class NewtronGreen : public Enemy {
private:
    enum class FlyState { Hidden, Appearing, Flying };
    FlyState fState = FlyState::Hidden;
    float playerPosX = 0;
    float detectRange = 120.0f;
    float flySpeed = 3.0f;
    uint64_t stateTime = 0;
    
public:
    NewtronGreen(float x, float y) : Enemy(Type::NewtronGreen, x, y, 48, 24) {}
    
    void SetPlayerPosition(float px) { playerPosX = px; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        float dist = std::abs(playerPosX - posX);
        
        switch (fState) {
            case FlyState::Hidden:
                if (dist < detectRange) {
                    fState = FlyState::Appearing;
                    stateTime = now;
                }
                break;
                
            case FlyState::Appearing:
                if (now - stateTime > 400) {
                    fState = FlyState::Flying;
                    facingRight = (playerPosX > posX);
                    velX = facingRight ? flySpeed : -flySpeed;
                }
                break;
                
            case FlyState::Flying:
                posX += velX;
                // Fly off screen eventually
                break;
        }
        
        if (now - lastFrameTime > 100) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (fState == FlyState::Hidden && state == State::Active) return;
        
        engine::Color bodyColor = engine::MakeColor(50, 180, 50);  // Green
        if (state == State::Dying) bodyColor = engine::MakeColor(255, 150, 150);
        
        // Body (flying pose)
        gfx.DrawRect({screenX + 4, screenY + 4, width - 8, height - 8}, bodyColor, true);
        // Wings
        int wingOffset = (currentFrame % 2) * 4;
        gfx.DrawRect({screenX + width/2 - 8, screenY - 4 - wingOffset, 16, 8}, bodyColor, true);
        // Eye
        int eyeX = screenX + (facingRight ? width - 12 : 4);
        gfx.DrawRect({eyeX, screenY + 6, 6, 6}, engine::MakeColor(255, 255, 255), true);
    }
};

// ============================================================
// BOMB - Round bomb enemy that walks and explodes
// ============================================================
class Bomb : public Enemy {
private:
    bool exploding = false;
    uint64_t explodeTime = 0;
    float moveSpeed = 1.0f;
    uint64_t pauseTime = 0;
    bool paused = false;
    
public:
    Bomb(float x, float y) : Enemy(Type::Bomb, x, y, 30, 32) {}
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                if (engine::GetSystemTime() - deathTime > 400) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        
        if (exploding) {
            if (now - explodeTime > 300) {
                state = State::Dying;
                deathTime = now;
                if (onDeath) onDeath(posX + width/2, posY);
            }
            return;
        }
        
        // Walk back and forth
        if (!paused) {
            float newX = posX + (facingRight ? moveSpeed : -moveSpeed);
            
            // Check for edges/walls
            bool shouldTurn = false;
            if (gridLayer) {
                int checkX = facingRight ? 
                    static_cast<int>(newX + width + 4) / 8 : 
                    static_cast<int>(newX - 4) / 8;
                int checkY = static_cast<int>(posY + height + 4) / 8;
                int groundCheck = static_cast<int>(posY + height) / 8;
                
                // Check for wall or no ground ahead
                if (gridLayer->GetTile(checkX, groundCheck) != 0 ||
                    gridLayer->GetTile(checkX, checkY) == 0) {
                    shouldTurn = true;
                }
            }
            
            if (shouldTurn) {
                paused = true;
                pauseTime = now;
            } else {
                posX = newX;
            }
        } else {
            if (now - pauseTime > 400) {
                paused = false;
                facingRight = !facingRight;
            }
        }
        
        if (now - lastFrameTime > 120) {
            currentFrame = (currentFrame + 1) % 4;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (screenX + width < 0 || screenX > viewWindow.w) return;
        
        engine::Color bodyColor = exploding ? 
            engine::MakeColor(255, 100, 50) : engine::MakeColor(40, 40, 40);
        
        // Round body
        gfx.DrawRect({screenX + 3, screenY + 3, width - 6, height - 6}, bodyColor, true);
        // Fuse
        gfx.DrawRect({screenX + width/2 - 2, screenY - 4, 4, 8}, engine::MakeColor(200, 100, 50), true);
        // Spark on fuse
        if (currentFrame % 2 == 0) {
            gfx.DrawRect({screenX + width/2 - 1, screenY - 6, 2, 2}, engine::MakeColor(255, 255, 0), true);
        }
        // Eyes
        int eyeY = screenY + height/2 - 4;
        gfx.DrawRect({screenX + 8, eyeY, 5, 5}, engine::MakeColor(255, 255, 255), true);
        gfx.DrawRect({screenX + width - 13, eyeY, 5, 5}, engine::MakeColor(255, 255, 255), true);
        // Feet
        int footY = screenY + height - 6;
        int footOffset = (currentFrame % 2) * 2;
        gfx.DrawRect({screenX + 4 + footOffset, footY, 8, 6}, engine::MakeColor(200, 50, 50), true);
        gfx.DrawRect({screenX + width - 12 - footOffset, footY, 8, 6}, engine::MakeColor(200, 50, 50), true);
        
        // Explosion
        if (exploding) {
            int r = static_cast<int>((engine::GetSystemTime() - explodeTime) / 10);
            gfx.DrawRect({screenX + width/2 - r, screenY + height/2 - r, r*2, r*2}, 
                         engine::MakeColor(255, 200, 50, 150), true);
        }
    }
};

// ============================================================
// CATERKILLER - Segmented caterpillar enemy
// Head is vulnerable, body segments have spikes
// ============================================================
class Caterkiller : public Enemy {
private:
    static constexpr int NUM_SEGMENTS = 4;
    float segmentX[NUM_SEGMENTS];
    float segmentY[NUM_SEGMENTS];
    float moveSpeed = 0.8f;
    int movePhase = 0;
    
public:
    Caterkiller(float x, float y) : Enemy(Type::Caterkiller, x, y, 64, 20) {
        // Initialize segments
        for (int i = 0; i < NUM_SEGMENTS; ++i) {
            segmentX[i] = x - i * 14;
            segmentY[i] = y;
        }
    }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        
        // Move head
        posX += facingRight ? moveSpeed : -moveSpeed;
        
        // Animate body segments (wave motion)
        for (int i = 0; i < NUM_SEGMENTS; ++i) {
            float targetX = (i == 0) ? posX - 14 : segmentX[i-1] - 14;
            segmentX[i] += (targetX - segmentX[i]) * 0.3f;
            segmentY[i] = posY + std::sin((now / 100.0f) + i) * 3.0f;
        }
        
        // Check for turn
        if (gridLayer) {
            int checkX = static_cast<int>(facingRight ? posX + 20 : posX - 4) / 8;
            int checkY = static_cast<int>(posY + height + 4) / 8;
            if (gridLayer->GetTile(checkX, checkY) == 0) {
                facingRight = !facingRight;
            }
        }
        
        if (now - lastFrameTime > 150) {
            currentFrame = (currentFrame + 1) % 3;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        // Draw segments (back to front)
        for (int i = NUM_SEGMENTS - 1; i >= 0; --i) {
            int sx = static_cast<int>(segmentX[i]) - viewWindow.x;
            int sy = static_cast<int>(segmentY[i]) - viewWindow.y;
            
            // Body segment (purple)
            gfx.DrawRect({sx, sy, 14, 14}, engine::MakeColor(150, 50, 150), true);
            // Spike on top
            gfx.DrawRect({sx + 5, sy - 4, 4, 6}, engine::MakeColor(200, 200, 200), true);
        }
        
        // Head (orange/red)
        engine::Color headColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(220, 100, 50);
        gfx.DrawRect({screenX, screenY - 2, 16, 16}, headColor, true);
        // Eyes
        int eyeX = screenX + (facingRight ? 10 : 2);
        gfx.DrawRect({eyeX, screenY + 2, 4, 4}, engine::MakeColor(255, 255, 255), true);
        // Antennae
        gfx.DrawRect({screenX + 4, screenY - 6, 2, 6}, headColor, true);
        gfx.DrawRect({screenX + 10, screenY - 6, 2, 6}, headColor, true);
    }
};

// ============================================================
// BATBRAIN - Bat that hangs from ceiling, swoops when player near
// ============================================================
class Batbrain : public Enemy {
private:
    enum class BatState { Hanging, Swooping, Flying };
    BatState bState = BatState::Hanging;
    float homeY;
    float playerPosX = 0;
    float detectRange = 100.0f;
    float swoopSpeed = 3.0f;
    
public:
    Batbrain(float x, float y) : Enemy(Type::Batbrain, x, y, 32, 24), homeY(y) {}
    
    void SetPlayerPosition(float px) { playerPosX = px; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        float dist = std::abs(playerPosX - posX);
        
        switch (bState) {
            case BatState::Hanging:
                if (dist < detectRange) {
                    bState = BatState::Swooping;
                    facingRight = (playerPosX > posX);
                    velX = facingRight ? swoopSpeed : -swoopSpeed;
                    velY = 2.0f;
                }
                break;
                
            case BatState::Swooping:
                posX += velX;
                posY += velY;
                if (posY > homeY + 80) {
                    bState = BatState::Flying;
                    velY = -1.5f;
                }
                break;
                
            case BatState::Flying:
                posX += velX;
                posY += velY;
                // Fly off screen
                break;
        }
        
        uint64_t now = engine::GetSystemTime();
        if (now - lastFrameTime > 80) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(80, 50, 100);
        
        // Body
        gfx.DrawRect({screenX + 8, screenY + 4, width - 16, height - 8}, bodyColor, true);
        
        // Wings
        int wingY = (bState == BatState::Hanging) ? 0 : ((currentFrame % 2) * 6 - 3);
        gfx.DrawRect({screenX - 4, screenY + 8 + wingY, 12, 8}, bodyColor, true);
        gfx.DrawRect({screenX + width - 8, screenY + 8 - wingY, 12, 8}, bodyColor, true);
        
        // Eyes (red)
        gfx.DrawRect({screenX + 10, screenY + 8, 4, 4}, engine::MakeColor(255, 50, 50), true);
        gfx.DrawRect({screenX + width - 14, screenY + 8, 4, 4}, engine::MakeColor(255, 50, 50), true);
        
        // Ears
        gfx.DrawRect({screenX + 8, screenY, 4, 6}, bodyColor, true);
        gfx.DrawRect({screenX + width - 12, screenY, 4, 6}, bodyColor, true);
    }
};

// ============================================================
// BURROBOT - Drilling robot that pops up from ground
// ============================================================
class Burrobot : public Enemy {
private:
    enum class BurroState { Underground, Rising, Active, Jumping };
    BurroState bState = BurroState::Underground;
    float homeY;
    float playerPosX = 0;
    float detectRange = 80.0f;
    float jumpForce = -8.0f;
    
public:
    Burrobot(float x, float y) : Enemy(Type::Burrobot, x, y, 30, 38), homeY(y) {
        posY = y + 30;  // Start mostly underground
    }
    
    void SetPlayerPosition(float px) { playerPosX = px; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        float dist = std::abs(playerPosX - posX);
        
        switch (bState) {
            case BurroState::Underground:
                if (dist < detectRange) {
                    bState = BurroState::Rising;
                }
                break;
                
            case BurroState::Rising:
                posY -= 2.0f;
                if (posY <= homeY) {
                    posY = homeY;
                    bState = BurroState::Active;
                }
                break;
                
            case BurroState::Active:
                if (dist < detectRange / 2) {
                    bState = BurroState::Jumping;
                    velY = jumpForce;
                    facingRight = (playerPosX > posX);
                    velX = facingRight ? 2.0f : -2.0f;
                }
                break;
                
            case BurroState::Jumping:
                velY += 0.4f;
                posX += velX;
                posY += velY;
                if (posY >= homeY && velY > 0) {
                    posY = homeY;
                    velY = 0;
                    bState = BurroState::Active;
                }
                break;
        }
        
        uint64_t now = engine::GetSystemTime();
        if (now - lastFrameTime > 100) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(100, 100, 150);
        
        // Body
        gfx.DrawRect({screenX + 4, screenY + 8, width - 8, height - 16}, bodyColor, true);
        // Drill (head)
        int drillOffset = (currentFrame % 2) * 2;
        gfx.DrawRect({screenX + 8, screenY - drillOffset, 14, 12}, engine::MakeColor(200, 200, 200), true);
        gfx.DrawRect({screenX + 12, screenY - 4 - drillOffset, 6, 6}, engine::MakeColor(150, 150, 150), true);
        // Treads
        gfx.DrawRect({screenX, screenY + height - 10, 10, 10}, engine::MakeColor(60, 60, 60), true);
        gfx.DrawRect({screenX + width - 10, screenY + height - 10, 10, 10}, engine::MakeColor(60, 60, 60), true);
        // Eyes
        gfx.DrawRect({screenX + 8, screenY + 12, 4, 4}, engine::MakeColor(255, 0, 0), true);
        gfx.DrawRect({screenX + width - 12, screenY + 12, 4, 4}, engine::MakeColor(255, 0, 0), true);
    }
};

// ============================================================
// ROLLER - Armadillo that curls into a ball and rolls
// ============================================================
class Roller : public Enemy {
private:
    enum class RollerState { Idle, Rolling };
    RollerState rState = RollerState::Idle;
    float rollSpeed = 5.0f;
    float playerPosX = 0;
    float detectRange = 150.0f;
    uint64_t rollStartTime = 0;
    
public:
    Roller(float x, float y) : Enemy(Type::Roller, x, y, 40, 30) {}
    
    void SetPlayerPosition(float px) { playerPosX = px; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        float dist = std::abs(playerPosX - posX);
        
        switch (rState) {
            case RollerState::Idle:
                if (dist < detectRange) {
                    rState = RollerState::Rolling;
                    facingRight = (playerPosX > posX);
                    rollStartTime = now;
                    width = 26;
                    height = 26;
                }
                break;
                
            case RollerState::Rolling:
                posX += facingRight ? rollSpeed : -rollSpeed;
                
                // Stop after a while
                if (now - rollStartTime > 2000) {
                    rState = RollerState::Idle;
                    width = 40;
                    height = 30;
                }
                break;
        }
        
        if (now - lastFrameTime > 60) {
            currentFrame = (currentFrame + 1) % 4;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(200, 100, 50);
        
        if (rState == RollerState::Rolling) {
            // Ball form
            gfx.DrawRect({screenX + 3, screenY + 3, 20, 20}, bodyColor, true);
            // Shell pattern
            gfx.DrawRect({screenX + 8, screenY + 5 + (currentFrame % 4) * 2, 10, 4}, 
                         engine::MakeColor(150, 75, 40), true);
        } else {
            // Standing form
            gfx.DrawRect({screenX + 4, screenY + 4, width - 8, height - 8}, bodyColor, true);
            // Shell
            gfx.DrawRect({screenX + 10, screenY, width - 20, height - 10}, 
                         engine::MakeColor(150, 75, 40), true);
            // Head
            int headX = facingRight ? screenX + width - 14 : screenX;
            gfx.DrawRect({headX, screenY + height/2 - 6, 14, 12}, bodyColor, true);
            // Eye
            gfx.DrawRect({headX + (facingRight ? 8 : 2), screenY + height/2 - 2, 4, 4}, 
                         engine::MakeColor(0, 0, 0), true);
        }
    }
};

// ============================================================
// JAWS - Fish enemy that swims in water
// ============================================================
class Jaws : public Enemy {
private:
    float swimSpeed = 2.0f;
    float homeY;
    float amplitude = 20.0f;
    float swimOffset = 0;
    
public:
    Jaws(float x, float y) : Enemy(Type::Jaws, x, y, 32, 20), homeY(y) {}
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.2f;  // Slower fall in water
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 700) state = State::Dead;
            }
            return;
        }
        
        // Swim back and forth
        posX += facingRight ? swimSpeed : -swimSpeed;
        
        // Sinusoidal vertical motion
        swimOffset += 0.05f;
        posY = homeY + std::sin(swimOffset) * amplitude;
        
        // Turn at edges (simplified)
        if (posX < 0 || posX > 10000) facingRight = !facingRight;
        
        uint64_t now = engine::GetSystemTime();
        if (now - lastFrameTime > 150) {
            currentFrame = (currentFrame + 1) % 3;
            lastFrameTime = now;
        }
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (screenX + width < 0 || screenX > viewWindow.w) return;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(50, 100, 200);
        
        // Body
        gfx.DrawRect({screenX + 4, screenY + 2, width - 8, height - 4}, bodyColor, true);
        // Mouth (opening animation)
        int mouthOpen = (currentFrame == 2) ? 6 : 3;
        int mouthX = facingRight ? screenX + width - 8 : screenX;
        gfx.DrawRect({mouthX, screenY + height/2 - mouthOpen/2, 8, mouthOpen}, 
                     engine::MakeColor(100, 50, 50), true);
        // Teeth
        gfx.DrawRect({mouthX + 2, screenY + height/2 - 2, 4, 2}, 
                     engine::MakeColor(255, 255, 255), true);
        // Eye
        int eyeX = facingRight ? screenX + width - 14 : screenX + 6;
        gfx.DrawRect({eyeX, screenY + 4, 4, 4}, engine::MakeColor(255, 255, 255), true);
        // Tail
        int tailX = facingRight ? screenX - 6 : screenX + width - 2;
        gfx.DrawRect({tailX, screenY + height/2 - 6, 8, 12}, bodyColor, true);
    }
};

// ============================================================
// BALLHOG - Pig that throws bouncing bombs
// ============================================================
class BallHog : public Enemy {
private:
    float playerPosX = 0;
    float throwRange = 200.0f;
    uint64_t lastThrow = 0;
    uint64_t throwCooldown = 2500;
    bool throwing = false;
    uint64_t throwAnimTime = 0;
    
    // Thrown ball
    struct ThrownBall {
        float x, y, velX, velY;
        bool active = false;
    };
    ThrownBall ball;
    
public:
    BallHog(float x, float y) : Enemy(Type::BallHog, x, y, 32, 32) {}
    
    void SetPlayerPosition(float px) { playerPosX = px; }
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        uint64_t now = engine::GetSystemTime();
        float dist = std::abs(playerPosX - posX);
        
        // Face player
        facingRight = (playerPosX > posX);
        
        // Throw ball
        if (dist < throwRange && now - lastThrow > throwCooldown) {
            throwing = true;
            throwAnimTime = now;
            lastThrow = now;
            
            // Launch ball
            ball.active = true;
            ball.x = posX + (facingRight ? width : 0);
            ball.y = posY;
            ball.velX = facingRight ? 3.0f : -3.0f;
            ball.velY = -4.0f;
        }
        
        if (throwing && now - throwAnimTime > 400) {
            throwing = false;
        }
        
        // Update ball
        if (ball.active) {
            ball.velY += 0.2f;
            ball.x += ball.velX;
            ball.y += ball.velY;
            
            // Bounce on ground
            if (ball.y > posY + height && ball.velY > 0) {
                ball.velY = -ball.velY * 0.7f;
                if (std::abs(ball.velY) < 1.0f) ball.active = false;
            }
            
            // Despawn if too far
            if (std::abs(ball.x - posX) > 300) ball.active = false;
        }
        
        if (now - lastFrameTime > 150) {
            currentFrame = (currentFrame + 1) % 2;
            lastFrameTime = now;
        }
    }
    
    bool DamagesPlayer(const engine::Rect& playerBox, bool inBall) override {
        if (state != State::Active) return false;
        if (inBall) return false;
        
        // Body collision
        if (CollidesWith(playerBox)) return true;
        
        // Ball collision
        if (ball.active) {
            engine::Rect ballBox = {static_cast<int>(ball.x) - 6, static_cast<int>(ball.y) - 6, 12, 12};
            if (playerBox.x < ballBox.x + ballBox.w && playerBox.x + playerBox.w > ballBox.x &&
                playerBox.y < ballBox.y + ballBox.h && playerBox.y + playerBox.h > ballBox.y) {
                ball.active = false;
                return true;
            }
        }
        return false;
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(200, 150, 150);
        
        // Body
        gfx.DrawRect({screenX + 4, screenY + 8, width - 8, height - 12}, bodyColor, true);
        // Snout
        int snoutX = facingRight ? screenX + width - 8 : screenX;
        gfx.DrawRect({snoutX, screenY + height/2 - 4, 12, 10}, bodyColor, true);
        // Nostrils
        gfx.DrawRect({snoutX + 2, screenY + height/2, 2, 2}, engine::MakeColor(150, 100, 100), true);
        gfx.DrawRect({snoutX + 6, screenY + height/2, 2, 2}, engine::MakeColor(150, 100, 100), true);
        // Eyes
        int eyeX = facingRight ? screenX + width - 16 : screenX + 8;
        gfx.DrawRect({eyeX, screenY + 10, 4, 4}, engine::MakeColor(0, 0, 0), true);
        // Ears
        gfx.DrawRect({screenX + 8, screenY, 6, 8}, bodyColor, true);
        gfx.DrawRect({screenX + width - 14, screenY, 6, 8}, bodyColor, true);
        
        // Thrown ball
        if (ball.active) {
            int bx = static_cast<int>(ball.x) - viewWindow.x;
            int by = static_cast<int>(ball.y) - viewWindow.y;
            gfx.DrawRect({bx - 6, by - 6, 12, 12}, engine::MakeColor(50, 50, 50), true);
            // Fuse spark
            if (currentFrame % 2) {
                gfx.DrawRect({bx - 1, by - 8, 2, 4}, engine::MakeColor(255, 200, 50), true);
            }
        }
    }
};

// ============================================================
// ORBINAUT - Enemy with spiked balls orbiting around it
// ============================================================
class Orbinaut : public Enemy {
private:
    static constexpr int NUM_ORBS = 4;
    float orbAngle = 0;
    float orbRadius = 24.0f;
    float orbitSpeed = 0.05f;
    float moveSpeed = 1.0f;
    bool orbsPresent[NUM_ORBS] = {true, true, true, true};
    
public:
    Orbinaut(float x, float y) : Enemy(Type::Orbinaut, x, y, 32, 32) {}
    
    void Update() override {
        if (state != State::Active) {
            if (state == State::Dying) {
                velY += 0.3f;
                posY += velY;
                if (engine::GetSystemTime() - deathTime > 500) state = State::Dead;
            }
            return;
        }
        
        // Move slowly
        posX += facingRight ? moveSpeed : -moveSpeed;
        
        // Rotate orbs
        orbAngle += orbitSpeed;
        if (orbAngle > 6.28318f) orbAngle -= 6.28318f;
        
        // Turn at edges
        if (posX < 0 || posX > 10000) facingRight = !facingRight;
    }
    
    bool DamagesPlayer(const engine::Rect& playerBox, bool inBall) override {
        if (state != State::Active) return false;
        if (inBall) return false;
        
        // Check orb collisions
        float cx = posX + width/2;
        float cy = posY + height/2;
        
        for (int i = 0; i < NUM_ORBS; ++i) {
            if (!orbsPresent[i]) continue;
            
            float angle = orbAngle + (6.28318f * i / NUM_ORBS);
            float ox = cx + std::cos(angle) * orbRadius;
            float oy = cy + std::sin(angle) * orbRadius;
            
            engine::Rect orbBox = {static_cast<int>(ox) - 6, static_cast<int>(oy) - 6, 12, 12};
            if (playerBox.x < orbBox.x + orbBox.w && playerBox.x + playerBox.w > orbBox.x &&
                playerBox.y < orbBox.y + orbBox.h && playerBox.y + playerBox.h > orbBox.y) {
                return true;
            }
        }
        
        // Center body is also dangerous
        return CollidesWith(playerBox);
    }
    
    void Render(const engine::Rect& viewWindow) override {
        auto& gfx = engine::GetGraphics();
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        engine::Color bodyColor = state == State::Dying ? 
            engine::MakeColor(255, 150, 150) : engine::MakeColor(50, 100, 50);
        
        // Center body
        gfx.DrawRect({screenX + 8, screenY + 8, 16, 16}, bodyColor, true);
        // Eyes
        gfx.DrawRect({screenX + 10, screenY + 12, 4, 4}, engine::MakeColor(255, 50, 50), true);
        gfx.DrawRect({screenX + 18, screenY + 12, 4, 4}, engine::MakeColor(255, 50, 50), true);
        
        // Orbiting spike balls
        float cx = screenX + width/2;
        float cy = screenY + height/2;
        
        for (int i = 0; i < NUM_ORBS; ++i) {
            if (!orbsPresent[i]) continue;
            
            float angle = orbAngle + (6.28318f * i / NUM_ORBS);
            int ox = static_cast<int>(cx + std::cos(angle) * orbRadius);
            int oy = static_cast<int>(cy + std::sin(angle) * orbRadius);
            
            // Spike ball
            gfx.DrawRect({ox - 5, oy - 5, 10, 10}, engine::MakeColor(200, 50, 50), true);
            // Spikes
            gfx.DrawRect({ox - 2, oy - 8, 4, 4}, engine::MakeColor(200, 50, 50), true);
            gfx.DrawRect({ox - 2, oy + 4, 4, 4}, engine::MakeColor(200, 50, 50), true);
            gfx.DrawRect({ox - 8, oy - 2, 4, 4}, engine::MakeColor(200, 50, 50), true);
            gfx.DrawRect({ox + 4, oy - 2, 4, 4}, engine::MakeColor(200, 50, 50), true);
        }
    }
};

// ============================================================
// ANIMAL - Small critters freed from destroyed enemies
// ============================================================
enum class AnimalType {
    Flicky,     // Blue bird
    Pocky,      // Rabbit  
    Cucky,      // Chicken
    Ricky       // Squirrel
};

class Animal {
public:
    float posX, posY;
    float velX, velY;
    AnimalType type;
    bool isActive = true;
    
    int width = 16;
    int height = 16;
    
    int animFrame = 0;
    uint64_t lastFrameTime = 0;
    uint64_t spawnTime = 0;
    
    static constexpr uint64_t LIFETIME = 5000;  // Disappear after 5 seconds
    static constexpr uint64_t FRAME_DELAY = 100;
    
    Animal(float x, float y, AnimalType t) 
        : posX(x), posY(y), type(t) {
        // Random initial velocity
        velX = (std::rand() % 5 - 2) * 0.5f;
        velY = -6.0f;  // Pop up
        spawnTime = engine::GetSystemTime();
        lastFrameTime = spawnTime;
    }
    
    void Update() {
        if (!isActive) return;
        
        uint64_t now = engine::GetSystemTime();
        
        // Animate
        if (now - lastFrameTime >= FRAME_DELAY) {
            animFrame = (animFrame + 1) % 2;
            lastFrameTime = now;
        }
        
        // Physics
        velY += 0.3f;  // Gravity
        posX += velX;
        posY += velY;
        
        // Bounce on ground (simplified)
        if (posY > 1600) {  // Approximate ground level
            posY = 1600;
            velY = -4.0f;  // Bounce
            velX *= 0.8f;  // Slow down
        }
        
        // Lifetime check
        if (now - spawnTime > LIFETIME) {
            isActive = false;
        }
    }
    
    void Render(const engine::Rect& viewWindow) {
        if (!isActive) return;
        
        auto& gfx = engine::GetGraphics();
        
        int screenX = static_cast<int>(posX) - viewWindow.x;
        int screenY = static_cast<int>(posY) - viewWindow.y;
        
        if (screenX + width < 0 || screenX > viewWindow.w ||
            screenY + height < 0 || screenY > viewWindow.h) {
            return;
        }
        
        engine::Color bodyColor = engine::MakeColor(100, 100, 100);  // Default gray
        switch (type) {
            case AnimalType::Flicky:
                bodyColor = engine::MakeColor(100, 150, 255);  // Blue
                break;
            case AnimalType::Pocky:
                bodyColor = engine::MakeColor(255, 200, 150);  // Tan
                break;
            case AnimalType::Cucky:
                bodyColor = engine::MakeColor(255, 255, 100);  // Yellow
                break;
            case AnimalType::Ricky:
                bodyColor = engine::MakeColor(200, 150, 100);  // Brown
                break;
            default:
                break;
        }
        
        // Simple body
        gfx.DrawRect({screenX + 2, screenY + 4, width - 4, height - 4}, bodyColor, true);
        
        // Head
        gfx.DrawRect({screenX + 4, screenY, width - 8, 8}, bodyColor, true);
        
        // Eye
        gfx.DrawRect({screenX + 6, screenY + 2, 4, 4}, engine::MakeColor(0, 0, 0), true);
        
        // Wings/ears (animate)
        int wingOffset = (animFrame == 0) ? -2 : 2;
        if (type == AnimalType::Flicky || type == AnimalType::Cucky) {
            // Wings
            gfx.DrawRect({screenX, screenY + 6 + wingOffset, 4, 6}, bodyColor, true);
            gfx.DrawRect({screenX + width - 4, screenY + 6 - wingOffset, 4, 6}, bodyColor, true);
        } else {
            // Ears
            gfx.DrawRect({screenX + 2, screenY - 4 + wingOffset, 3, 6}, bodyColor, true);
            gfx.DrawRect({screenX + width - 5, screenY - 4 - wingOffset, 3, 6}, bodyColor, true);
        }
        
        // Border
        gfx.DrawRect({screenX, screenY, width, height}, engine::MakeColor(0, 0, 0), false);
    }
};

class AnimalManager {
private:
    std::vector<Animal> animals;
    
public:
    void Spawn(float x, float y) {
        // Random animal type
        AnimalType type = static_cast<AnimalType>(std::rand() % 4);
        animals.emplace_back(x, y, type);
    }
    
    void Update() {
        for (auto& animal : animals) {
            animal.Update();
        }
        
        // Remove inactive animals
        animals.erase(
            std::remove_if(animals.begin(), animals.end(),
                [](const Animal& a) { return !a.isActive; }),
            animals.end()
        );
    }
    
    void Render(const engine::Rect& viewWindow) {
        for (auto& animal : animals) {
            animal.Render(viewWindow);
        }
    }
    
    void Clear() { animals.clear(); }
};

// Enemy manager
class EnemyManager {
private:
    std::vector<std::unique_ptr<Enemy>> enemies;
    engine::GridLayer* gridLayer = nullptr;
    std::function<void(int)> onEnemyKilled;
    std::function<void(float, float)> onAnimalFreed;
    
    // Store player position for enemies that need it
    float playerX = 0;
    float playerY = 0;

public:
    void SetGridLayer(engine::GridLayer* grid) { gridLayer = grid; }
    void SetOnEnemyKilled(std::function<void(int)> callback) { onEnemyKilled = callback; }
    void SetOnAnimalFreed(std::function<void(float, float)> callback) { onAnimalFreed = callback; }
    
    void AddMotobug(float x, float y) {
        auto enemy = std::make_unique<Motobug>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddCrabmeat(float x, float y) {
        auto enemy = std::make_unique<Crabmeat>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddBuzzBomber(float x, float y, float patrolRange = 200.0f) {
        auto enemy = std::make_unique<BuzzBomber>(x, y, patrolRange);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddMasher(float x, float y, uint64_t jumpCooldown = 2000) {
        auto enemy = std::make_unique<Masher>(x, y, jumpCooldown);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    // === NEW ENEMY TYPES ===
    
    void AddNewtronBlue(float x, float y) {
        auto enemy = std::make_unique<NewtronBlue>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddNewtronGreen(float x, float y) {
        auto enemy = std::make_unique<NewtronGreen>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddBomb(float x, float y) {
        auto enemy = std::make_unique<Bomb>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddCaterkiller(float x, float y) {
        auto enemy = std::make_unique<Caterkiller>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddBatbrain(float x, float y) {
        auto enemy = std::make_unique<Batbrain>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddBurrobot(float x, float y) {
        auto enemy = std::make_unique<Burrobot>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddRoller(float x, float y) {
        auto enemy = std::make_unique<Roller>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddJaws(float x, float y) {
        auto enemy = std::make_unique<Jaws>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddBallHog(float x, float y) {
        auto enemy = std::make_unique<BallHog>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void AddOrbinaut(float x, float y) {
        auto enemy = std::make_unique<Orbinaut>(x, y);
        enemy->SetGridLayer(gridLayer);
        enemy->SetOnDeath(onAnimalFreed);
        enemies.push_back(std::move(enemy));
    }
    
    void Update(float pX = 0, float pY = 0) {
        playerX = pX;
        playerY = pY;
        
        for (auto& enemy : enemies) {
            // Update player position for enemies that track it
            if (auto* crab = dynamic_cast<Crabmeat*>(enemy.get())) {
                crab->SetPlayerPosition(playerX);
            } else if (auto* buzz = dynamic_cast<BuzzBomber*>(enemy.get())) {
                buzz->SetPlayerPosition(playerX, playerY);
            } else if (auto* masher = dynamic_cast<Masher*>(enemy.get())) {
                masher->SetPlayerPosition(playerX);
            } else if (auto* newtronB = dynamic_cast<NewtronBlue*>(enemy.get())) {
                newtronB->SetPlayerPosition(playerX, playerY);
            } else if (auto* newtronG = dynamic_cast<NewtronGreen*>(enemy.get())) {
                newtronG->SetPlayerPosition(playerX);
            } else if (auto* bat = dynamic_cast<Batbrain*>(enemy.get())) {
                bat->SetPlayerPosition(playerX);
            } else if (auto* burro = dynamic_cast<Burrobot*>(enemy.get())) {
                burro->SetPlayerPosition(playerX);
            } else if (auto* roller = dynamic_cast<Roller*>(enemy.get())) {
                roller->SetPlayerPosition(playerX);
            } else if (auto* ballhog = dynamic_cast<BallHog*>(enemy.get())) {
                ballhog->SetPlayerPosition(playerX);
            }
            enemy->Update();
        }
        
        // Remove dead enemies
        enemies.erase(
            std::remove_if(enemies.begin(), enemies.end(),
                [](const std::unique_ptr<Enemy>& e) { return e->IsDead(); }),
            enemies.end()
        );
    }
    
    // Returns score if enemy killed, -1 if player damaged, 0 if no collision
    // playerInBallState: true if jumping/rolling (kills enemies)
    // playerCanKillOnContact: true if power-up invincibility (kills enemies on any contact)
    // playerDamageImmune: true if any invincibility (post-hit or power-up, skips damage)
    int CheckCollision(const engine::Rect& playerBox, bool playerInBallState, 
                       bool playerCanKillOnContact, bool playerDamageImmune) {
        for (auto& enemy : enemies) {
            if (!enemy->IsActive()) continue;
            
            // Player can kill enemy (ball state or power-up invincibility)
            if (playerInBallState || playerCanKillOnContact) {
                if (enemy->CollidesWith(playerBox)) {
                    int score = enemy->GetScore();
                    enemy->Kill();
                    if (onEnemyKilled) onEnemyKilled(score);
                    return score;
                }
            }
            // Player has damage immunity (post-hit invincibility) - no kill, no damage
            else if (playerDamageImmune) {
                // Just pass through, no interaction
                continue;
            }
            // Enemy damages player (normal collision, no protection)
            else if (enemy->DamagesPlayer(playerBox, playerInBallState)) {
                return -1;
            }
        }
        return 0;
    }
    
    void Render(const engine::Rect& viewWindow) {
        for (auto& enemy : enemies) {
            enemy->Render(viewWindow);
        }
    }
    
    void Clear() { enemies.clear(); }
    int GetCount() const { return static_cast<int>(enemies.size()); }
};

} // namespace app

#endif // ENEMY_HPP
