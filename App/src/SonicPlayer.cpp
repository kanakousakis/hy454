#include "SonicPlayer.hpp"
#include "Input.hpp"
#include "SystemClock.hpp"
#include "TileConstants.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace engine;

namespace app {

SonicPlayer::SonicPlayer() {}

SonicPlayer::~SonicPlayer() {
    // Note: sprite and animator are managed externally
}

void SonicPlayer::Create(float startX, float startY, engine::GridLayer* grid) {
    posX = startX;
    posY = startY;
    gridLayer = grid;
    state = SonicState::Idle;
    velX = velY = groundSpeed = 0;
    rings = 0;
    lives = 3;
    score = 0;
    isOnGround = false;
    isInvincible = false;
    hasShield = false;
    hasSpeedBoost = false;
}

int SonicPlayer::GetCurrentHeight() const {
    return IsBallState() ? ballHeight : height;
}

bool SonicPlayer::IsBallState() const {
    // Ball states: jumping, rolling, spindash
    // Also: Spring state when FALLING (velY >= 0) because Sonic curls into ball
    if (state == SonicState::Jumping || state == SonicState::Rolling || state == SonicState::Spindash) {
        return true;
    }
    // Spring state: only ball when falling down (going up = arms spread, not ball)
    if (state == SonicState::Spring && velY >= 0) {
        return true;
    }
    return false;
}

engine::Rect SonicPlayer::GetBoundingBox() const {
    int h = GetCurrentHeight();
    // Center the collision box horizontally, align to bottom
    return {
        static_cast<int>(posX) - width / 2,
        static_cast<int>(posY) - h,
        width,
        h
    };
}

void SonicPlayer::HandleInput(bool left, bool right, bool jump, bool jumpPressed, bool down, bool up) {
    inputLeft = left;
    inputRight = right;
    inputJump = jump;
    inputJumpPressed = jumpPressed;
    inputDown = down;
    inputUp = up;
    
    // Reset idle timer when any movement key is pressed (like Dami-Karv)
    if (left || right || jump || down || up) {
        ResetIdleTimer();
    }
}

void SonicPlayer::Update() {
    // Update power-up timers
    uint64_t now = engine::GetSystemTime();
    
    if (isInvincible && now >= invincibleEndTime) {
        isInvincible = false;
        hasPowerUpInvincibility = false;
        inPostDamageInvincibility = false;  // Clear post-damage flag too
        std::cout << "Invincibility EXPIRED at " << now << std::endl;
    }
    if (hasShield && now >= shieldEndTime) {
        hasShield = false;
    }
    if (hasSpeedBoost && now >= speedBoostEndTime) {
        hasSpeedBoost = false;
    }
    
    // PDF: "Player cannot leave the hurt state until they land on the ground"
    // When landing from hurt state, X Speed is set to 0
    // Only transition when actually falling and hitting ground (not immediately after bounce)
    if (state == SonicState::Hurt) {
        // Must be falling (velY > 0) and hit ground to recover
        // This prevents immediate recovery when hurt bounce hasn't lifted us yet
        if (isOnGround && velY >= 0) {
            // PDF: "When they do land, their X Speed is set to 0, so they stop completely"
            SetState(SonicState::Idle);
            velX = 0;
            groundSpeed = 0;
            // Note: invincibility continues after landing (handled by invincibleEndTime)
            std::cout << "Recovered from hurt (landed)" << std::endl;
        }
    }
    
    // Skip updates if dead
    if (state == SonicState::Dead) {
        ApplyGravity();
        posY += velY;
        return;
    }
    
    UpdatePhysics();
    UpdateState();
    UpdateAnimation();
}

void SonicPlayer::UpdatePhysics() {
    if (state == SonicState::Hurt) {
        // Hurt physics - reduced control
        velY += physics.hurtGravity;
        if (velY > physics.maxFallSpeed) velY = physics.maxFallSpeed;
        
        // Filter movement through collision grid with sub-pixel
        if (gridLayer) {
            engine::Rect box = GetBoundingBox();
            
            subPixelX += velX;
            subPixelY += velY;
            int dx = static_cast<int>(subPixelX);
            int dy = static_cast<int>(subPixelY);
            subPixelX -= dx;
            subPixelY -= dy;
            
            gridLayer->FilterGridMotion(box, &dx, &dy);
            posX += dx;
            posY += dy;
            if (dx == 0 && velX != 0) { velX = 0; subPixelX = 0; }
            if (dy == 0 && velY > 0) { velY = 0; subPixelY = 0; isOnGround = true; }
        } else {
            posX += velX;
            posY += velY;
        }
        return;
    }
    
    if (isOnGround) {
        ApplyGroundMovement();
    } else {
        ApplyAirMovement();
    }
    
    // Filter movement through collision BEFORE applying position
    if (gridLayer) {
        engine::Rect box = GetBoundingBox();
        
        // Accumulate sub-pixel movement
        subPixelX += velX;
        subPixelY += velY;
        
        // Extract whole pixels to move
        int dx = static_cast<int>(subPixelX);
        int dy = static_cast<int>(subPixelY);
        
        // Keep the fractional part for next frame
        subPixelX -= dx;
        subPixelY -= dy;
        
        // Filter horizontal movement
        // Use shorter box to avoid ground tiles being detected as walls
        // Ground tiles (GRID_SOLID) block all directions including horizontal,
        // so we need to exclude the feet area from horizontal collision checks
        if (dx != 0) {
            engine::Rect horizBox = box;
            horizBox.h = std::max(8, horizBox.h - 12);  // Remove bottom 12 pixels (feet area)
            
            int testDx = dx;
            int testDy = 0;
            gridLayer->FilterGridMotion(horizBox, &testDx, &testDy);
            if (testDx != dx) {
                // Hit a wall - stop and clear sub-pixel accumulator
                velX = 0;
                groundSpeed = 0;
                subPixelX = 0;
                dx = testDx;
            }
        }
        
        // Apply horizontal movement
        posX += dx;
        box.x = static_cast<int>(posX) - width / 2;
        
        // Filter vertical movement
        if (dy != 0) {
            int testDx = 0;
            int testDy = dy;
            gridLayer->FilterGridMotion(box, &testDx, &testDy);
            if (testDy != dy) {
                if (dy > 0) {
                    // Hit ground
                    isOnGround = true;
                }
                velY = 0;
                subPixelY = 0;  // Clear vertical sub-pixel on landing
                dy = testDy;
            }
        }
        
        // Apply vertical movement
        posY += dy;
    } else {
        posX += velX;
        posY += velY;
    }
    
    // Additional ground check
    CheckGroundCollision();
    
    // Push out of walls (handles cases where Sonic clips through)
    ResolveWallOverlap();
}

void SonicPlayer::ResolveWallOverlap() {
    if (!gridLayer) return;
    if (state == SonicState::Dead) return;  // Let dead Sonic fall through everything
    
    engine::Rect box = GetBoundingBox();
    
    int topRow = box.y / GRID_ELEMENT_HEIGHT;
    int bottomRow = (box.y + box.h - 1) / GRID_ELEMENT_HEIGHT;
    
    // Clamp rows to valid range
    if (topRow < 0) topRow = 0;
    if (bottomRow < 0) return;  // Above map, no wall collision needed
    
    // Check if we're inside any solid tiles and push out
    int leftEdgeCol = box.x / GRID_ELEMENT_WIDTH;
    int rightEdgeCol = (box.x + box.w - 1) / GRID_ELEMENT_WIDTH;
    
    // === CEILING CHECK (jumping into solid from below) ===
    // Check if top of Sonic is inside a solid tile
    bool topBlocked = false;
    for (int col = leftEdgeCol; col <= rightEdgeCol; ++col) {
        GridIndex tile = gridLayer->GetTile(col, topRow);
        if (tile & GRID_BOTTOM_SOLID) {  // Solid from below
            topBlocked = true;
            break;
        }
    }
    
    if (topBlocked && velY < 0) {
        // Push down - find bottom edge of ceiling
        int ceilingBottom = (topRow + 1) * GRID_ELEMENT_HEIGHT;
        float newPosY = ceilingBottom + GetCurrentHeight() + 1;
        if (posY < newPosY) {
            posY = newPosY;
            velY = 0;  // Stop upward movement
        }
    }
    
    // Refresh box after potential Y adjustment
    box = GetBoundingBox();
    topRow = box.y / GRID_ELEMENT_HEIGHT;
    bottomRow = (box.y + box.h - 1) / GRID_ELEMENT_HEIGHT;
    if (topRow < 0) topRow = 0;
    leftEdgeCol = box.x / GRID_ELEMENT_WIDTH;
    rightEdgeCol = (box.x + box.w - 1) / GRID_ELEMENT_WIDTH;
    
    // === HORIZONTAL WALL CHECK ===
    // Use shorter box to avoid ground tiles being detected as walls
    // Ground tiles (GRID_SOLID) block all directions, so we exclude feet area
    engine::Rect horizBox = box;
    horizBox.h = std::max(8, horizBox.h - 12);  // Remove bottom 12 pixels
    int horizTopRow = horizBox.y / GRID_ELEMENT_HEIGHT;
    int horizBottomRow = (horizBox.y + horizBox.h - 1) / GRID_ELEMENT_HEIGHT;
    if (horizTopRow < 0) horizTopRow = 0;
    
    // Check if right side is in a wall
    bool rightBlocked = false;
    for (int row = horizTopRow; row <= horizBottomRow; ++row) {
        GridIndex tile = gridLayer->GetTile(rightEdgeCol, row);
        if (tile & GRID_LEFT_SOLID) {  // Solid from left side
            rightBlocked = true;
            break;
        }
    }
    
    // Check if left side is in a wall
    bool leftBlocked = false;
    for (int row = horizTopRow; row <= horizBottomRow; ++row) {
        GridIndex tile = gridLayer->GetTile(leftEdgeCol, row);
        if (tile & GRID_RIGHT_SOLID) {  // Solid from right side
            leftBlocked = true;
            break;
        }
    }
    
    // Push out of walls
    if (rightBlocked && !leftBlocked) {
        // Push left - find the left edge of the wall
        int wallLeftEdge = rightEdgeCol * GRID_ELEMENT_WIDTH;
        float newPosX = wallLeftEdge - (box.w / 2.0f) - 1;
        if (posX > newPosX) {
            posX = newPosX;
            velX = 0;
            groundSpeed = 0;
        }
    } else if (leftBlocked && !rightBlocked) {
        // Push right - find the right edge of the wall
        int wallRightEdge = (leftEdgeCol + 1) * GRID_ELEMENT_WIDTH;
        float newPosX = wallRightEdge + (box.w / 2.0f) + 1;
        if (posX < newPosX) {
            posX = newPosX;
            velX = 0;
            groundSpeed = 0;
        }
    } else if (rightBlocked && leftBlocked) {
        // Both sides blocked - stop movement
        velX = 0;
        groundSpeed = 0;
    }
}

void SonicPlayer::ApplyGravity() {
    float grav = (state == SonicState::Hurt) ? physics.hurtGravity : physics.gravity;
    velY += grav;
    if (velY > physics.maxFallSpeed) {
        velY = physics.maxFallSpeed;
    }
}

void SonicPlayer::ApplyGroundMovement() {
    float topSpd = hasSpeedBoost ? physics.topSpeed * 1.5f : physics.topSpeed;
    
    // Acceleration and deceleration
    if (inputLeft && !inputRight) {
        if (groundSpeed > 0) {
            // Braking
            groundSpeed -= physics.deceleration;
            if (groundSpeed < 0) groundSpeed = -physics.deceleration;
        } else if (groundSpeed > -topSpd) {
            // Accelerating left
            groundSpeed -= physics.acceleration;
            if (groundSpeed < -topSpd) groundSpeed = -topSpd;
        }
        facing = FacingDirection::Left;
    } else if (inputRight && !inputLeft) {
        if (groundSpeed < 0) {
            // Braking
            groundSpeed += physics.deceleration;
            if (groundSpeed > 0) groundSpeed = physics.deceleration;
        } else if (groundSpeed < topSpd) {
            // Accelerating right
            groundSpeed += physics.acceleration;
            if (groundSpeed > topSpd) groundSpeed = topSpd;
        }
        facing = FacingDirection::Right;
    } else {
        // Apply friction when no input
        if (groundSpeed > 0) {
            groundSpeed -= physics.friction;
            if (groundSpeed < 0) groundSpeed = 0;
        } else if (groundSpeed < 0) {
            groundSpeed += physics.friction;
            if (groundSpeed > 0) groundSpeed = 0;
        }
    }
    
    // Ground speed to velocity (flat ground)
    velX = groundSpeed;
    velY = 0;
    
    // Jump
    if (inputJumpPressed && state != SonicState::Hurt) {
        velY = -physics.jumpForce;
        isOnGround = false;
        SetState(SonicState::Jumping);
    }
}

void SonicPlayer::ApplyAirMovement() {
    // Air drag and control
    if (inputLeft && !inputRight) {
        velX -= physics.airAcceleration;
        facing = FacingDirection::Left;
    } else if (inputRight && !inputLeft) {
        velX += physics.airAcceleration;
        facing = FacingDirection::Right;
    }
    
    // Clamp air speed
    float topSpd = hasSpeedBoost ? physics.topSpeed * 1.5f : physics.topSpeed;
    if (velX > topSpd) velX = topSpd;
    if (velX < -topSpd) velX = -topSpd;
    
    // Apply gravity
    ApplyGravity();
}

void SonicPlayer::CheckGroundCollision() {
    if (!gridLayer) return;
    
    engine::Rect box = GetBoundingBox();
    
    // Check below for ground
    int dy = 1;
    int dx = 0;
    gridLayer->FilterGridMotion(box, &dx, &dy);
    
    if (dy == 0 && velY >= 0) {
        // Hit ground
        if (!isOnGround) {
            isOnGround = true;
            groundSpeed = velX;
            if (state == SonicState::Jumping) {
                // Return to previous state or idle
                if (std::abs(groundSpeed) > 0.1f) {
                    SetState(std::abs(groundSpeed) >= physics.fullSpeedThreshold ? 
                             SonicState::FullSpeed : SonicState::Walking);
                } else {
                    SetState(SonicState::Idle);
                }
            }
        }
        velY = 0;
    } else if (velY > 0) {
        // Falling - check if we'll hit ground
        int testDy = static_cast<int>(velY);
        if (testDy > 0) {
            gridLayer->FilterGridMotion(box, &dx, &testDy);
            if (testDy < static_cast<int>(velY)) {
                // Adjust position
                posY += testDy;
                velY = 0;
                isOnGround = true;
                groundSpeed = velX;
                if (state == SonicState::Jumping) {
                    SetState(std::abs(groundSpeed) > 0.1f ? SonicState::Walking : SonicState::Idle);
                }
            } else {
                isOnGround = false;
            }
        }
    } else {
        // Moving up or stationary - check if we're still on ground
        if (isOnGround) {
            int checkDy = 2;
            gridLayer->FilterGridMotion(box, &dx, &checkDy);
            if (checkDy >= 2) {
                // No ground beneath us
                isOnGround = false;
                if (state != SonicState::Jumping && state != SonicState::Hurt) {
                    SetState(SonicState::Jumping);
                }
            }
        }
    }
    
    // Check ceiling when moving up
    if (velY < 0) {
        int testDy = static_cast<int>(velY);
        dx = 0;
        gridLayer->FilterGridMotion(box, &dx, &testDy);
        if (testDy > static_cast<int>(velY)) {
            // Hit ceiling
            posY += testDy;
            velY = 0;
        }
    }
}

void SonicPlayer::CheckWallCollision() {
    if (!gridLayer) return;
    
    // This function is now largely handled in UpdatePhysics
    // Kept for potential future use
    
    // Check horizontal collision BEFORE position was updated
    // Need to check the new position we want to go to
    int dx = static_cast<int>(velX);
    int dy = 0;
    
    if (dx != 0) {
        // Create box at current position to test movement
        // Use shorter box to avoid ground tiles being detected as walls
        int boxHeight = std::max(8, GetCurrentHeight() - 12);  // Remove bottom 12 pixels
        engine::Rect testBox = {
            static_cast<int>(posX - velX) - width / 2,
            static_cast<int>(posY) - GetCurrentHeight(),
            width,
            boxHeight
        };
        
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        
        if (std::abs(dx) < std::abs(static_cast<int>(velX))) {
            // Collision detected - adjust position
            posX = posX - velX + dx;
            velX = 0;
            groundSpeed = 0;
        }
    }
}

void SonicPlayer::UpdateState() {
    if (state == SonicState::Hurt || state == SonicState::Dead) {
        return;
    }
    
    if (!isOnGround && state != SonicState::Jumping && state != SonicState::Spring) {
        SetState(SonicState::Jumping);
        return;
    }
    
    if (isOnGround && (state == SonicState::Jumping || state == SonicState::Spring)) {
        // Just landed - use Landing state for smooth transition from spring
        if (state == SonicState::Spring) {
            // Spring landing - use brief landing animation for smooth transition
            landingEndTime = engine::GetSystemTime() + LANDING_DURATION;
            SetState(SonicState::Landing);
        } else {
            // Normal jump landing - go directly to movement state
            if (std::abs(groundSpeed) >= physics.fullSpeedThreshold) {
                SetState(SonicState::FullSpeed);
            } else if (std::abs(groundSpeed) > 0.5f) {
                SetState(SonicState::Walking);
            } else {
                SetState(SonicState::Idle);
            }
        }
        return;
    }
    
    // Handle Landing state - can be interrupted by input or times out
    if (state == SonicState::Landing) {
        bool inputActive = inputLeft || inputRight || inputJump;
        bool timedOut = engine::GetSystemTime() >= landingEndTime;
        
        // Exit landing if timed out OR player gives input
        if (timedOut || inputActive) {
            // Transition to appropriate movement state
            if (std::abs(groundSpeed) >= physics.fullSpeedThreshold) {
                SetState(SonicState::FullSpeed);
            } else if (std::abs(groundSpeed) > 0.1f || inputLeft || inputRight) {
                SetState(SonicState::Walking);
            } else {
                SetState(SonicState::Idle);
            }
        }
        // Don't return - allow normal physics to run
    }
    
    if (isOnGround) {
        float absSpeed = std::abs(groundSpeed);
        
        // Handle spindash state
        if (state == SonicState::Spindash) {
            if (!inputDown) {
                // Release spindash!
                float dashSpeed = SPINDASH_BASE_SPEED + spindashCharge;
                groundSpeed = (facing == FacingDirection::Right) ? dashSpeed : -dashSpeed;
                spindashCharge = 0.0f;
                SetState(SonicState::Rolling);
                return;
            }
            // Charge spindash when jump pressed
            if (inputJumpPressed) {
                spindashCharge = std::min(spindashCharge + SPINDASH_CHARGE_RATE, SPINDASH_MAX_CHARGE);
            }
            return;
        }
        
        // Handle crouch/lookup when standing still or slow
        if (absSpeed < 0.5f) {
            if (inputDown && !inputLeft && !inputRight) {
                if (state == SonicState::Crouching && inputJumpPressed) {
                    // Start spindash!
                    spindashCharge = 0.0f;
                    SetState(SonicState::Spindash);
                    return;
                }
                if (state != SonicState::Crouching && state != SonicState::Spindash) {
                    SetState(SonicState::Crouching);
                }
                return;
            }
            if (inputUp && !inputLeft && !inputRight && !inputJump) {
                if (state != SonicState::LookingUp) {
                    SetState(SonicState::LookingUp);
                }
                return;
            }
            // Release crouch/lookup when keys released
            if (state == SonicState::Crouching && !inputDown) {
                SetState(SonicState::Idle);
            }
            if (state == SonicState::LookingUp && !inputUp) {
                SetState(SonicState::Idle);
            }
        } else {
            // Moving - exit crouch/lookup
            if (state == SonicState::Crouching || state == SonicState::LookingUp) {
                SetState(SonicState::Walking);
            }
        }
        
        // Rolling: press down while moving
        if (inputDown && absSpeed >= 1.0f && state != SonicState::Rolling && state != SonicState::Spindash) {
            SetState(SonicState::Rolling);
            return;
        }
        
        // Check for skidding (Dami-Karv: threshold = 4.0f)
        bool tryingLeft = engine::IsKeyPressed(engine::KeyCode::Left);
        bool tryingRight = engine::IsKeyPressed(engine::KeyCode::Right);
        
        if ((groundSpeed > SKID_THRESHOLD && tryingLeft) || 
            (groundSpeed < -SKID_THRESHOLD && tryingRight)) {
            if (state != SonicState::Skidding) {
                isSkidding = true;
                SetState(SonicState::Skidding);
                return;
            }
        }
        
        // Normal state transitions (skip if in crouch/lookup)
        if (state == SonicState::Crouching || state == SonicState::LookingUp) {
            return;
        }
        
        // Use hysteresis to prevent rapid state flickering
        // Going UP requires full threshold, going DOWN requires lower threshold
        const float IDLE_THRESHOLD = 0.1f;
        const float WALK_TO_RUN_THRESHOLD = 2.0f;
        const float RUN_TO_WALK_THRESHOLD = 1.5f;  // Hysteresis
        const float RUN_TO_FULL_THRESHOLD = physics.fullSpeedThreshold;
        const float FULL_TO_RUN_THRESHOLD = physics.fullSpeedThreshold - 0.5f;  // Hysteresis
        
        if (absSpeed < IDLE_THRESHOLD) {
            if (state != SonicState::Idle && state != SonicState::Bored &&
                state != SonicState::Crouching && state != SonicState::LookingUp) {
                SetState(SonicState::Idle);
                idleStartTime = engine::GetSystemTime();
            }
            
            // Check for bored animation (Dami-Karv: 5 seconds)
            if (state == SonicState::Idle) {
                uint64_t now = engine::GetSystemTime();
                float idleSeconds = (now - idleStartTime) / 1000.0f;
                if (idleSeconds > BORED_DELAY) {
                    SetState(SonicState::Bored);
                }
            }
        } else if (absSpeed >= RUN_TO_FULL_THRESHOLD) {
            // Transition to FullSpeed
            if (state != SonicState::FullSpeed && state != SonicState::Skidding && 
                state != SonicState::Rolling) {
                SetState(SonicState::FullSpeed);
            }
        } else if (absSpeed >= WALK_TO_RUN_THRESHOLD) {
            // In Running zone
            if (state == SonicState::FullSpeed && absSpeed < FULL_TO_RUN_THRESHOLD) {
                SetState(SonicState::Running);  // Dropping from FullSpeed
            } else if (state != SonicState::Running && state != SonicState::FullSpeed && 
                       state != SonicState::Skidding && state != SonicState::Rolling) {
                SetState(SonicState::Running);  // Coming up from Walking
            }
        } else if (absSpeed >= IDLE_THRESHOLD) {
            // In Walking zone
            if (state == SonicState::Running && absSpeed < RUN_TO_WALK_THRESHOLD) {
                SetState(SonicState::Walking);  // Dropping from Running
            } else if (state != SonicState::Walking && state != SonicState::Running && 
                       state != SonicState::FullSpeed && state != SonicState::Skidding &&
                       state != SonicState::Rolling) {
                SetState(SonicState::Walking);  // Starting to move
            }
        }
        
        // Exit skidding when slow enough
        if (state == SonicState::Skidding && absSpeed < 1.0f) {
            isSkidding = false;
            SetState(SonicState::Idle);
        }
        
        // Exit rolling when too slow
        if (state == SonicState::Rolling && absSpeed < 0.5f) {
            SetState(SonicState::Idle);
        }
        
        // Balance detection: standing on edge with no ground ahead
        // Much more aggressive - trigger if ANY part of feet is over empty space
        if (gridLayer && (state == SonicState::Idle || state == SonicState::Bored || 
            state == SonicState::Walking) && absSpeed < 0.5f && !inputLeft && !inputRight) {
            engine::Rect box = GetBoundingBox();
            int footY = box.y + box.h + 2;  // Just below feet
            
            // Check multiple points along feet
            int leftFootX = box.x + 2;
            int rightFootX = box.x + box.w - 2;
            int centerX = box.x + box.w / 2;
            
            bool groundLeftFoot = gridLayer->GetTile(leftFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundRightFoot = gridLayer->GetTile(rightFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundCenter = gridLayer->GetTile(centerX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            
            // Balance if one foot is over empty space but we're still standing
            // (center or other foot must have ground)
            bool leftFootDangling = !groundLeftFoot && (groundCenter || groundRightFoot);
            bool rightFootDangling = !groundRightFoot && (groundCenter || groundLeftFoot);
            
            // Only enter balance state if not already balancing
            if ((leftFootDangling || rightFootDangling) && state != SonicState::Balancing) {
                SetState(SonicState::Balancing);
                // Set facing direction based on which side the edge is
                if (leftFootDangling) facing = FacingDirection::Left;
                if (rightFootDangling) facing = FacingDirection::Right;
            }
        }
        
        // Exit balancing when moving, pressing direction, or no longer on edge
        if (state == SonicState::Balancing) {
            // Re-check if we're still on an edge
            engine::Rect box = GetBoundingBox();
            int footY = box.y + box.h + 2;
            int leftFootX = box.x + 2;
            int rightFootX = box.x + box.w - 2;
            int centerX = box.x + box.w / 2;
            
            bool groundLeftFoot = gridLayer->GetTile(leftFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundRightFoot = gridLayer->GetTile(rightFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundCenter = gridLayer->GetTile(centerX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            
            bool stillOnEdge = (!groundLeftFoot && (groundCenter || groundRightFoot)) ||
                               (!groundRightFoot && (groundCenter || groundLeftFoot));
            
            // Exit if input given, moved, or no longer on edge
            if (absSpeed > 0.3f || inputLeft || inputRight || inputJump || !stillOnEdge) {
                SetState(SonicState::Idle);
            }
        }
        
        // Push detection: against wall while trying to move (but can't)
        // More forgiving - trigger when velocity is blocked
        if (gridLayer && (inputLeft || inputRight) && 
            state != SonicState::Pushing && state != SonicState::Crouching && 
            state != SonicState::Spindash && state != SonicState::LookingUp &&
            state != SonicState::Jumping && state != SonicState::Rolling) {
            engine::Rect box = GetBoundingBox();
            
            // Check for wall in direction of input
            int testX = inputRight ? (box.x + box.w + 4) : (box.x - 4);
            int testYTop = box.y + box.h / 4;
            int testYMid = box.y + box.h / 2;
            int testYBot = box.y + (box.h * 3) / 4;
            
            bool wallTop = gridLayer->GetTile(testX / GRID_ELEMENT_WIDTH, testYTop / GRID_ELEMENT_HEIGHT) == GRID_SOLID;
            bool wallMid = gridLayer->GetTile(testX / GRID_ELEMENT_WIDTH, testYMid / GRID_ELEMENT_HEIGHT) == GRID_SOLID;
            bool wallBot = gridLayer->GetTile(testX / GRID_ELEMENT_WIDTH, testYBot / GRID_ELEMENT_HEIGHT) == GRID_SOLID;
            
            // If any wall check hits AND we're not moving (blocked), enter push state
            if ((wallTop || wallMid || wallBot) && absSpeed < 0.3f) {
                SetState(SonicState::Pushing);
            }
        }
        
        // Exit pushing when not pressing into wall or gained speed
        if (state == SonicState::Pushing) {
            if ((!inputLeft && !inputRight) || absSpeed > 0.5f || inputJump) {
                SetState(SonicState::Idle);
            }
        }
    }
}

void SonicPlayer::ResetIdleTimer() {
    idleStartTime = engine::GetSystemTime();
    if (state == SonicState::Bored) {
        SetState(SonicState::Idle);
    }
}

void SonicPlayer::SetState(SonicState newState) {
    if (state != newState) {
        previousState = state;
        state = newState;
        if (onStateChanged) {
            onStateChanged(newState);
        }
    }
}

void SonicPlayer::UpdateAnimation() {
    // Animation updates would go here
    // For now, this is a placeholder that would switch animation films
    // based on state and facing direction
}

void SonicPlayer::StartAnimation(const std::string& animId) {
    if (currentAnimId != animId) {
        currentAnimId = animId;
        // Start the new animation using animator
    }
}

void SonicPlayer::AddRings(int count) {
    int old = rings;
    rings += count;
    if (rings >= 100) {
        // Extra life at 100 rings
        rings -= 100;
        AddLife();
    }
    if (onRingsChanged) {
        onRingsChanged(old, rings);
    }
}

void SonicPlayer::LoseRings() {
    // PDF: "WARNING if Sonic carries more than 32 rings, the rest are kept by Sonic"
    // Only scatter up to 32 rings, keep the remainder
    int toScatter = std::min(rings, 32);
    int toKeep = rings - toScatter;  // Keep rings over 32
    int old = rings;
    rings = toKeep;  // Keep excess rings (0 if had <= 32)
    
    std::cout << "LoseRings: Had " << old << ", scattering " << toScatter 
              << ", keeping " << toKeep << std::endl;
    
    // Scatter the rings using callback
    if (toScatter > 0 && onScatterRings) {
        onScatterRings(posX, posY - height / 2, toScatter);
    }
    
    if (onRingsChanged) {
        onRingsChanged(old, rings);
    }
}

void SonicPlayer::AddLife() {
    int old = lives;
    lives++;
    if (onLivesChanged) {
        onLivesChanged(old, lives);
    }
}

void SonicPlayer::LoseLife() {
    int old = lives;
    lives--;
    if (onLivesChanged) {
        onLivesChanged(old, lives);
    }
    if (lives <= 0 && onDeath) {
        onDeath();
    }
}

void SonicPlayer::AddScore(int points) {
    score += points;
    if (onScoreChanged) {
        onScoreChanged(score);
    }
}

void SonicPlayer::TakeDamage() {
    // Can't take damage if invincible
    if (isInvincible) {
        std::cout << "TakeDamage BLOCKED - still invincible!" << std::endl;
        return;
    }
    
    // Shield absorbs hit
    if (hasShield) {
        hasShield = false;
        std::cout << "Shield absorbed hit!" << std::endl;
        return;
    }
    
    if (rings > 0) {
        std::cout << "TakeDamage: Had " << rings << " rings" << std::endl;
        // Lose rings (keeps excess over 32 per PDF)
        LoseRings();
        SetState(SonicState::Hurt);
        
        // PDF: "Speed is set to the opposite of what the player had"
        // "The direction their sprite is facing, however, does not change"
        velY = -5.0f;  // Upward bounce (increased to ensure leaving ground)
        velX = -groundSpeed * 0.5f;  // Reverse horizontal movement
        if (std::abs(velX) < 2.0f) {
            // Minimum knockback if standing still
            velX = (facing == FacingDirection::Right) ? -2.0f : 2.0f;
        }
        groundSpeed = 0;
        isOnGround = false;
        // Note: facing direction NOT changed per PDF
        
        // PDF: "Player becomes invulnerable for a short time"
        // Classic Sonic uses about 3 seconds of invincibility frames
        uint64_t now = engine::GetSystemTime();
        hurtEndTime = now + 3000;  // 3 seconds
        isInvincible = true;
        inPostDamageInvincibility = true;  // Can't kill enemies during this period
        invincibleEndTime = now + 3000;
        
        std::cout << "Hurt! Knockback velX=" << velX << ", velY=" << velY 
                  << " INVINCIBLE for 3 seconds (until " << invincibleEndTime << ")" << std::endl;
    } else {
        // PDF: "If Sonic has 0 rings then he loses 1 life"
        Kill();
    }
}

void SonicPlayer::Kill() {
    // PDF: "X Speed is set to 0, and Y Speed to -7. Gravity is set to the normal value."
    SetState(SonicState::Dead);
    velX = 0;
    groundSpeed = 0;
    velY = -7.0f;  // Jump up before falling (PDF spec)
    isOnGround = false;
    LoseLife();
    std::cout << "DEATH! Lives remaining: " << lives << std::endl;
}

void SonicPlayer::Die() {
    Kill();
}

void SonicPlayer::CollectInvincibility(uint64_t duration) {
    isInvincible = true;
    hasPowerUpInvincibility = true;  // Power-up invincibility can kill enemies
    inPostDamageInvincibility = false;  // Power-up overrides post-damage
    invincibleEndTime = engine::GetSystemTime() + duration;
}

void SonicPlayer::CollectShield(uint64_t duration) {
    hasShield = true;
    shieldEndTime = engine::GetSystemTime() + duration;
}

void SonicPlayer::CollectSpeedBoost(uint64_t duration) {
    hasSpeedBoost = true;
    speedBoostEndTime = engine::GetSystemTime() + duration;
}

void SonicPlayer::Respawn(float x, float y) {
    posX = x;
    posY = y;
    velX = velY = groundSpeed = 0;
    subPixelX = subPixelY = 0;  // Clear sub-pixel accumulators
    state = SonicState::Idle;
    facing = FacingDirection::Right;
    isOnGround = false;
    isInvincible = false;
    hasPowerUpInvincibility = false;
    inPostDamageInvincibility = false;
    hasShield = false;
    hasSpeedBoost = false;
    rings = 0;  // Reset rings on respawn (death)
}

void SonicPlayer::Reset() {
    // Full reset for level restart
    rings = 0;
    lives = 3;
    score = 0;
    velX = velY = groundSpeed = 0;
    subPixelX = subPixelY = 0;  // Clear sub-pixel accumulators
    state = SonicState::Idle;
    facing = FacingDirection::Right;
    isOnGround = false;
    isInvincible = false;
    hasPowerUpInvincibility = false;
    inPostDamageInvincibility = false;
    hasShield = false;
    hasSpeedBoost = false;
    invincibleEndTime = 0;
    shieldEndTime = 0;
    speedBoostEndTime = 0;
    hurtEndTime = 0;
    idleStartTime = engine::GetSystemTime();
}

void SonicPlayer::Render(engine::Rect& viewWindow) {
    // This would render the sprite
    // For now, just a placeholder - actual rendering done in main.cpp
}

} // namespace app
