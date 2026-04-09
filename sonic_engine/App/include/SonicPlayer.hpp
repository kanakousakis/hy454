#ifndef SONIC_PLAYER_HPP
#define SONIC_PLAYER_HPP

#include "Engine.hpp"
#include "TunnelSystem.hpp"
#include <functional>
#include <string>

namespace app {

enum class SonicState {
    Idle,  //standing still
    Bored,  //waiting animation (foot tap, looking around)
    LookingUp,  //looking upward
    Crouching,  //ducking down
    Spindash,  //charging spindash (curled up, revving)
    Walking,  //slow movement
    Running,  //fast movement
    FullSpeed,  //terminal velocity - can go through loops
    Jumping,  //in air, ball form
    Rolling,  //ball form on ground (spin dash)
    Skidding,  //braking hard, sliding
    Balancing,  //on edge of platform
    Pushing,  //against wall
    Spring,  //bounced by spring (in air)
    Landing,  //brief landing from spring/jump (smooth transition)
    Hurt,  //taking damage, knocked back
    Dead,  //lost all lives - shows continue animation
    Invincible,  //temporary invulnerability after power-up
    PipeCling,  //holding onto pipe
    Tunnel,  //spinning through tunnel
    AirGasp,  //underwater air bubble
    Drowning  //ran out of air
};

//facing direction
enum class FacingDirection {
    Left,
    Right
};

class SonicPlayer {
public:
//physics constants (configurable)
    struct PhysicsConfig {
        float acceleration = 0.046875f;  //ground acceleration
        float deceleration = 0.5f;  //braking deceleration
        float friction = 0.046875f;  //ground friction
        float topSpeed = 6.0f;  //normal max speed
        float fullSpeedThreshold = 5.5f;  //speed to enter full speed state
        float jumpForce = 6.5f;  //initial jump velocity
        float gravity = 0.21875f;  //normal gravity
        float hurtGravity = 0.1875f;  //gravity while hurt
        float maxFallSpeed = 16.0f;  //terminal fall velocity
        float airAcceleration = 0.09375f;  //air control
    };

private:
//position (sub-pixel precision)
    float posX = 0;
    float posY = 0;
    
//velocity
    float velX = 0;
    float velY = 0;
    float groundSpeed = 0;  //speed along ground (for slopes)
    float groundAngle = 0;  //ground angle in degrees (0=flat, positive=uphill right, negative=downhill right)
    
//state
    SonicState state = SonicState::Idle;
    SonicState previousState = SonicState::Idle;
    FacingDirection facing = FacingDirection::Right;
    
//collision box
    int width = 20;  //normal width
    int height = 40;  //normal height
    int ballHeight = 20;  //ball form height
    
//status
    int rings = 0;
    int lives = 3;
    int score = 0;
    bool isOnGround = false;
    bool isInvincible = false;  //damage immunity (post-hit or power-up)
    bool hasPowerUpInvincibility = false;  //power-up invincibility (kills enemies)
    bool inPostDamageInvincibility = false;  //post-damage invincibility (can't kill enemies)
    uint64_t invincibleEndTime = 0;
    uint64_t hurtEndTime = 0;
    bool hasShield = false;
    uint64_t shieldEndTime = 0;
    bool hasSpeedBoost = false;
    uint64_t speedBoostEndTime = 0;
    
//special tile states
    bool inTunnel = false;
    bool inLoop = false;
    bool onSlopeTile = false;  //true ONLY when standing on GRID_SLOPE tiles
    
//loop physics
    bool loopActive = false;  //currently traversing a loop
    float loopAngle = 0.0f;  //current angle in loop (0-360)
    float loopCenterX = 0.0f;  //center of current loop
    float loopCenterY = 0.0f;
    float loopRadius = 48.0f;  //loop radius in pixels
    static constexpr float LOOP_MIN_SPEED = 6.0f;  //minimum speed to complete loop
    
//tunnel physics - NEW SYSTEM
    TunnelSystem tunnelSystem;
    bool inTunnelMode = false;
    
//god mode (cheat)
    bool godMode = false;

//whirl circle / spin loop
    bool physicsEnabled = true;  //can be disabled during whirl circle

//animation
    engine::Sprite* sprite = nullptr;
    engine::FrameRangeAnimator* animator = nullptr;
    std::string currentAnimId;
    
//idle timer for bored animation (Dami-Karv: 5 seconds)
    uint64_t idleStartTime = 0;
    static constexpr float BORED_DELAY = 3.0f;  //seconds before bored animation
    
//skidding (Dami-Karv: threshold = 4.0f)
    static constexpr float SKID_THRESHOLD = 4.0f;
    bool isSkidding = false;
    
//landing transition (for smooth spring->run)
    uint64_t landingEndTime = 0;
    static constexpr uint64_t LANDING_DURATION = 80;  //80ms landing animation (can be interrupted by input)
    
//sub-pixel position accumulator for smooth movement
    float subPixelX = 0.0f;
    float subPixelY = 0.0f;
    
//spindash
    float spindashCharge = 0.0f;
    static constexpr float SPINDASH_CHARGE_RATE = 2.0f;
    static constexpr float SPINDASH_MAX_CHARGE = 12.0f;
    static constexpr float SPINDASH_BASE_SPEED = 8.0f;
    
//physics config
    PhysicsConfig physics;
    
//grid layer reference for collision
    engine::GridLayer* gridLayer = nullptr;
    
//input state
    bool inputLeft = false;
    bool inputRight = false;
    bool inputJump = false;
    bool inputJumpPressed = false;  //just pressed this frame
    bool inputDown = false;
    bool inputUp = false;
    
//callbacks
    std::function<void(int oldRings, int newRings)> onRingsChanged;
    std::function<void(int oldLives, int newLives)> onLivesChanged;
    std::function<void(int points)> onScoreChanged;
    std::function<void(SonicState newState)> onStateChanged;
    std::function<void()> onDeath;
    std::function<void(float x, float y, int count)> onScatterRings;
    
//internal methods
    void UpdateState();
    void UpdatePhysics();
    void UpdateAnimation();
    void ApplyGravity();
    void ApplyGroundMovement();
    void ApplyAirMovement();
    void CheckGroundCollision();
    void CheckWallCollision();
    void ResolveWallOverlap();
    void DetectGroundSlope();
    void SnapToGround();
    void CheckSpecialTiles();
    void EnterTunnelMode(int tunnelIndex);
    void ExitTunnelMode();
    void SetState(SonicState newState);
    void StartAnimation(const std::string& animId);
    int GetCurrentHeight() const;
    
public:
    SonicPlayer();
    ~SonicPlayer();
    
//setup
    void Create(float startX, float startY, engine::GridLayer* grid);
    void SetSprite(engine::Sprite* spr) { sprite = spr; }
    void SetAnimator(engine::FrameRangeAnimator* anim) { animator = anim; }
    void SetPhysicsConfig(const PhysicsConfig& config) { physics = config; }
    
//game loop
    void HandleInput(bool left, bool right, bool jump, bool jumpPressed, bool down = false, bool up = false);
    void Update();  //called each frame
    void Render(engine::Rect& viewWindow);
    
//position
    float GetX() const { return posX; }
    float GetY() const { return posY; }
    int GetIntX() const { return static_cast<int>(posX); }
    int GetIntY() const { return static_cast<int>(posY); }
    void SetPosition(float x, float y) { posX = x; posY = y; }
    
//velocity
    float GetVelX() const { return velX; }
    float GetVelY() const { return velY; }
    float GetSpeed() const { return groundSpeed; }
    float GetSpindashCharge() const { return spindashCharge; }
    float GetGroundAngle() const { return groundAngle; }
    void SetGroundAngle(float angle) { groundAngle = angle; }
    
//collision box
    engine::Rect GetBoundingBox() const;
    int GetWidth() const { return width; }
    int GetHeight() const { return GetCurrentHeight(); }
    
//state
    SonicState GetState() const { return state; }
    bool IsDead() const { return state == SonicState::Dead; }
    FacingDirection GetFacing() const { return facing; }
    bool IsOnGround() const { return isOnGround; }
    bool IsInTunnel() const { return inTunnel; }
    bool IsInLoop() const { return inLoop; }
    bool IsLoopActive() const { return loopActive; }
    float GetLoopAngle() const { return loopAngle; }
    bool IsTunnelActive() const { return inTunnelMode; }
    bool IsOnSlopeTile() const { return onSlopeTile; }
    bool IsGodMode() const { return godMode; }
    void SetGodMode(bool enabled) { godMode = enabled; }
    uint64_t GetIdleStartTime() const { return idleStartTime; }
    void ResetIdleTimer();
    bool IsInvincible() const { return isInvincible; }
    bool HasPowerUpInvincibility() const { return hasPowerUpInvincibility; }
    bool IsInPostDamageInvincibility() const { return inPostDamageInvincibility; }
    bool HasShield() const { return hasShield; }
    bool HasSpeedBoost() const { return hasSpeedBoost; }
    bool IsBallState() const;
    
//status
    int GetRings() const { return rings; }
    void AddRings(int count);
    void LoseRings();  //scatter rings on damage
    int GetLives() const { return lives; }
    void AddLife();
    void LoseLife();
    int GetScore() const { return score; }
    void AddScore(int points);
    
//damage and power-ups
    void TakeDamage();
    void Kill();  //instant death (pit, crushed)
    void Die();  //process death
    void Reset();  //reset to initial state (for level restart)
    void CollectInvincibility(uint64_t duration = 20000);  //20 seconds
    void CollectShield(uint64_t duration = 30000);  //30 seconds
    void CollectSpeedBoost(uint64_t duration = 10000);  //10 seconds
    
//alternate names for power-ups
    void GiveInvincibility(uint64_t duration) { CollectInvincibility(duration); }
    void GiveShield(uint64_t duration) { CollectShield(duration); }
    void GiveSpeedBoost(uint64_t duration) { CollectSpeedBoost(duration); }

//physics control (for whirl circles)
    void DisablePhysics() { physicsEnabled = false; }
    void EnablePhysics() { physicsEnabled = true; }
    bool IsPhysicsEnabled() const { return physicsEnabled; }

//spring bounce - uses Spring state for "bounced up" animation
    void ApplyBounce(float bounceVelX, float bounceVelY) {
        velX = bounceVelX;
        velY = bounceVelY;
        groundSpeed = bounceVelX;  //sync ground speed
        isOnGround = false;
//use Spring state for the "bounced up" animation!
        SetState(SonicState::Spring);
    }
    
//callbacks
    void SetOnRingsChanged(std::function<void(int, int)> cb) { onRingsChanged = cb; }
    void SetOnLivesChanged(std::function<void(int, int)> cb) { onLivesChanged = cb; }
    void SetOnScoreChanged(std::function<void(int)> cb) { onScoreChanged = cb; }
    void SetOnStateChanged(std::function<void(SonicState)> cb) { onStateChanged = cb; }
    void SetOnDeath(std::function<void()> cb) { onDeath = cb; }
    void SetOnScatterRings(std::function<void(float, float, int)> cb) { onScatterRings = cb; }
    
//respawn
    void Respawn(float x, float y);
};

}  //namespace app

#endif  //SONIC_PLAYER_HPP
