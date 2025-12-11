#ifndef SONIC_PLAYER_HPP
#define SONIC_PLAYER_HPP

#include "Engine.hpp"
#include <functional>
#include <string>

namespace app {

// Sonic states as per project specification and sprite sheet
enum class SonicState {
    Idle,           // Standing still
    Bored,          // Waiting animation (foot tap, looking around)
    LookingUp,      // Looking upward
    Crouching,      // Ducking down
    Spindash,       // Charging spindash (curled up, revving)
    Walking,        // Slow movement
    Running,        // Fast movement  
    FullSpeed,      // Terminal velocity - can go through loops
    Jumping,        // In air, ball form
    Rolling,        // Ball form on ground (spin dash)
    Skidding,       // Braking hard, sliding
    Balancing,      // On edge of platform
    Pushing,        // Against wall
    Spring,         // Bounced by spring (in air)
    Landing,        // Brief landing from spring/jump (smooth transition)
    Hurt,           // Taking damage, knocked back
    Dead,           // Lost all lives - shows continue animation
    Invincible,     // Temporary invulnerability after power-up
    PipeCling,      // Holding onto pipe
    Tunnel,         // Spinning through tunnel
    AirGasp,        // Underwater air bubble
    Drowning        // Ran out of air
};

// Facing direction
enum class FacingDirection {
    Left,
    Right
};

class SonicPlayer {
public:
    // Physics constants (configurable)
    struct PhysicsConfig {
        float acceleration = 0.046875f;     // Ground acceleration
        float deceleration = 0.5f;          // Braking deceleration
        float friction = 0.046875f;         // Ground friction
        float topSpeed = 6.0f;              // Normal max speed
        float fullSpeedThreshold = 5.5f;    // Speed to enter full speed state
        float jumpForce = 6.5f;             // Initial jump velocity
        float gravity = 0.21875f;           // Normal gravity
        float hurtGravity = 0.1875f;        // Gravity while hurt
        float maxFallSpeed = 16.0f;         // Terminal fall velocity
        float airAcceleration = 0.09375f;   // Air control
    };

private:
    // Position (sub-pixel precision)
    float posX = 0;
    float posY = 0;
    
    // Velocity
    float velX = 0;
    float velY = 0;
    float groundSpeed = 0;  // Speed along ground (for slopes)
    
    // State
    SonicState state = SonicState::Idle;
    SonicState previousState = SonicState::Idle;
    FacingDirection facing = FacingDirection::Right;
    
    // Collision box
    int width = 20;         // Normal width
    int height = 40;        // Normal height
    int ballHeight = 20;    // PDF: "Sonic's hitbox is halved" when in ball form (40/2 = 20)
    
    // Status
    int rings = 0;
    int lives = 3;
    int score = 0;
    bool isOnGround = false;
    bool isInvincible = false;          // Damage immunity (post-hit or power-up)
    bool hasPowerUpInvincibility = false; // Power-up invincibility (kills enemies)
    bool inPostDamageInvincibility = false; // Post-damage invincibility (can't kill enemies)
    uint64_t invincibleEndTime = 0;
    uint64_t hurtEndTime = 0;
    bool hasShield = false;
    uint64_t shieldEndTime = 0;
    bool hasSpeedBoost = false;
    uint64_t speedBoostEndTime = 0;
    
    // Animation
    engine::Sprite* sprite = nullptr;
    engine::FrameRangeAnimator* animator = nullptr;
    std::string currentAnimId;
    
    // Idle timer for bored animation (Dami-Karv: 5 seconds)
    uint64_t idleStartTime = 0;
    static constexpr float BORED_DELAY = 5.0f;  // Seconds before bored animation
    
    // Skidding (Dami-Karv: threshold = 4.0f)
    static constexpr float SKID_THRESHOLD = 4.0f;
    bool isSkidding = false;
    
    // Landing transition (for smooth spring->run)
    uint64_t landingEndTime = 0;
    static constexpr uint64_t LANDING_DURATION = 80;  // 80ms landing animation (can be interrupted by input)
    
    // Sub-pixel position accumulator for smooth movement
    float subPixelX = 0.0f;
    float subPixelY = 0.0f;
    
    // Spindash
    float spindashCharge = 0.0f;
    static constexpr float SPINDASH_CHARGE_RATE = 2.0f;
    static constexpr float SPINDASH_MAX_CHARGE = 12.0f;
    static constexpr float SPINDASH_BASE_SPEED = 8.0f;
    
    // Physics config
    PhysicsConfig physics;
    
    // Grid layer reference for collision
    engine::GridLayer* gridLayer = nullptr;
    
    // Input state
    bool inputLeft = false;
    bool inputRight = false;
    bool inputJump = false;
    bool inputJumpPressed = false;  // Just pressed this frame
    bool inputDown = false;
    bool inputUp = false;
    
