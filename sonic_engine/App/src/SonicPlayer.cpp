#include "SonicPlayer.hpp"
#include "Input.hpp"
#include "SystemClock.hpp"
#include "TileConstants.hpp"
#include "SoundManager.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace engine;

namespace app {

SonicPlayer::SonicPlayer() {}

SonicPlayer::~SonicPlayer() {
//note: sprite and animator are managed externally
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
    
//initialize tunnel system (data is hardcoded in LoadFromFile)
    tunnelSystem.LoadFromFile("");
    inTunnelMode = false;
}

int SonicPlayer::GetCurrentHeight() const {
//crouching also has reduced hitbox like ball state
    if (IsBallState() || state == SonicState::Crouching) {
        return ballHeight;
    }
    return height;
}

bool SonicPlayer::IsBallState() const {
//ball states: jumping, rolling, spindash
//also: Spring state when FALLING (velY >= 0) because Sonic curls into ball
    if (state == SonicState::Jumping || state == SonicState::Rolling || state == SonicState::Spindash) {
        return true;
    }
//spring state: only ball when falling down (going up = arms spread, not ball)
    if (state == SonicState::Spring && velY >= 0) {
        return true;
    }
    return false;
}

engine::Rect SonicPlayer::GetBoundingBox() const {
    int h = GetCurrentHeight();
//center the collision box horizontally, align to bottom
    return {
        static_cast<int>(posX) - width / 2,
        static_cast<int>(posY) - h,
        width,
        h
    };
}

void SonicPlayer::HandleInput(bool left, bool right, bool jump, bool jumpPressed, bool down, bool up) {
//disable input during tunnel sequences
    if (inTunnelMode) {
        inputLeft = false;
        inputRight = false;
        inputJump = false;
        inputJumpPressed = false;
        inputDown = false;
        inputUp = false;
        return;
    }
    
    inputLeft = left;
    inputRight = right;
    inputJump = jump;
    inputJumpPressed = jumpPressed;
    inputDown = down;
    inputUp = up;
    
//reset idle timer when any movement key is pressed (like Dami-Karv)
    if (left || right || jump || down || up) {
        ResetIdleTimer();
    }
}

void SonicPlayer::Update() {
    uint64_t now = engine::GetSystemTime();
    
    if (isInvincible && now >= invincibleEndTime) {
        isInvincible = false;
        hasPowerUpInvincibility = false;
        inPostDamageInvincibility = false;  //clear post-damage flag too
        std::cout << "Invincibility EXPIRED at " << now << std::endl;
    }
    if (hasShield && now >= shieldEndTime) {
        hasShield = false;
    }
    if (hasSpeedBoost && now >= speedBoostEndTime) {
        hasSpeedBoost = false;
    }
    
//when landing from hurt state, X Speed is set to 0
//only transition when actually falling and hitting ground (not immediately after bounce)
    if (state == SonicState::Hurt) {
//must be falling (velY > 0) and hit ground to recover
//this prevents immediate recovery when hurt bounce hasn't lifted us yet
        if (isOnGround && velY >= 0) {
            SetState(SonicState::Idle);
            velX = 0;
            groundSpeed = 0;
//note: invincibility continues after landing (handled by invincibleEndTime)
            std::cout << "Recovered from hurt (landed)" << std::endl;
        }
    }
    
//skip updates if dead
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
//skip physics when disabled (e.g., during whirl circle)
    if (!physicsEnabled) {
        return;
    }

//skip normal physics when in active loop (loop handles its own positioning)
    if (loopActive) {
//just check special tiles - loop physics are handled there
        CheckSpecialTiles();
        return;
    }

//==================== NEW TUNNEL PHYSICS ====================
    if (inTunnelMode && state == SonicState::Tunnel) {
        float facingDir = (facing == FacingDirection::Right) ? 1.0f : -1.0f;
        
        bool stillInTunnel = tunnelSystem.UpdateTunnelMovement(
            posX, posY, facingDir, 1.0f/60.0f
        );
        
        if (facingDir > 0) {
            facing = FacingDirection::Right;
        } else if (facingDir < 0) {
            facing = FacingDirection::Left;
        }
        
//exit tunnel when complete
        if (!stillInTunnel) {
            ExitTunnelMode();
        }
        
//tunnel mode - skip all other physics
        return;
    }
    
    if (state == SonicState::Hurt) {
//hurt physics - reduced control
        velY += physics.hurtGravity;
        if (velY > physics.maxFallSpeed) velY = physics.maxFallSpeed;
        
//filter movement through collision grid with sub-pixel
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
    
//filter movement through collision BEFORE applying position
    if (gridLayer) {
        engine::Rect box = GetBoundingBox();
        
//accumulate sub-pixel movement
        subPixelX += velX;
        subPixelY += velY;
        
//extract whole pixels to move
        int dx = static_cast<int>(subPixelX);
        int dy = static_cast<int>(subPixelY);
        
//keep the fractional part for next frame
        subPixelX -= dx;
        subPixelY -= dy;
        
//filter horizontal movement
//use shorter box to avoid ground tiles being detected as walls
//ground tiles (GRID_SOLID) block all directions including horizontal,
//so we need to exclude the feet area from horizontal collision checks
        if (dx != 0) {
            engine::Rect horizBox = box;
            horizBox.h = std::max(8, horizBox.h - 12);  //remove bottom 12 pixels (feet area)
            
            int testDx = dx;
            int testDy = 0;
            gridLayer->FilterGridMotion(horizBox, &testDx, &testDy);
            if (testDx != dx) {
//hit a wall - stop and clear sub-pixel accumulator
                velX = 0;
                groundSpeed = 0;
                subPixelX = 0;
                dx = testDx;
            }
        }
        
//apply horizontal movement
        posX += dx;
        box.x = static_cast<int>(posX) - width / 2;
        
//when on ground and moving horizontally, adjust Y to follow terrain
//this prevents Sonic from "floating" over or "clipping through" slopes
        if (isOnGround && dx != 0 && state != SonicState::Jumping && state != SonicState::Spring) {
//check if we're still on ground at new position
            int testDx = 0;
            int testDy = 1;
            gridLayer->FilterGridMotion(box, &testDx, &testDy);
            
            if (testDy != 0) {
//not touching ground - need to find it
                int groundOffset = 0;
                bool foundGround = false;
                
//search range based on speed - faster movement needs larger range
                int searchRange = 8 + static_cast<int>(std::abs(groundSpeed));
                if (searchRange > 16) searchRange = 16;
                
//first search below (walking down terrain)
                for (int offset = 1; offset <= searchRange; ++offset) {
                    engine::Rect testBox = box;
                    testBox.y += offset;
                    testDx = 0;
                    testDy = 1;
                    gridLayer->FilterGridMotion(testBox, &testDx, &testDy);
                    if (testDy == 0) {
                        groundOffset = offset;
                        foundGround = true;
                        break;
                    }
                }
                
//if not found below, search above (walking up terrain)
                if (!foundGround) {
                    for (int offset = 1; offset <= searchRange; ++offset) {
                        engine::Rect testBox = box;
                        testBox.y -= offset;
                        testDx = 0;
                        testDy = 1;
                        gridLayer->FilterGridMotion(testBox, &testDx, &testDy);
                        if (testDy == 0) {
                            groundOffset = -offset;
                            foundGround = true;
                            break;
                        }
                    }
                }
                
                if (foundGround) {
                    posY += groundOffset;
                    box.y = static_cast<int>(posY) - GetCurrentHeight();
                } else {
//can't find ground - we've walked off an edge
//let gravity handle it
                }
            }
        }
        
//filter vertical movement with step limiting for fast falls
        if (dy != 0) {
//for fast downward movement, step through to avoid clipping
            if (dy > GRID_ELEMENT_HEIGHT) {
                int remaining = dy;
                int step = GRID_ELEMENT_HEIGHT;
                while (remaining > 0) {
                    int thisStep = std::min(step, remaining);
                    int testDx = 0;
                    int testDy = thisStep;
                    gridLayer->FilterGridMotion(box, &testDx, &testDy);
                    if (testDy != thisStep) {
//hit ground during this step
                        posY += testDy;
                        isOnGround = true;
                        velY = 0;
                        subPixelY = 0;
                        break;
                    }
                    posY += thisStep;
                    box.y = static_cast<int>(posY) - GetCurrentHeight();
                    remaining -= thisStep;
                }
            } else {
                int testDx = 0;
                int testDy = dy;
                gridLayer->FilterGridMotion(box, &testDx, &testDy);
                if (testDy != dy) {
                    if (dy > 0) {
//hit ground
                        isOnGround = true;
                    }
                    velY = 0;
                    subPixelY = 0;  //clear vertical sub-pixel on landing
                    dy = testDy;
                }
//apply vertical movement
                posY += dy;
            }
        }
        
//snap to slope when on ground (prevents bouncing on slopes)
        if (isOnGround && state != SonicState::Jumping && state != SonicState::Spring) {
            SnapToGround();
        }
    } else {
        posX += velX;
        posY += velY;
    }
    
//additional ground check
    CheckGroundCollision();
    
//check for special tiles (tunnel, loop)
    CheckSpecialTiles();
    
//push out of walls (handles cases where Sonic clips through)
    ResolveWallOverlap();
}

void SonicPlayer::CheckSpecialTiles() {
    if (!gridLayer) return;
    if (state == SonicState::Dead || state == SonicState::Hurt) return;
    
    engine::Rect box = GetBoundingBox();
    int centerX = static_cast<int>(posX);
    int feetY = static_cast<int>(posY);
    
//==================== TUNNEL DETECTION ====================
    if (!inTunnelMode) {
//check if Sonic is near a tunnel entrance
        int tunnelIndex = tunnelSystem.CheckTunnelEntry(posX, posY);
        
        if (tunnelIndex >= 0) {
            EnterTunnelMode(tunnelIndex);
        }
    }
    
//==================== LOOP HANDLING ====================
    bool wasInLoop = inLoop;
    inLoop = gridLayer->IsInLoop(box);
    
    if (inLoop) {
        float absSpeed = std::abs(groundSpeed);
        
//detect if we're entering a loop
        if (!loopActive && absSpeed >= LOOP_MIN_SPEED) {
//find loop center - look for the center of loop tiles
//loop tiles form a circular structure
            int col = centerX / GRID_ELEMENT_WIDTH;
            int row = feetY / GRID_ELEMENT_HEIGHT;
            
//scan to find approximate loop center
//look for where loop tiles surround an empty center
            int loopTileCount = 0;
            int sumX = 0, sumY = 0;
            
            for (int scanRow = row - 8; scanRow <= row + 8; ++scanRow) {
                for (int scanCol = col - 8; scanCol <= col + 8; ++scanCol) {
                    if (gridLayer->GetTile(scanCol, scanRow) == GRID_LOOP) {
                        sumX += scanCol;
                        sumY += scanRow;
                        loopTileCount++;
                    }
                }
            }
            
            if (loopTileCount > 10) {
//found a loop structure - calculate center
                loopCenterX = (sumX / loopTileCount) * GRID_ELEMENT_WIDTH + GRID_ELEMENT_WIDTH / 2;
                loopCenterY = (sumY / loopTileCount) * GRID_ELEMENT_HEIGHT + GRID_ELEMENT_HEIGHT / 2;
                
//estimate radius from distance to center
                float dx = posX - loopCenterX;
                float dy = posY - loopCenterY;
                loopRadius = std::sqrt(dx * dx + dy * dy);
                if (loopRadius < 32) loopRadius = 48;  //minimum radius
                if (loopRadius > 80) loopRadius = 64;  //maximum radius
                
//calculate starting angle
                loopAngle = std::atan2(dy, dx) * 180.0f / 3.14159f;
                
                loopActive = true;
            }
        }
        
//process active loop
        if (loopActive) {
            float absSpeed = std::abs(groundSpeed);
            
//check if we have enough speed to continue
            if (absSpeed < LOOP_MIN_SPEED * 0.7f) {
//too slow - fall off loop
                loopActive = false;
                isOnGround = false;
                SetState(SonicState::Jumping);
            } else {
//continue loop traversal
//angular velocity based on ground speed
                float angularSpeed = (groundSpeed / loopRadius) * (180.0f / 3.14159f);
                loopAngle += angularSpeed;
                
//normalize angle
                while (loopAngle >= 360.0f) loopAngle -= 360.0f;
                while (loopAngle < 0.0f) loopAngle += 360.0f;
                
//calculate position on loop
                float angleRad = loopAngle * 3.14159f / 180.0f;
                posX = loopCenterX + loopRadius * std::cos(angleRad);
                posY = loopCenterY + loopRadius * std::sin(angleRad);
                
//set ground angle based on loop position
                groundAngle = loopAngle + 90.0f;  //perpendicular to radius
                while (groundAngle >= 180.0f) groundAngle -= 360.0f;
                while (groundAngle < -180.0f) groundAngle += 360.0f;
                
//apply centripetal "gravity" - slows down going up, speeds up going down
                float gravityEffect = std::sin(angleRad) * 0.3f;
                groundSpeed -= gravityEffect;
                
//keep Sonic on ground during loop
                isOnGround = true;
                velY = 0;
                
//use rolling state during loop
                if (state != SonicState::Rolling && state != SonicState::Jumping) {
                    SetState(SonicState::Rolling);
                }
            }
        }
        
    } else if (wasInLoop && loopActive) {
//exiting loop
        loopActive = false;
        groundAngle = 0;
//convert loop velocity back to normal
        velX = groundSpeed;
        velY = 0;
    }
}

void SonicPlayer::ResolveWallOverlap() {
    if (!gridLayer) return;
    if (state == SonicState::Dead) return;  //let dead Sonic fall through everything
    
    engine::Rect box = GetBoundingBox();
    
    int topRow = box.y / GRID_ELEMENT_HEIGHT;
    int bottomRow = (box.y + box.h - 1) / GRID_ELEMENT_HEIGHT;
    
//clamp rows to valid range
    if (topRow < 0) topRow = 0;
    if (bottomRow < 0) return;  //above map, no wall collision needed
    
//check if we're inside any solid tiles and push out
    int leftEdgeCol = box.x / GRID_ELEMENT_WIDTH;
    int rightEdgeCol = (box.x + box.w - 1) / GRID_ELEMENT_WIDTH;
    int centerCol = (box.x + box.w / 2) / GRID_ELEMENT_WIDTH;
    
//=== GROUND EMBEDDED CHECK ===
//if Sonic's feet are embedded in solid ground, push him up
//this handles cases where Sonic clips through floor when moving fast
    int feetRow = bottomRow;
    bool feetInGround = false;
    for (int col = leftEdgeCol; col <= rightEdgeCol; ++col) {
        GridIndex tile = gridLayer->GetTile(col, feetRow);
        if (tile == GRID_SOLID) {
            feetInGround = true;
            break;
        }
    }
    
    if (feetInGround && velY >= 0) {
//find the top of the ground we're embedded in
        int groundTop = feetRow * GRID_ELEMENT_HEIGHT;
        float newPosY = groundTop;  //posY is at feet level
        if (posY > newPosY) {
            posY = newPosY;
            velY = 0;
            isOnGround = true;
//don't override tunnel/rolling state when landing
            if ((state == SonicState::Jumping || state == SonicState::Spring) &&
                state != SonicState::Tunnel && state != SonicState::Rolling) {
                groundSpeed = velX;
                SetState(std::abs(groundSpeed) > 0.1f ? SonicState::Walking : SonicState::Idle);
            }
        }
    }
    
//refresh box after potential Y adjustment
    box = GetBoundingBox();
    topRow = box.y / GRID_ELEMENT_HEIGHT;
    bottomRow = (box.y + box.h - 1) / GRID_ELEMENT_HEIGHT;
    if (topRow < 0) topRow = 0;
    leftEdgeCol = box.x / GRID_ELEMENT_WIDTH;
    rightEdgeCol = (box.x + box.w - 1) / GRID_ELEMENT_WIDTH;
    centerCol = (box.x + box.w / 2) / GRID_ELEMENT_WIDTH;
    
//=== CEILING CHECK (jumping into solid from below) ===
//check if top of Sonic is inside a solid tile
    bool topBlocked = false;
    for (int col = leftEdgeCol; col <= rightEdgeCol; ++col) {
        GridIndex tile = gridLayer->GetTile(col, topRow);
//GRID_SOLID blocks from all directions (ceiling)
        if (tile == GRID_SOLID) {
            topBlocked = true;
            break;
        }
    }
    
    if (topBlocked && velY < 0) {
//push down - find bottom edge of ceiling
        int ceilingBottom = (topRow + 1) * GRID_ELEMENT_HEIGHT;
        float newPosY = ceilingBottom + GetCurrentHeight() + 1;
        if (posY < newPosY) {
            posY = newPosY;
            velY = 0;  //stop upward movement
        }
    }
    
//refresh box after potential Y adjustment
    box = GetBoundingBox();
    topRow = box.y / GRID_ELEMENT_HEIGHT;
    bottomRow = (box.y + box.h - 1) / GRID_ELEMENT_HEIGHT;
    if (topRow < 0) topRow = 0;
    leftEdgeCol = box.x / GRID_ELEMENT_WIDTH;
    rightEdgeCol = (box.x + box.w - 1) / GRID_ELEMENT_WIDTH;
    
//=== HORIZONTAL WALL CHECK ===
//use shorter box to avoid ground tiles being detected as walls
//exclude bottom STEP_HEIGHT pixels for step-up tolerance
    engine::Rect horizBox = box;
    horizBox.h = std::max(8, horizBox.h - STEP_HEIGHT - 4);
    int horizTopRow = horizBox.y / GRID_ELEMENT_HEIGHT;
    int horizBottomRow = (horizBox.y + horizBox.h - 1) / GRID_ELEMENT_HEIGHT;
    if (horizTopRow < 0) horizTopRow = 0;
    
//helper: check if tile is a wall (not slope or tunnel - those are passable)
    auto isWallTile = [this](GridIndex tile) {
//TUNNEL is NOT a wall when in tunnel mode
        if (inTunnelMode && tile == GRID_TUNNEL) return false;
        return tile == GRID_SOLID || tile == GRID_LOOP;
    };
    
//check if right side is in a wall
    bool rightBlocked = false;
    for (int row = horizTopRow; row <= horizBottomRow; ++row) {
        GridIndex tile = gridLayer->GetTile(rightEdgeCol, row);
        if (isWallTile(tile)) {
            rightBlocked = true;
            break;
        }
    }
    
//check if left side is in a wall
    bool leftBlocked = false;
    for (int row = horizTopRow; row <= horizBottomRow; ++row) {
        GridIndex tile = gridLayer->GetTile(leftEdgeCol, row);
        if (isWallTile(tile)) {
            leftBlocked = true;
            break;
        }
    }
    
//also check center column for cases where Sonic is fully embedded
    bool centerBlocked = false;
    for (int row = horizTopRow; row <= horizBottomRow; ++row) {
        GridIndex tile = gridLayer->GetTile(centerCol, row);
        if (isWallTile(tile)) {
            centerBlocked = true;
            break;
        }
    }
    
//push out of walls
    if (rightBlocked && !leftBlocked) {
//push left - find the left edge of the wall
        int wallLeftEdge = rightEdgeCol * GRID_ELEMENT_WIDTH;
        float newPosX = wallLeftEdge - (box.w / 2.0f) - 1;
        if (posX > newPosX) {
            posX = newPosX;
            velX = 0;
            groundSpeed = 0;
        }
    } else if (leftBlocked && !rightBlocked) {
//push right - find the right edge of the wall
        int wallRightEdge = (leftEdgeCol + 1) * GRID_ELEMENT_WIDTH;
        float newPosX = wallRightEdge + (box.w / 2.0f) + 1;
        if (posX < newPosX) {
            posX = newPosX;
            velX = 0;
            groundSpeed = 0;
        }
    } else if (centerBlocked || (rightBlocked && leftBlocked)) {
//fully embedded in wall - push based on velocity direction or facing
        velX = 0;
        groundSpeed = 0;
        
//try to find an exit direction
//check how far we'd need to go in each direction
        int pushLeft = 0, pushRight = 0;
        for (int testCol = centerCol; testCol >= 0; --testCol) {
            bool blocked = false;
            for (int row = horizTopRow; row <= horizBottomRow; ++row) {
                GridIndex tile = gridLayer->GetTile(testCol, row);
                if (isWallTile(tile)) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) {
                pushLeft = centerCol - testCol;
                break;
            }
            pushLeft++;
        }
        for (int testCol = centerCol; testCol < static_cast<int>(gridLayer->GetCols()); ++testCol) {
            bool blocked = false;
            for (int row = horizTopRow; row <= horizBottomRow; ++row) {
                GridIndex tile = gridLayer->GetTile(testCol, row);
                if (isWallTile(tile)) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) {
                pushRight = testCol - centerCol;
                break;
            }
            pushRight++;
        }
        
//push in shorter direction
        if (pushLeft <= pushRight && pushLeft < 10) {
            posX -= pushLeft * GRID_ELEMENT_WIDTH + box.w / 2 + 1;
        } else if (pushRight < 10) {
            posX += pushRight * GRID_ELEMENT_WIDTH + box.w / 2 + 1;
        }
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
    
//=== SLOPE PHYSICS - Apply on slope tiles for speed boost/reduction ===
    const float SLOPE_FACTOR = 0.125f;  //gravity factor on slopes (Genesis value)
    
//only apply slope gravity on actual slope tiles
    if (onSlopeTile && std::abs(groundAngle) > 5.0f) {
        float angleRad = groundAngle * 3.14159f / 180.0f;
        float slopeGravity = std::sin(angleRad) * SLOPE_FACTOR;
        groundSpeed -= slopeGravity;
    }
    
//acceleration and deceleration
    if (inputLeft && !inputRight) {
        if (groundSpeed > 0) {
//braking
            groundSpeed -= physics.deceleration;
            if (groundSpeed < 0) groundSpeed = -physics.deceleration;
        } else if (groundSpeed > -topSpd) {
//accelerating left
            float accel = physics.acceleration;
//slope boost only on slope tiles
            if (onSlopeTile) {
                if (groundAngle > 10.0f) accel *= 1.3f;  //boost going downhill
                else if (groundAngle < -10.0f) accel *= 0.7f;  //harder going uphill
            }
            groundSpeed -= accel;
            if (groundSpeed < -topSpd) groundSpeed = -topSpd;
        }
        facing = FacingDirection::Left;
    } else if (inputRight && !inputLeft) {
        if (groundSpeed < 0) {
//braking
            groundSpeed += physics.deceleration;
            if (groundSpeed > 0) groundSpeed = physics.deceleration;
        } else if (groundSpeed < topSpd) {
//accelerating right
            float accel = physics.acceleration;
//slope boost only on slope tiles
            if (onSlopeTile) {
                if (groundAngle < -10.0f) accel *= 1.3f;  //boost going downhill
                else if (groundAngle > 10.0f) accel *= 0.7f;  //harder going uphill
            }
            groundSpeed += accel;
            if (groundSpeed > topSpd) groundSpeed = topSpd;
        }
        facing = FacingDirection::Right;
    } else {
//apply friction when no input
        float fric = physics.friction;
//less friction only on actual slope tiles
        if (onSlopeTile && std::abs(groundAngle) > 15.0f) {
            fric *= 0.5f;
        }
        
        if (groundSpeed > 0) {
            groundSpeed -= fric;
            if (groundSpeed < 0) groundSpeed = 0;
        } else if (groundSpeed < 0) {
            groundSpeed += fric;
            if (groundSpeed > 0) groundSpeed = 0;
        }
    }
    
//allow higher speeds on slope tiles
    if (onSlopeTile && std::abs(groundAngle) > 20.0f) {
        float maxSlopeSpeed = topSpd * 1.5f;
        if (groundSpeed > maxSlopeSpeed) groundSpeed = maxSlopeSpeed;
        if (groundSpeed < -maxSlopeSpeed) groundSpeed = -maxSlopeSpeed;
    }
    
//ground speed to velocity
    velX = groundSpeed;
    velY = 0;
    
//jump
    if (inputJumpPressed && state != SonicState::Hurt) {
        velY = -physics.jumpForce;
        isOnGround = false;
        SetState(SonicState::Jumping);
        SoundManager::Instance().OnJump();
    }
}

void SonicPlayer::ApplyAirMovement() {
//air drag and control
    if (inputLeft && !inputRight) {
        velX -= physics.airAcceleration;
        facing = FacingDirection::Left;
    } else if (inputRight && !inputLeft) {
        velX += physics.airAcceleration;
        facing = FacingDirection::Right;
    }
    
//clamp air speed
    float topSpd = hasSpeedBoost ? physics.topSpeed * 1.5f : physics.topSpeed;
    if (velX > topSpd) velX = topSpd;
    if (velX < -topSpd) velX = -topSpd;
    
//apply gravity
    ApplyGravity();
}

void SonicPlayer::CheckGroundCollision() {
    if (!gridLayer) return;
    
    engine::Rect box = GetBoundingBox();
    
//check below for ground
    int dy = 1;
    int dx = 0;
    gridLayer->FilterGridMotion(box, &dx, &dy);
    
    if (dy == 0 && velY >= 0) {
//hit ground
        if (!isOnGround) {
            isOnGround = true;
            groundSpeed = velX;
            if (state == SonicState::Jumping) {
//return to previous state or idle
                if (std::abs(groundSpeed) > 0.1f) {
                    SetState(std::abs(groundSpeed) >= physics.fullSpeedThreshold ? 
                             SonicState::FullSpeed : SonicState::Walking);
                } else {
                    SetState(SonicState::Idle);
                }
            }
        }
        velY = 0;
        
//detect ground slope by sampling left and right
        DetectGroundSlope();
    } else if (velY > 0) {
//falling - check if we'll hit ground
        int testDy = static_cast<int>(velY);
        if (testDy > 0) {
            gridLayer->FilterGridMotion(box, &dx, &testDy);
            if (testDy < static_cast<int>(velY)) {
//adjust position
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
//moving up or stationary - check if we're still on ground
        if (isOnGround) {
//use a larger check distance to handle slopes properly
//on slopes, ground might be offset horizontally
            int checkDy = 12;  //increased check distance for slopes
            gridLayer->FilterGridMotion(box, &dx, &checkDy);
            
//check multiple points - center, left, and right of feet
            bool foundGround = (checkDy < 12);
            
            if (!foundGround) {
//try checking to left for downward slopes
                engine::Rect leftBox = box;
                leftBox.x -= 8;
                leftBox.w = 12;
                int leftDy = 12;
                gridLayer->FilterGridMotion(leftBox, &dx, &leftDy);
                if (leftDy < 12) foundGround = true;
            }
            
            if (!foundGround) {
//try checking to right for upward slopes
                engine::Rect rightBox = box;
                rightBox.x += box.w - 4;
                rightBox.w = 12;
                int rightDy = 12;
                gridLayer->FilterGridMotion(rightBox, &dx, &rightDy);
                if (rightDy < 12) foundGround = true;
            }
            
//also check below-left and below-right for diagonal slopes
            if (!foundGround) {
                engine::Rect belowBox = box;
                belowBox.y += 4;  //check slightly below current position
                int belowDy = 8;
                gridLayer->FilterGridMotion(belowBox, &dx, &belowDy);
                if (belowDy < 8) foundGround = true;
            }
            
            if (!foundGround) {
//no ground found anywhere - we're truly airborne
                isOnGround = false;
                groundAngle = 0;  //reset angle when airborne
                if (state != SonicState::Jumping && state != SonicState::Hurt && 
                    state != SonicState::Spring && state != SonicState::Rolling) {
                    SetState(SonicState::Jumping);
                }
            }
        }
    }
    
//check ceiling when moving up
    if (velY < 0) {
        int testDy = static_cast<int>(velY);
        dx = 0;
        gridLayer->FilterGridMotion(box, &dx, &testDy);
        if (testDy > static_cast<int>(velY)) {
//hit ceiling
            posY += testDy;
            velY = 0;
        }
    }
}

void SonicPlayer::SnapToGround() {
    if (!gridLayer || !isOnGround) return;
    
    engine::Rect box = GetBoundingBox();
    
//check if we're currently ON ground (touching it)
    int checkDy = 1;
    int checkDx = 0;
    gridLayer->FilterGridMotion(box, &checkDx, &checkDy);
    if (checkDy == 0) {
//already touching ground, nothing to do
        return;
    }
    
//search range based on speed
    int searchRange = 8 + static_cast<int>(std::abs(groundSpeed));
    if (searchRange > 16) searchRange = 16;
    
//try to find ground below current position
    for (int checkY = 1; checkY <= searchRange; ++checkY) {
        engine::Rect testBox = box;
        testBox.y += checkY;
        int dx = 0, dy = 1;
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        if (dy == 0) {
//found ground at this offset - snap to it
            posY += checkY;
            return;
        }
    }
    
//also check above (in case we're embedded in terrain)
    for (int checkY = 1; checkY <= 8; ++checkY) {
        engine::Rect testBox = box;
        testBox.y -= checkY;
        int dx = 0, dy = 1;
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        if (dy == 0) {
            posY -= checkY;
            return;
        }
    }
    
//if we can't find ground, we might have walked off an edge
}

void SonicPlayer::DetectGroundSlope() {
    if (!gridLayer || !isOnGround) {
        groundAngle = 0;
        onSlopeTile = false;
        return;
    }
    
//sample ground height at multiple points to determine slope angle
    int centerX = static_cast<int>(posX);
    int feetY = static_cast<int>(posY);
    
//use multiple sample distances for accuracy
    int sampleDist = 6;  //pixels from center
    
//find ground Y at left point
    int leftX = centerX - sampleDist;
    int leftGroundY = feetY;
    bool leftFound = false;
    
    for (int testY = feetY - 12; testY <= feetY + 16; ++testY) {
        engine::Rect testBox = {leftX - 1, testY, 2, 2};
        int dx = 0, dy = 1;
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        if (dy == 0) {
            leftGroundY = testY;
            leftFound = true;
            break;
        }
    }
    
//find ground Y at right point
    int rightX = centerX + sampleDist;
    int rightGroundY = feetY;
    bool rightFound = false;
    
    for (int testY = feetY - 12; testY <= feetY + 16; ++testY) {
        engine::Rect testBox = {rightX - 1, testY, 2, 2};
        int dx = 0, dy = 1;
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        if (dy == 0) {
            rightGroundY = testY;
            rightFound = true;
            break;
        }
    }
    
//calculate angle if both points found
    if (leftFound && rightFound) {
        int heightDiff = rightGroundY - leftGroundY;
        float slopeRatio = static_cast<float>(heightDiff) / (sampleDist * 2);
        groundAngle = std::atan(slopeRatio) * 180.0f / 3.14159f;
        
//clamp angle
        if (groundAngle > 45.0f) groundAngle = 45.0f;
        if (groundAngle < -45.0f) groundAngle = -45.0f;
    } else {
        groundAngle = 0;
    }
    
//check if we're on an actual SLOPE tile (for special slope physics)
    int feetCol = centerX / GRID_ELEMENT_WIDTH;
    int feetRow = feetY / GRID_ELEMENT_HEIGHT;
    int belowRow = feetRow + 1;
    
    GridIndex tileAtFeet = gridLayer->GetTile(feetCol, feetRow);
    GridIndex tileBelow = gridLayer->GetTile(feetCol, belowRow);
    
    onSlopeTile = (tileAtFeet == GRID_SLOPE || tileBelow == GRID_SLOPE);
}

void SonicPlayer::CheckWallCollision() {
    if (!gridLayer) return;
    
//this function is now largely handled in UpdatePhysics
//kept for potential future use
    
//check horizontal collision BEFORE position was updated
//need to check the new position we want to go to
    int dx = static_cast<int>(velX);
    int dy = 0;
    
    if (dx != 0) {
//create box at current position to test movement
//use shorter box to avoid ground tiles being detected as walls
        int boxHeight = std::max(8, GetCurrentHeight() - 12);  //remove bottom 12 pixels
        engine::Rect testBox = {
            static_cast<int>(posX - velX) - width / 2,
            static_cast<int>(posY) - GetCurrentHeight(),
            width,
            boxHeight
        };
        
        gridLayer->FilterGridMotion(testBox, &dx, &dy);
        
        if (std::abs(dx) < std::abs(static_cast<int>(velX))) {
//collision detected - adjust position
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
    
//don't override tunnel or rolling states - they handle their own physics
    if (state == SonicState::Tunnel || state == SonicState::Rolling) {
        return;
    }
    
    if (!isOnGround && state != SonicState::Jumping && state != SonicState::Spring) {
        SetState(SonicState::Jumping);
        return;
    }
    
    if (isOnGround && (state == SonicState::Jumping || state == SonicState::Spring)) {
//just landed - use Landing state for smooth transition from spring
        if (state == SonicState::Spring) {
//spring landing - use brief landing animation for smooth transition
            landingEndTime = engine::GetSystemTime() + LANDING_DURATION;
            SetState(SonicState::Landing);
        } else {
//normal jump landing - go directly to movement state
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
    
//handle Landing state - can be interrupted by input or times out
    if (state == SonicState::Landing) {
        bool inputActive = inputLeft || inputRight || inputJump;
        bool timedOut = engine::GetSystemTime() >= landingEndTime;
        
//exit landing if timed out OR player gives input
        if (timedOut || inputActive) {
//transition to appropriate movement state
            if (std::abs(groundSpeed) >= physics.fullSpeedThreshold) {
                SetState(SonicState::FullSpeed);
            } else if (std::abs(groundSpeed) > 0.1f || inputLeft || inputRight) {
                SetState(SonicState::Walking);
            } else {
                SetState(SonicState::Idle);
            }
        }
//don't return - allow normal physics to run
    }
    
    if (isOnGround) {
        float absSpeed = std::abs(groundSpeed);
        
//handle spindash state
        if (state == SonicState::Spindash) {
            if (!inputDown) {
//release spindash!
                float dashSpeed = SPINDASH_BASE_SPEED + spindashCharge;
                groundSpeed = (facing == FacingDirection::Right) ? dashSpeed : -dashSpeed;
                spindashCharge = 0.0f;
                SetState(SonicState::Rolling);
                return;
            }
//charge spindash when jump pressed
            if (inputJumpPressed) {
                spindashCharge = std::min(spindashCharge + SPINDASH_CHARGE_RATE, SPINDASH_MAX_CHARGE);
            }
            return;
        }
        
//handle crouch/lookup when standing still or slow
        if (absSpeed < 0.5f) {
            if (inputDown && !inputLeft && !inputRight) {
                if (state == SonicState::Crouching && inputJumpPressed) {
//start spindash!
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
//release crouch/lookup when keys released
            if (state == SonicState::Crouching && !inputDown) {
                SetState(SonicState::Idle);
            }
            if (state == SonicState::LookingUp && !inputUp) {
                SetState(SonicState::Idle);
            }
        } else {
//moving - exit crouch/lookup
            if (state == SonicState::Crouching || state == SonicState::LookingUp) {
                SetState(SonicState::Walking);
            }
        }
        
//rolling: press down while moving
        if (inputDown && absSpeed >= 1.0f && state != SonicState::Rolling && state != SonicState::Spindash) {
            SetState(SonicState::Rolling);
            return;
        }
        
//check for skidding (Dami-Karv: threshold = 4.0f)
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
        
//normal state transitions (skip if in crouch/lookup)
        if (state == SonicState::Crouching || state == SonicState::LookingUp) {
            return;
        }
        
//use hysteresis to prevent rapid state flickering
//going UP requires full threshold, going DOWN requires lower threshold
        const float IDLE_THRESHOLD = 0.1f;
        const float WALK_TO_RUN_THRESHOLD = 2.0f;
        const float RUN_TO_WALK_THRESHOLD = 1.5f;  //hysteresis
        const float RUN_TO_FULL_THRESHOLD = physics.fullSpeedThreshold;
        const float FULL_TO_RUN_THRESHOLD = physics.fullSpeedThreshold - 0.5f;  //hysteresis
        
        if (absSpeed < IDLE_THRESHOLD) {
            if (state != SonicState::Idle && state != SonicState::Bored &&
                state != SonicState::Crouching && state != SonicState::LookingUp) {
                SetState(SonicState::Idle);
                idleStartTime = engine::GetSystemTime();
            }
            
//check for bored animation (Dami-Karv: 5 seconds)
            if (state == SonicState::Idle) {
                uint64_t now = engine::GetSystemTime();
                float idleSeconds = (now - idleStartTime) / 1000.0f;
                if (idleSeconds > BORED_DELAY) {
                    SetState(SonicState::Bored);
                }
            }
        } else if (absSpeed >= RUN_TO_FULL_THRESHOLD) {
//transition to FullSpeed
            if (state != SonicState::FullSpeed && state != SonicState::Skidding && 
                state != SonicState::Rolling) {
                SetState(SonicState::FullSpeed);
            }
        } else if (absSpeed >= WALK_TO_RUN_THRESHOLD) {
//in Running zone
            if (state == SonicState::FullSpeed && absSpeed < FULL_TO_RUN_THRESHOLD) {
                SetState(SonicState::Running);  //dropping from FullSpeed
            } else if (state != SonicState::Running && state != SonicState::FullSpeed && 
                       state != SonicState::Skidding && state != SonicState::Rolling) {
                SetState(SonicState::Running);  //coming up from Walking
            }
        } else if (absSpeed >= IDLE_THRESHOLD) {
//in Walking zone
            if (state == SonicState::Running && absSpeed < RUN_TO_WALK_THRESHOLD) {
                SetState(SonicState::Walking);  //dropping from Running
            } else if (state != SonicState::Walking && state != SonicState::Running && 
                       state != SonicState::FullSpeed && state != SonicState::Skidding &&
                       state != SonicState::Rolling) {
                SetState(SonicState::Walking);  //starting to move
            }
        }
        
//exit skidding when slow enough
        if (state == SonicState::Skidding && absSpeed < 1.0f) {
            isSkidding = false;
            SetState(SonicState::Idle);
        }
        
//exit rolling when too slow
        if (state == SonicState::Rolling && absSpeed < 0.5f) {
            SetState(SonicState::Idle);
        }
        
//balance detection: standing on edge with no ground ahead
//much more aggressive - trigger if ANY part of feet is over empty space
        if (gridLayer && (state == SonicState::Idle || state == SonicState::Bored || 
            state == SonicState::Walking) && absSpeed < 0.5f && !inputLeft && !inputRight) {
            engine::Rect box = GetBoundingBox();
            int footY = box.y + box.h + 2;  //just below feet
            
//check multiple points along feet
            int leftFootX = box.x + 2;
            int rightFootX = box.x + box.w - 2;
            int centerX = box.x + box.w / 2;
            
            bool groundLeftFoot = gridLayer->GetTile(leftFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundRightFoot = gridLayer->GetTile(rightFootX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            bool groundCenter = gridLayer->GetTile(centerX / GRID_ELEMENT_WIDTH, footY / GRID_ELEMENT_HEIGHT) != GRID_EMPTY;
            
//balance if one foot is over empty space but we're still standing
//(center or other foot must have ground)
            bool leftFootDangling = !groundLeftFoot && (groundCenter || groundRightFoot);
            bool rightFootDangling = !groundRightFoot && (groundCenter || groundLeftFoot);
            
//only enter balance state if not already balancing
            if ((leftFootDangling || rightFootDangling) && state != SonicState::Balancing) {
                SetState(SonicState::Balancing);
//set facing direction based on which side the edge is
                if (leftFootDangling) facing = FacingDirection::Left;
                if (rightFootDangling) facing = FacingDirection::Right;
            }
        }
        
//exit balancing when moving, pressing direction, or no longer on edge
        if (state == SonicState::Balancing) {
//re-check if we're still on an edge
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
            
//exit if input given, moved, or no longer on edge
            if (absSpeed > 0.3f || inputLeft || inputRight || inputJump || !stillOnEdge) {
                SetState(SonicState::Idle);
            }
        }
        
//push detection: against wall while trying to move (but can't)
//more forgiving - trigger when velocity is blocked
        if (gridLayer && (inputLeft || inputRight) && 
            state != SonicState::Pushing && state != SonicState::Crouching && 
            state != SonicState::Spindash && state != SonicState::LookingUp &&
            state != SonicState::Jumping && state != SonicState::Rolling) {
            engine::Rect box = GetBoundingBox();
            
//check for wall in direction of input - check upper body only (not feet)
            int testX = inputRight ? (box.x + box.w + 4) : (box.x - 4);
            int testYTop = box.y + box.h / 4;
            int testYMid = box.y + box.h / 2;
//don't check at foot level to avoid slopes triggering push
            
            auto isWallTile = [](GridIndex tile) {
//SOLID, LOOP, TUNNEL are walls - SLOPE is not
                return tile == GRID_SOLID || tile == GRID_LOOP || tile == GRID_TUNNEL;
            };
            
            GridIndex tileTop = gridLayer->GetTile(testX / GRID_ELEMENT_WIDTH, testYTop / GRID_ELEMENT_HEIGHT);
            GridIndex tileMid = gridLayer->GetTile(testX / GRID_ELEMENT_WIDTH, testYMid / GRID_ELEMENT_HEIGHT);
            
            bool wallTop = isWallTile(tileTop);
            bool wallMid = isWallTile(tileMid);
            
//if wall detected at upper body AND we're not moving (blocked), enter push state
            if ((wallTop || wallMid) && absSpeed < 0.3f) {
                SetState(SonicState::Pushing);
            }
        }
        
//exit pushing when not pressing into wall or gained speed
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
//animation updates would go here
//for now, this is a placeholder that would switch animation films
//based on state and facing direction
}

void SonicPlayer::StartAnimation(const std::string& animId) {
    if (currentAnimId != animId) {
        currentAnimId = animId;
//start the new animation using animator
    }
}

void SonicPlayer::AddRings(int count) {
    int old = rings;
    rings += count;
    if (rings >= 100) {
//extra life at 100 rings
        rings -= 100;
        AddLife();
    }
    if (onRingsChanged) {
        onRingsChanged(old, rings);
    }
}

void SonicPlayer::LoseRings() {
//only scatter up to 32 rings, keep the remainder
    int toScatter = std::min(rings, 32);
    int toKeep = rings - toScatter;  //keep rings over 32
    int old = rings;
    rings = toKeep;  //keep excess rings (0 if had <= 32)
    
    std::cout << "LoseRings: Had " << old << ", scattering " << toScatter 
              << ", keeping " << toKeep << std::endl;
    
//scatter the rings using callback
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
//NOTE: onDeath callback removed - game over handled in main.cpp death logic
//dual logic was causing "GAME OVER" to trigger twice
}

void SonicPlayer::AddScore(int points) {
    score += points;
    if (onScoreChanged) {
        onScoreChanged(score);
    }
}

void SonicPlayer::TakeDamage() {
//CRITICAL: Can't take damage if already dead
    if (state == SonicState::Dead) {
        std::cout << "TakeDamage BLOCKED - already dead!" << std::endl;
        return;
    }
    
//can't take damage if invincible
    if (isInvincible) {
        std::cout << "TakeDamage BLOCKED - still invincible!" << std::endl;
        return;
    }
    
//shield absorbs hit
    if (hasShield) {
        hasShield = false;
        std::cout << "Shield absorbed hit!" << std::endl;
        return;
    }
    
    if (rings > 0) {
        std::cout << "TakeDamage: Had " << rings << " rings" << std::endl;
        LoseRings();
        SetState(SonicState::Hurt);
        
//"The direction their sprite is facing, however, does not change"
        velY = -5.0f;  //upward bounce (increased to ensure leaving ground)
        velX = -groundSpeed * 0.5f;  //reverse horizontal movement
        if (std::abs(velX) < 2.0f) {
//minimum knockback if standing still
            velX = (facing == FacingDirection::Right) ? -2.0f : 2.0f;
        }
        groundSpeed = 0;
        isOnGround = false;
        
//classic Sonic uses about 3 seconds of invincibility frames
        uint64_t now = engine::GetSystemTime();
        hurtEndTime = now + 3000;  //3 seconds
        isInvincible = true;
        inPostDamageInvincibility = true;  //can't kill enemies during this period
        invincibleEndTime = now + 3000;
        
        std::cout << "Hurt! Knockback velX=" << velX << ", velY=" << velY 
                  << " INVINCIBLE for 3 seconds (until " << invincibleEndTime << ")" << std::endl;
    } else {
        Kill();
    }
}

void SonicPlayer::Kill() {
//can't kill if already dead
    if (state == SonicState::Dead) {
        return;  //silently ignore - this is expected when falling after death
    }
    
    SetState(SonicState::Dead);
    velX = 0;
    groundSpeed = 0;
    velY = -7.0f;
    isOnGround = false;
    LoseLife();
    std::cout << "=== KILLED! Lives remaining: " << lives << " ===" << std::endl;
}

void SonicPlayer::Die() {
    Kill();
}

void SonicPlayer::CollectInvincibility(uint64_t duration) {
    isInvincible = true;
    hasPowerUpInvincibility = true;  //power-up invincibility can kill enemies
    inPostDamageInvincibility = false;  //power-up overrides post-damage
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
    subPixelX = subPixelY = 0;
    state = SonicState::Idle;
    facing = FacingDirection::Right;
    isOnGround = false;
    hasShield = false;
    hasSpeedBoost = false;
    rings = 0;

//give 2 seconds of invincibility after respawn with visual effect
    isInvincible = true;
    inPostDamageInvincibility = true;
    invincibleEndTime = engine::GetSystemTime() + 2000;  //2 seconds of invincibility
    hasPowerUpInvincibility = false;

    std::cout << "Respawned at (" << x << ", " << y << ") with 2s invincibility" << std::endl;
}

void SonicPlayer::Reset() {
//full reset for level restart
    rings = 0;
    lives = 3;
    score = 0;
    velX = velY = groundSpeed = 0;
    subPixelX = subPixelY = 0;  //clear sub-pixel accumulators
    state = SonicState::Idle;
    facing = FacingDirection::Right;
    isOnGround = false;
    isInvincible = true;
    hasPowerUpInvincibility = false;
    inPostDamageInvincibility = true;
    hasShield = false;
    hasSpeedBoost = false;
    invincibleEndTime = engine::GetSystemTime() + 2000;  //2 seconds of invincibility after checkpoint teleport with visual effect
    shieldEndTime = 0;
    speedBoostEndTime = 0;
    hurtEndTime = 0;
    idleStartTime = engine::GetSystemTime();
}

void SonicPlayer::Render(engine::Rect& /*viewWindow*/) {
//this would render the sprite
//for now, just a placeholder - actual rendering done in main.cpp
}

//==================== NEW TUNNEL MODE METHODS ====================

void SonicPlayer::EnterTunnelMode(int tunnelIndex) {
    std::cout << "=== ENTERING TUNNEL MODE ===" << std::endl;
    
//activate tunnel in system
    tunnelSystem.EnterTunnel(tunnelIndex);
    
//set tunnel state
    inTunnelMode = true;
    SetState(SonicState::Tunnel);
    
//zero out physics
    velX = 0;
    velY = 0;
    groundSpeed = 0;
    isOnGround = false;
    
//set tunnel animation (spinning)
    StartAnimation("spin");
}

void SonicPlayer::ExitTunnelMode() {
    std::cout << "=== EXITING TUNNEL MODE ===" << std::endl;
    
//deactivate tunnel system
    tunnelSystem.ExitTunnel();
    inTunnelMode = false;
    
//restore physics - exit as jumping/falling
    isOnGround = false;
    velY = 1.0f;  //small downward velocity
    groundSpeed = 3.0f;  //moderate forward momentum
    
//always exit as jumping - will naturally transition on landing
    SetState(SonicState::Jumping);
}

}  //namespace app