    // Callbacks
    std::function<void(int oldRings, int newRings)> onRingsChanged;
    std::function<void(int oldLives, int newLives)> onLivesChanged;
    std::function<void(int points)> onScoreChanged;
    std::function<void(SonicState newState)> onStateChanged;
    std::function<void()> onDeath;
    std::function<void(float x, float y, int count)> onScatterRings;
    
    // Internal methods
    void UpdateState();
    void UpdatePhysics();
    void UpdateAnimation();
    void ApplyGravity();
    void ApplyGroundMovement();
    void ApplyAirMovement();
    void CheckGroundCollision();
    void CheckWallCollision();
    void ResolveWallOverlap();
    void SetState(SonicState newState);
    void StartAnimation(const std::string& animId);
    int GetCurrentHeight() const;
    
public:
    SonicPlayer();
    ~SonicPlayer();
    
    // Setup
    void Create(float startX, float startY, engine::GridLayer* grid);
    void SetSprite(engine::Sprite* spr) { sprite = spr; }
    void SetAnimator(engine::FrameRangeAnimator* anim) { animator = anim; }
    void SetPhysicsConfig(const PhysicsConfig& config) { physics = config; }
    
    // Game loop
    void HandleInput(bool left, bool right, bool jump, bool jumpPressed, bool down = false, bool up = false);
    void Update();  // Called each frame
    void Render(engine::Rect& viewWindow);
    
    // Position
    float GetX() const { return posX; }
    float GetY() const { return posY; }
    int GetIntX() const { return static_cast<int>(posX); }
    int GetIntY() const { return static_cast<int>(posY); }
    void SetPosition(float x, float y) { posX = x; posY = y; }
    
    // Velocity
    float GetVelX() const { return velX; }
    float GetVelY() const { return velY; }
    float GetSpeed() const { return groundSpeed; }
    float GetSpindashCharge() const { return spindashCharge; }
    
    // Collision box
    engine::Rect GetBoundingBox() const;
    int GetWidth() const { return width; }
    int GetHeight() const { return GetCurrentHeight(); }
    
    // State
    SonicState GetState() const { return state; }
    FacingDirection GetFacing() const { return facing; }
    bool IsOnGround() const { return isOnGround; }
    uint64_t GetIdleStartTime() const { return idleStartTime; }
    void ResetIdleTimer();
    bool IsInvincible() const { return isInvincible; }
    bool HasPowerUpInvincibility() const { return hasPowerUpInvincibility; }
    bool IsInPostDamageInvincibility() const { return inPostDamageInvincibility; }
    bool HasShield() const { return hasShield; }
    bool IsBallState() const;
    
    // Status
    int GetRings() const { return rings; }
    void AddRings(int count);
    void LoseRings();  // Scatter rings on damage
    int GetLives() const { return lives; }
    void AddLife();
    void LoseLife();
    int GetScore() const { return score; }
    void AddScore(int points);
    
    // Damage and power-ups
    void TakeDamage();
    void Kill();  // Instant death (pit, crushed)
    void Die();   // Process death
    void Reset(); // Reset to initial state (for level restart)
    void CollectInvincibility(uint64_t duration = 20000);  // 20 seconds
    void CollectShield(uint64_t duration = 30000);         // 30 seconds
    void CollectSpeedBoost(uint64_t duration = 10000);     // 10 seconds
    
    // Alternate names for power-ups
    void GiveInvincibility(uint64_t duration) { CollectInvincibility(duration); }
    void GiveShield(uint64_t duration) { CollectShield(duration); }
    void GiveSpeedBoost(uint64_t duration) { CollectSpeedBoost(duration); }
    
    // Spring bounce - uses Spring state for "bounced up" animation
    void ApplyBounce(float bounceVelX, float bounceVelY) {
        velX = bounceVelX;
        velY = bounceVelY;
        groundSpeed = bounceVelX;  // Sync ground speed
        isOnGround = false;
        // Use Spring state for the "bounced up" animation!
        SetState(SonicState::Spring);
    }
    
    // Callbacks
    void SetOnRingsChanged(std::function<void(int, int)> cb) { onRingsChanged = cb; }
    void SetOnLivesChanged(std::function<void(int, int)> cb) { onLivesChanged = cb; }
    void SetOnScoreChanged(std::function<void(int)> cb) { onScoreChanged = cb; }
    void SetOnStateChanged(std::function<void(SonicState)> cb) { onStateChanged = cb; }
    void SetOnDeath(std::function<void()> cb) { onDeath = cb; }
    void SetOnScatterRings(std::function<void(float, float, int)> cb) { onScatterRings = cb; }
    
    // Respawn
    void Respawn(float x, float y);
};

} // namespace app

#endif // SONIC_PLAYER_HPP
