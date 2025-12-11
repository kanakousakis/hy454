#include "Engine.hpp"
#include "SonicPlayer.hpp"
#include "Ring.hpp"
#include "Enemy.hpp"
#include "GameObjects.hpp"
#include "HUD.hpp"
#include "Parallax.hpp"
#include "TileMapLoader.hpp"
#include "SpriteSheetConfig.hpp"
#include "ResourceManager.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

using namespace engine;
using namespace app;

// Helper function to find asset path - handles running from project root OR bin/ directory
std::string FindAssetPath() {
    std::vector<std::string> possiblePaths = {
        "assets/",           // Running from project root (./bin/SonicGame)
        "../assets/",        // Running from bin/ directory (./SonicGame)
        "./assets/",         // Explicit current directory
        "sonic_engine/assets/"  // Running from parent of project
    };
    
    for (const auto& path : possiblePaths) {
        std::ifstream testFile(path + "sonic_sheet_fixed.png");
        if (testFile.good()) {
            testFile.close();
            return path;
        }
    }
    return "assets/";  // Fallback to default
}

// Global asset path - detected once at startup
std::string g_assetPath;

// Forward declarations
void ResetLevel();
const char* StateToString(SonicState state);

// Screen constants
constexpr Dim SCREEN_WIDTH = 640;
constexpr Dim SCREEN_HEIGHT = 480;

// Game state
enum class GameScreen {
    Title,
    Playing,
    GameOver
};

GameScreen currentScreen = GameScreen::Title;
bool gameRunning = true;
bool gamePaused = false;
bool showGrid = false;
bool showDebug = true;
bool assetsLoaded = false;

// Menu state
int pauseMenuSelection = 0;  // 0=Continue, 1=Restart, 2=Quit
constexpr int PAUSE_MENU_ITEMS = 3;

// Scroll multiplication factor (PDF Step 1 requirement)
// PDF: "default value 0.5"
const float SCROLL_FACTORS[] = {0.5f, 1.0f, 1.5f, 2.0f};
int scrollFactorIndex = 0;  // Default 0.5f per PDF
constexpr int NUM_SCROLL_FACTORS = 4;

// Invincibility sparkle state
struct SparkleEffect {
    float angle = 0.0f;
    float radius = 35.0f;
    int numSparkles = 4;
    uint64_t lastUpdate = 0;
} sparkleEffect;

// Shield flicker state
bool shieldVisible = true;
uint64_t lastShieldFlicker = 0;

// Terrain
TileLayer* actionLayer = nullptr;
GridLayer* gridLayer = nullptr;

// Player
SonicPlayer* sonic = nullptr;

// Game objects
RingManager ringManager;
EnemyManager enemyManager;
AnimalManager animalManager;
SpringManager springManager;
SpikeManager spikeManager;
CheckpointManager checkpointManager;
MonitorManager monitorManager;

// Background
ParallaxManager parallaxManager;

// HUD
HUD gameHUD;

// View window (follows player)
Rect viewWindow = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

// Sprite animation state
int sonicFrame = 0;
uint64_t lastFrameTime = 0;
std::string currentAnim = "sonic_idle";

// Forward declare object placement
void PlaceGameObjects();

// Reset level to initial state (PDF: collectibles and objects should reset)
void ResetLevel() {
    // Reset HUD
    gameHUD.lives = 3;
    gameHUD.score = 0;
    gameHUD.rings = 0;
    gameHUD.StartLevel(GetSystemTime());
    
    // Reset Sonic
    if (sonic) {
        sonic->Respawn(150.0f, 1600.0f);
        sonic->Reset();  // Reset rings, lives, state
    }
    
    // Clear and re-place all game objects
    ringManager.Clear();
    enemyManager.Clear();
    animalManager.Clear();
    springManager.Clear();
    spikeManager.Clear();
    checkpointManager.Clear();
    monitorManager.Clear();
    
    // Re-place all objects
    PlaceGameObjects();
    
    std::cout << "Level reset!" << std::endl;
}

void UpdateViewWindow() {
    if (!actionLayer || !sonic) return;
    
    int targetX = sonic->GetIntX() - SCREEN_WIDTH / 2;
    int targetY = sonic->GetIntY() - SCREEN_HEIGHT / 2;
    
    int maxX = actionLayer->GetPixelWidth() - SCREEN_WIDTH;
    int maxY = actionLayer->GetPixelHeight() - SCREEN_HEIGHT;
    
    if (maxX < 0) maxX = 0;
    if (maxY < 0) maxY = 0;
    
    viewWindow.x = std::max(0, std::min(targetX, maxX));
    viewWindow.y = std::max(0, std::min(targetY, maxY));
    
    actionLayer->SetViewWindow(viewWindow);
}

void HandleInput() {
    auto& input = GetInput();
    input.Poll();
    
    // Title screen input
    if (currentScreen == GameScreen::Title) {
        if (input.IsKeyJustPressed(KeyCode::Enter)) {
            currentScreen = GameScreen::Playing;
            // Start the level timer when transitioning to gameplay
            gameHUD.StartLevel(GetSystemTime());
        }
        if (input.IsKeyJustPressed(KeyCode::Q) || input.IsWindowClosed()) {
            gameRunning = false;
        }
        input.Update();
        return;
    }
    
    // Game Over screen input
    if (currentScreen == GameScreen::GameOver) {
        if (input.IsKeyJustPressed(KeyCode::Enter) || input.IsKeyJustPressed(KeyCode::Space)) {
            // Restart game
            currentScreen = GameScreen::Playing;
            ResetLevel();
        }
        if (input.IsKeyJustPressed(KeyCode::Q) || input.IsWindowClosed()) {
            gameRunning = false;
        }
        input.Update();
        return;
    }
    
    // Normal gameplay input
    if (input.IsKeyJustPressed(KeyCode::Escape)) {
        if (gamePaused) {
            // If paused, ESC resumes
            GetGame().Resume();
            pauseMenuSelection = 0;
        } else {
            GetGame().Pause(GetSystemTime());
        }
    }
    
    // Pause menu navigation
    if (gamePaused) {
        if (input.IsKeyJustPressed(KeyCode::Up) || input.IsKeyJustPressed(KeyCode::W)) {
            pauseMenuSelection--;
            if (pauseMenuSelection < 0) pauseMenuSelection = PAUSE_MENU_ITEMS - 1;
        }
        if (input.IsKeyJustPressed(KeyCode::Down) || input.IsKeyJustPressed(KeyCode::S)) {
            pauseMenuSelection++;
            if (pauseMenuSelection >= PAUSE_MENU_ITEMS) pauseMenuSelection = 0;
        }
        if (input.IsKeyJustPressed(KeyCode::Enter)) {
            switch (pauseMenuSelection) {
                case 0:  // Continue
                    GetGame().Resume();
                    pauseMenuSelection = 0;
                    break;
                case 1:  // Restart
                    GetGame().Resume();
                    pauseMenuSelection = 0;
                    ResetLevel();
                    break;
                case 2:  // Quit
                    gameRunning = false;
                    break;
            }
        }
        input.Update();
        return;
    }
    
    if (input.IsKeyJustPressed(KeyCode::Q) || input.IsWindowClosed()) {
        gameRunning = false;
    }
    if (input.IsKeyJustPressed(KeyCode::G)) {
        showGrid = !showGrid;
        std::cout << "Grid overlay: " << (showGrid ? "ON" : "OFF") << std::endl;
    }
    if (input.IsKeyJustPressed(KeyCode::F1)) {
        showDebug = !showDebug;
        std::cout << "Debug panel: " << (showDebug ? "ON" : "OFF") << std::endl;
    }
    
    // Scroll multiplication factor controls (PDF Step 1)
    // + key: next factor (looped: 2.0 -> 0.5)
    if (input.IsKeyJustPressed(KeyCode::Plus) || input.IsKeyJustPressed(KeyCode::Equals)) {
        scrollFactorIndex = (scrollFactorIndex + 1) % NUM_SCROLL_FACTORS;
        std::cout << "Scroll factor: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
    // - key: previous factor (looped: 0.5 -> 2.0)
    if (input.IsKeyJustPressed(KeyCode::Minus)) {
        scrollFactorIndex = (scrollFactorIndex - 1 + NUM_SCROLL_FACTORS) % NUM_SCROLL_FACTORS;
        std::cout << "Scroll factor: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
    // 0 key: reset to default (0.5)
    if (input.IsKeyJustPressed(KeyCode::Num0)) {
        scrollFactorIndex = 0;  // 0.5f
        std::cout << "Scroll factor reset to: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
    // Home key: scroll to top-left
    if (input.IsKeyJustPressed(KeyCode::Home)) {
        viewWindow.x = 0;
        viewWindow.y = 0;
        std::cout << "View: top-left" << std::endl;
    }
    // End key: scroll to bottom-right
    if (input.IsKeyJustPressed(KeyCode::End)) {
        if (actionLayer) {
            viewWindow.x = actionLayer->GetPixelWidth() - SCREEN_WIDTH;
            viewWindow.y = actionLayer->GetPixelHeight() - SCREEN_HEIGHT;
            if (viewWindow.x < 0) viewWindow.x = 0;
            if (viewWindow.y < 0) viewWindow.y = 0;
        }
        std::cout << "View: bottom-right" << std::endl;
    }
    
    if (!GetGame().IsPaused() && sonic) {
        bool left = input.IsKeyPressed(KeyCode::Left) || input.IsKeyPressed(KeyCode::A);
        bool right = input.IsKeyPressed(KeyCode::Right) || input.IsKeyPressed(KeyCode::D);
        bool down = input.IsKeyPressed(KeyCode::Down) || input.IsKeyPressed(KeyCode::S);
        
        // Up/W always jumps now
        bool jump = input.IsKeyPressed(KeyCode::Space) || input.IsKeyPressed(KeyCode::Up) || 
                    input.IsKeyPressed(KeyCode::W);
        bool jumpPressed = input.IsKeyJustPressed(KeyCode::Space) || 
                           input.IsKeyJustPressed(KeyCode::Up) ||
                           input.IsKeyJustPressed(KeyCode::W);
        
        // Mouse click for look up (only when standing still)
        bool lookUp = input.IsMousePressed(MouseButton::Left);
        
        sonic->HandleInput(left, right, jump, jumpPressed, down, lookUp);
    }
    
    input.Update();
}

void UpdateSonicAnimation() {
    if (!sonic) return;
    
    // Determine which animation to use based on state
    std::string newAnim = "sonic_idle";
    unsigned frameDelay = 100;
    
    SonicState state = sonic->GetState();
    float speed = std::abs(sonic->GetSpeed());
    
    switch (state) {
        case SonicState::Idle:
            newAnim = "sonic_idle";
            frameDelay = 100;
            break;
        case SonicState::Bored:
            newAnim = "sonic_bored";
            frameDelay = 300;
            break;
        case SonicState::LookingUp:
            newAnim = "sonic_lookup";
            frameDelay = 100;
            break;
        case SonicState::Crouching:
            newAnim = "sonic_crouch";
            frameDelay = 80;
            break;
        case SonicState::Spindash:
            // Spindash uses dedicated spindash animation (curled + ball frames)
            newAnim = "sonic_spindash";
            {
                float charge = sonic ? sonic->GetSpindashCharge() : 0.0f;
                // Spin speed increases with charge: 60ms at 0, down to 20ms at max charge
                frameDelay = std::max(20u, static_cast<unsigned>(60 - charge * 4));
            }
            break;
        case SonicState::Walking:
            newAnim = "sonic_walk";
            // Faster animation when moving faster
            frameDelay = std::max(40u, static_cast<unsigned>(120 - speed * 15));
            break;
        case SonicState::Running:
            // Use walk but with faster animation (transitional state)
            newAnim = "sonic_walk";
            frameDelay = std::max(30u, static_cast<unsigned>(60 - speed * 5));
            break;
        case SonicState::FullSpeed:
            newAnim = "sonic_run";
            frameDelay = 40;
            break;
        case SonicState::Jumping:
            newAnim = "sonic_ball";
            // Spin faster at higher speeds
            frameDelay = std::max(25u, static_cast<unsigned>(50 - speed * 3));
            // Debug: verify ball animation
            if (currentAnim != "sonic_ball") {
                std::cout << "Switching to ball animation" << std::endl;
            }
            break;
        case SonicState::Rolling:
            newAnim = "sonic_ball";
            frameDelay = std::max(20u, static_cast<unsigned>(40 - speed * 3));
            break;
        case SonicState::Skidding:
            newAnim = "sonic_skid";
            frameDelay = 80;
            break;
        case SonicState::Balancing:
            newAnim = "sonic_balance";
            frameDelay = 150;
            break;
        case SonicState::Pushing:
            newAnim = "sonic_push";
            frameDelay = 150;
            break;
        case SonicState::Spring:
            // Simple spring animation:
            // Going UP (velY < 0): arms spread (bounced up sprite)
            // Coming DOWN (velY >= 0): ball form (can kill enemies)
            if (sonic->GetVelY() < 0) {
                newAnim = "sonic_spring";  // Arms spread going up
            } else {
                newAnim = "sonic_ball";    // Ball form falling
            }
            frameDelay = 80;
            break;
        case SonicState::Landing:
            // Brief landing pose - use idle for smooth transition
            newAnim = "sonic_idle";
            frameDelay = 50;
            break;
        case SonicState::Hurt:
            newAnim = "sonic_hurt";
            frameDelay = 100;
            break;
        case SonicState::Dead:
            // Use death animation (falling pose with arms up)
            newAnim = "sonic_death";
            frameDelay = 100;
            break;
        case SonicState::PipeCling:
            newAnim = "sonic_pipe";
            frameDelay = 100;
            break;
        case SonicState::Tunnel:
            newAnim = "sonic_tunnel";
            frameDelay = 50;
            break;
        case SonicState::AirGasp:
            newAnim = "sonic_gasp";
            frameDelay = 100;
            break;
        case SonicState::Drowning:
            newAnim = "sonic_drown";
            frameDelay = 100;
            break;
        case SonicState::Invincible:
            // Use current movement animation with sparkle effects
            if (speed > 6.0f) newAnim = "sonic_run";
            else if (speed > 0.5f) newAnim = "sonic_walk";
            else newAnim = "sonic_idle";
            frameDelay = 60;
            break;
        default:
            break;
    }
    
    // Handle animation changes with smooth transitions
    if (newAnim != currentAnim) {
        // For walk<->run transitions, preserve animation phase
        bool wasWalkOrRun = (currentAnim == "sonic_walk" || currentAnim == "sonic_run");
        bool isWalkOrRun = (newAnim == "sonic_walk" || newAnim == "sonic_run");
        
        if (wasWalkOrRun && isWalkOrRun) {
            // Preserve frame progress proportionally
            // Walk has 6 frames, Run has 4 frames
            auto* oldFilm = ResourceManager::Instance().GetFilm(currentAnim);
            auto* newFilm = ResourceManager::Instance().GetFilm(newAnim);
            if (oldFilm && newFilm && oldFilm->GetTotalFrames() > 0) {
                float progress = static_cast<float>(sonicFrame % oldFilm->GetTotalFrames()) / 
                                 static_cast<float>(oldFilm->GetTotalFrames());
                sonicFrame = static_cast<int>(progress * newFilm->GetTotalFrames());
            }
        } else {
            sonicFrame = 0;
        }
        currentAnim = newAnim;
    }
    
    // Advance frame
    uint64_t now = GetSystemTime();
    if (now - lastFrameTime >= frameDelay) {
        sonicFrame++;
        lastFrameTime = now;
    }
}

void Physics() {
    // Only run physics when playing
    if (currentScreen != GameScreen::Playing) return;
    
    uint64_t currentTime = GetSystemTime();
    
    if (gamePaused) return;
    
    if (sonic && !GetGame().IsPaused()) {
        sonic->Update();
        UpdateSonicAnimation();
        
        // Update HUD
        gameHUD.Update(currentTime);
        gameHUD.rings = sonic->GetRings();
        gameHUD.lives = sonic->GetLives();
        gameHUD.score = sonic->GetScore();
        
        // Update ring manager
        ringManager.Update();
        
        Rect playerBox = sonic->GetBoundingBox();
        int collected = ringManager.CheckCollision(playerBox);
        if (collected > 0) {
            sonic->AddRings(collected);
        }
        
        // Update enemies
        enemyManager.Update(sonic->GetX(), sonic->GetY());
        
        // Update animals (freed from enemies)
        animalManager.Update();
        
        bool inBall = sonic->IsBallState();
        bool canKillOnContact = sonic->HasPowerUpInvincibility();  // Power-up kills on contact
        bool damageImmune = sonic->IsInvincible();  // Any invincibility prevents damage
        bool inPostDamage = sonic->IsInPostDamageInvincibility();  // Post-damage can't kill
        
        // During post-damage invincibility, ball state should NOT kill enemies
        // Only power-up invincibility allows killing on contact
        bool canKillInBall = inBall && !inPostDamage;
        
        int result = enemyManager.CheckCollision(playerBox, canKillInBall, canKillOnContact, damageImmune);
        
        if (result > 0) {
            sonic->AddScore(result);
            gameHUD.AddScore(result, sonic->GetX(), sonic->GetY());
        } else if (result < 0) {
            sonic->TakeDamage();
        }
        
        // Update and check springs
        springManager.Update(currentTime);
        float bounceVelX = 0, bounceVelY = 0;
        if (springManager.CheckCollision(playerBox, bounceVelX, bounceVelY, currentTime)) {
            sonic->ApplyBounce(bounceVelX, bounceVelY);
            std::cout << "Spring bounce!" << std::endl;
        }
        
        // Check spikes (only damage if not invincible)
        if (!sonic->IsInvincible()) {
            float sonicSpeed = sonic->GetSpeed();
            float sonicVelY = sonic->GetVelY();
            if (spikeManager.CheckCollision(playerBox, sonicSpeed, sonicVelY)) {
                sonic->TakeDamage();
                std::cout << "Spike damage! velY=" << sonicVelY << " speed=" << sonicSpeed << std::endl;
            }
        }
        
        // Update and check checkpoints
        checkpointManager.Update(currentTime);
        if (checkpointManager.CheckCollision(playerBox, currentTime)) {
            std::cout << "Checkpoint activated!" << std::endl;
            // Could play sound here
        }
        
        // Update and check monitors
        monitorManager.Update(currentTime);
        float sonicVelY = sonic->GetVelY();
        int monitorResult = monitorManager.CheckCollision(playerBox, inBall, sonicVelY, currentTime);
        if (monitorResult >= 0) {
            MonitorType type = static_cast<MonitorType>(monitorResult);
            switch (type) {
                case MonitorType::Ring:
                    sonic->AddRings(10);
                    std::cout << "+10 Rings!" << std::endl;
                    break;
                case MonitorType::Shield:
                    sonic->GiveShield(30000);  // 30 seconds
                    std::cout << "Shield!" << std::endl;
                    break;
                case MonitorType::Invincibility:
                    sonic->GiveInvincibility(20000);  // 20 seconds
                    std::cout << "Invincibility!" << std::endl;
                    break;
                case MonitorType::SpeedShoes:
                    sonic->GiveSpeedBoost(10000);  // 10 seconds
                    std::cout << "Speed Shoes!" << std::endl;
                    break;
                case MonitorType::ExtraLife:
                    sonic->AddLife();
                    gameHUD.GainLife();
                    std::cout << "Extra Life!" << std::endl;
                    break;
                case MonitorType::Eggman:
                    sonic->TakeDamage();
                    std::cout << "Eggman trap!" << std::endl;
                    break;
            }
            gameHUD.AddScore(100, sonic->GetX(), sonic->GetY());
        }
        
        // Check for death (falling off screen / into pit)
        // Terrain is 15 tiles * 128px = 1920px total height
        // Ground is at row 13, so ground level ~1664px
        // Death should trigger when falling below the map
        const float PIT_DEATH_Y = 1900.0f;  // Below ground level
        if (sonic->GetY() > PIT_DEATH_Y && sonic->GetState() != SonicState::Dead) {
            std::cout << "PIT DEATH! Y=" << sonic->GetY() << " Lives=" << sonic->GetLives() << std::endl;
            sonic->Die();  // This decreases lives
        }
        
        // Handle death state - wait for animation then respawn or game over
        static uint64_t deathStartTime = 0;
        if (sonic->GetState() == SonicState::Dead && currentScreen == GameScreen::Playing) {
            // Track when death started
            if (deathStartTime == 0) {
                deathStartTime = GetSystemTime();
            }
            
            // Wait 2 seconds for death animation, or until Sonic falls off screen
            uint64_t deathDuration = GetSystemTime() - deathStartTime;
            bool timeExpired = deathDuration > 2000;  // 2 second death animation
            bool fellOffScreen = sonic->GetY() > 2100.0f;
            
            if (timeExpired || fellOffScreen) {
                deathStartTime = 0;  // Reset for next death
                
                if (sonic->GetLives() <= 0) {
                    std::cout << "GAME OVER - No lives left!" << std::endl;
                    currentScreen = GameScreen::GameOver;
                } else {
                    // Respawn at checkpoint
                    float respawnX, respawnY;
                    checkpointManager.GetRespawnPosition(respawnX, respawnY);
                    std::cout << "Respawning at (" << respawnX << ", " << respawnY << ") - Lives: " << sonic->GetLives() << std::endl;
                    sonic->Respawn(respawnX, respawnY);
                    gameHUD.rings = 0;  // Reset rings on death
                }
            }
        } else {
            deathStartTime = 0;  // Reset if not dead
        }
    }
}

const char* StateToString(SonicState state) {
    switch (state) {
        case SonicState::Idle:       return "IDLE";
        case SonicState::Bored:      return "BORED";
        case SonicState::LookingUp:  return "LOOKUP";
        case SonicState::Crouching:  return "CROUCH";
        case SonicState::Spindash:   return "SPINDASH";
        case SonicState::Walking:    return "WALK";
        case SonicState::Running:    return "RUN";
        case SonicState::FullSpeed:  return "FULL";
        case SonicState::Jumping:    return "JUMP";
        case SonicState::Rolling:    return "ROLL";
        case SonicState::Skidding:   return "SKID";
        case SonicState::Balancing:  return "BALANCE";
        case SonicState::Pushing:    return "PUSH";
        case SonicState::Spring:     return "SPRING";
        case SonicState::Landing:    return "LANDING";
        case SonicState::Hurt:       return "HURT";
        case SonicState::Dead:       return "DEAD";
        case SonicState::Invincible: return "INVINCIBLE";
        case SonicState::PipeCling:  return "PIPE";
        case SonicState::Tunnel:     return "TUNNEL";
        case SonicState::AirGasp:    return "GASP";
        case SonicState::Drowning:   return "DROWN";
        default: return "???";
    }
}

void DrawGridOverlay() {
    if (!showGrid || !gridLayer) return;
    
    auto& gfx = GetGraphics();
    
    int startCol = viewWindow.x / GRID_ELEMENT_WIDTH;
    int startRow = viewWindow.y / GRID_ELEMENT_HEIGHT;
    int endCol = (viewWindow.x + viewWindow.w) / GRID_ELEMENT_WIDTH + 1;
    int endRow = (viewWindow.y + viewWindow.h) / GRID_ELEMENT_HEIGHT + 1;
    
    endCol = std::min(endCol, static_cast<int>(gridLayer->GetCols()));
    endRow = std::min(endRow, static_cast<int>(gridLayer->GetRows()));
    startCol = std::max(0, startCol);
    startRow = std::max(0, startRow);
    
    for (int row = startRow; row < endRow; ++row) {
        for (int col = startCol; col < endCol; ++col) {
            GridIndex tile = gridLayer->GetTile(static_cast<Dim>(col), static_cast<Dim>(row));
            if (tile != GRID_EMPTY) {
                int screenX = col * GRID_ELEMENT_WIDTH - viewWindow.x;
                int screenY = row * GRID_ELEMENT_HEIGHT - viewWindow.y;
                
                Color c;
                if (tile == GRID_SOLID) {
                    c = MakeColor(101, 67, 33, 200);
                } else if (tile & GRID_TOP_SOLID) {
                    c = MakeColor(34, 139, 34, 160);
                } else {
                    c = MakeColor(255, 255, 0, 100);
                }
                
                gfx.DrawRect({screenX, screenY, GRID_ELEMENT_WIDTH, GRID_ELEMENT_HEIGHT}, c, true);
            }
        }
    }
}

void DrawSonicSprite() {
    if (!sonic) return;
    
    auto& gfx = GetGraphics();
    Rect box = sonic->GetBoundingBox();
    int screenX = box.x - viewWindow.x;
    int screenY = box.y - viewWindow.y;
    
    // Skip if completely off screen
    if (screenX + box.w < 0 || screenX > SCREEN_WIDTH ||
        screenY + box.h < 0 || screenY > SCREEN_HEIGHT) {
        return;
    }
    
    // Draw collision box in debug mode (magenta outline only)
    if (showDebug) {
        gfx.DrawRect({screenX, screenY, box.w, box.h}, MakeColor(255, 0, 255), false);
    }
    
    // Try to draw with loaded sprite
    if (assetsLoaded) {
        auto* film = ResourceManager::Instance().GetFilm(currentAnim);
        if (film && film->GetTotalFrames() > 0) {
            int frameIdx = sonicFrame % film->GetTotalFrames();
            Rect frameBox = film->GetFrameBox(static_cast<byte>(frameIdx));
            
            // Center sprite horizontally on collision box, align bottom
            int drawX = screenX + (box.w - frameBox.w) / 2;
            int drawY = screenY + box.h - frameBox.h;
            
            // Flip sprite based on facing direction (like Dami-Karv)
            bool flipX = (sonic->GetFacing() == FacingDirection::Left);
            
            if (flipX) {
                film->DisplayFrameFlipped({drawX, drawY}, static_cast<byte>(frameIdx));
            } else {
                film->DisplayFrame({drawX, drawY}, static_cast<byte>(frameIdx));
            }
            return;  // Successfully drew sprite
        }
    }
    
    // Simple fallback - just draw a blue rectangle if sprites fail
    gfx.DrawRect({screenX + 2, screenY + 2, box.w - 4, box.h - 4}, MakeColor(0, 80, 200), true);
    gfx.DrawRect({screenX + 2, screenY + 2, box.w - 4, box.h - 4}, MakeColor(0, 40, 160), false);
}

void DrawSprings() {
    auto& gfx = GetGraphics();
    
    // Get spring films if loaded
    auto* yellowSpringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("spring_yellow") : nullptr;
    auto* redSpringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("spring_red") : nullptr;
    
    for (const auto& spring : springManager.GetSprings()) {
        int screenX = static_cast<int>(spring.x) - viewWindow.x;
        int screenY = static_cast<int>(spring.y) - viewWindow.y;
        
        // Skip if off-screen
        if (screenX + spring.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + spring.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        // Choose film based on spring type
        auto* springFilm = spring.isYellow ? yellowSpringFilm : redSpringFilm;
        int frameIdx = spring.isCompressed ? 1 : 0;  // 0=normal, 1=compressed
        
        if (springFilm && springFilm->GetTotalFrames() > 0) {
            springFilm->DisplayFrame({screenX, screenY}, static_cast<byte>(frameIdx % springFilm->GetTotalFrames()));
        } else {
            // Fallback placeholder
            Color springColor = spring.isYellow ? MakeColor(255, 215, 0) : MakeColor(255, 50, 50);
            int currentHeight = spring.GetCurrentHeight();
            gfx.DrawRect({screenX, screenY + spring.height - 8, spring.width, 8}, 
                         MakeColor(100, 100, 100), true);
            int coilTop = screenY + spring.height - currentHeight;
            gfx.DrawRect({screenX + 4, coilTop, spring.width - 8, currentHeight - 8}, 
                         springColor, true);
            gfx.DrawRect({screenX, coilTop - 4, spring.width, 6}, springColor, true);
        }
    }
}

void DrawSpikes() {
    auto& gfx = GetGraphics();
    
    for (const auto& spike : spikeManager.GetSpikes()) {
        int screenX = static_cast<int>(spike.x) - viewWindow.x;
        int screenY = static_cast<int>(spike.y) - viewWindow.y;
        
        if (screenX + spike.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + spike.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        // Get spike sprite frame from misc sheet
        if (assetsLoaded) {
            auto& rm = ResourceManager::Instance();
            auto miscBmp = rm.GetMiscSheet();
            if (miscBmp) {
                auto frame = MiscSpriteConfig::GetSpikesUp();
                gfx.DrawTexture(miscBmp->GetTexture(), 
                               {frame.x, frame.y, frame.w, frame.h},
                               {screenX, screenY});
            }
        }
        
        // Fallback placeholder (always draw for now since sprite may not work)
        Color spikeColor = MakeColor(180, 180, 180);
        Color baseColor = MakeColor(100, 100, 100);
        gfx.DrawRect({screenX, screenY + spike.height - 6, spike.width, 6}, baseColor, true);
        int spikeCount = 4;
        int spikeWidth = spike.width / spikeCount;
        for (int i = 0; i < spikeCount; ++i) {
            int sx = screenX + i * spikeWidth + spikeWidth/4;
            int sy = screenY;
            gfx.DrawRect({sx, sy, spikeWidth/2, spike.height - 6}, spikeColor, true);
        }
        
        // Debug: draw hitbox when grid is shown
        if (showGrid) {
            Rect hitbox = spike.GetBoundingBox();
            int hx = hitbox.x - viewWindow.x;
            int hy = hitbox.y - viewWindow.y;
            gfx.DrawRect({hx, hy, hitbox.w, hitbox.h}, MakeColor(255, 0, 0, 128), false);
        }
    }
}

void DrawCheckpoints() {
    auto& gfx = GetGraphics();
    
    // Get checkpoint film if loaded
    auto* checkpointFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("checkpoint") : nullptr;
    
    for (const auto& cp : checkpointManager.GetCheckpoints()) {
        int screenX = static_cast<int>(cp.x) - viewWindow.x;
        int screenY = static_cast<int>(cp.y) - viewWindow.y;
        
        if (screenX + cp.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + cp.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        if (checkpointFilm && checkpointFilm->GetTotalFrames() > 0) {
            // Use animated frames based on state
            int frameIdx = cp.activated ? (cp.animFrame % checkpointFilm->GetTotalFrames()) : 0;
            checkpointFilm->DisplayFrame({screenX, screenY - 32}, static_cast<byte>(frameIdx));
        } else {
            // Fallback placeholder
            Color poleColor = MakeColor(100, 100, 100);
            gfx.DrawRect({screenX + cp.width/2 - 3, screenY, 6, cp.height}, poleColor, true);
            
            Color ballColor = cp.activated ? MakeColor(0, 100, 255) : MakeColor(255, 50, 50);
            if (cp.activated) {
                int flash = (cp.animFrame % 2 == 0) ? 50 : 0;
                ballColor = MakeColor(0, 100 + flash, 255);
            }
            
            int ballSize = 16;
            gfx.DrawRect({screenX + cp.width/2 - ballSize/2, screenY - ballSize/2, ballSize, ballSize}, 
                         ballColor, true);
        }
    }
}

void DrawMonitors() {
    auto& gfx = GetGraphics();
    auto miscBmp = assetsLoaded ? ResourceManager::Instance().GetMiscSheet() : nullptr;
    
    for (const auto& monitor : monitorManager.GetMonitors()) {
        if (monitor.state == MonitorState::Destroyed) continue;
        
        int screenX = static_cast<int>(monitor.x) - viewWindow.x;
        int screenY = static_cast<int>(monitor.y) - viewWindow.y;
        
        if (screenX + monitor.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + monitor.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        if (miscBmp) {
            if (monitor.state == MonitorState::Breaking) {
                // Draw broken box (shifted down by 17px like Dami-Karv)
                auto frame = MiscSpriteConfig::GetMonitorBroken();
                gfx.DrawTexture(miscBmp->GetTexture(),
                               {frame.x, frame.y, frame.w, frame.h},
                               {screenX, screenY + 17});
            } else {
                // Draw unbroken monitor box
                auto boxFrame = MiscSpriteConfig::GetMonitorUnbroken();
                gfx.DrawTexture(miscBmp->GetTexture(),
                               {boxFrame.x, boxFrame.y, boxFrame.w, boxFrame.h},
                               {screenX, screenY});
                
                // Draw icon on top (centered, with flicker)
                if (monitor.animFrame == 0) {
                    FrameRect iconFrame;
                    switch (monitor.type) {
                        case MonitorType::Ring:         iconFrame = MiscSpriteConfig::GetIconRing(); break;
                        case MonitorType::Shield:       iconFrame = MiscSpriteConfig::GetIconShield(); break;
                        case MonitorType::Invincibility: iconFrame = MiscSpriteConfig::GetIconInvincible(); break;
                        case MonitorType::SpeedShoes:   iconFrame = MiscSpriteConfig::GetIconSpeed(); break;
                        case MonitorType::ExtraLife:    iconFrame = MiscSpriteConfig::GetIconLife(); break;
                        case MonitorType::Eggman:       iconFrame = MiscSpriteConfig::GetIconEggman(); break;
                        default: iconFrame = MiscSpriteConfig::GetIconRing(); break;
                    }
                    // Center icon in monitor
                    int iconX = screenX + (monitor.width - iconFrame.w) / 2;
                    int iconY = screenY + (monitor.height - iconFrame.h) / 2 - 2;
                    gfx.DrawTexture(miscBmp->GetTexture(),
                                   {iconFrame.x, iconFrame.y, iconFrame.w, iconFrame.h},
                                   {iconX, iconY});
                }
            }
        } else {
            // Fallback placeholder
            if (monitor.state == MonitorState::Breaking) {
                gfx.DrawRect({screenX - 5, screenY - 5, monitor.width + 10, monitor.height + 10}, 
                             MakeColor(255, 200, 100), true);
            } else {
                gfx.DrawRect({screenX, screenY, monitor.width, monitor.height}, 
                             MakeColor(80, 80, 80), true);
                gfx.DrawRect({screenX + 3, screenY + 3, monitor.width - 6, monitor.height - 10}, 
                             MakeColor(50, 80, 120), true);
                if (monitor.animFrame == 0) {
                    Color iconColor = monitor.GetIconColor();
                    int iconSize = 12;
                    int iconX = screenX + monitor.width/2 - iconSize/2;
                    int iconY = screenY + monitor.height/2 - iconSize/2 - 2;
                    gfx.DrawRect({iconX, iconY, iconSize, iconSize}, iconColor, true);
                }
            }
        }
        
        // Power-up popup
        if (monitor.showPopup) {
            int popupX = screenX + monitor.width/2 - 8;
            int popupY = static_cast<int>(monitor.popupY) - viewWindow.y;
            Color iconColor = monitor.GetIconColor();
            gfx.DrawRect({popupX, popupY, 16, 16}, iconColor, true);
            gfx.DrawRect({popupX, popupY, 16, 16}, MakeColor(255, 255, 255), false);
        }
    }
}

void DrawHUD() {
    auto& gfx = GetGraphics();
    
    gfx.DrawRect({5, 5, 200, 100}, MakeColor(0, 0, 0, 200), true);
    gfx.DrawRect({5, 5, 200, 100}, MakeColor(255, 255, 255, 100), false);
    
    // Ring icon and count bar
    gfx.DrawRect({15, 15, 16, 16}, MakeColor(255, 215, 0), true);
    gfx.DrawRect({19, 19, 8, 8}, MakeColor(0, 0, 0, 200), true);
    gfx.DrawRect({15, 15, 16, 16}, MakeColor(200, 150, 0), false);
    
    if (sonic) {
        int rings = sonic->GetRings();
        int barWidth = std::min(rings * 3, 150);
        gfx.DrawRect({40, 18, barWidth, 10}, MakeColor(255, 215, 0), true);
        gfx.DrawRect({40, 18, 150, 10}, MakeColor(100, 100, 100), false);
    }
    
    // Life icon
    gfx.DrawRect({15, 40, 16, 16}, MakeColor(0, 100, 255), true);
    gfx.DrawRect({15, 40, 16, 16}, MakeColor(0, 0, 0), false);
    
    if (sonic) {
        int lives = sonic->GetLives();
        for (int i = 0; i < lives && i < 5; ++i) {
            gfx.DrawRect({40 + i * 20, 43, 12, 12}, MakeColor(0, 150, 255), true);
        }
    }
    
    // Score bar
    gfx.DrawRect({15, 65, 16, 16}, MakeColor(255, 255, 255), true);
    gfx.DrawRect({15, 65, 16, 16}, MakeColor(200, 200, 200), false);
    
    if (sonic) {
        int score = sonic->GetScore();
        int scoreWidth = std::min(score / 10, 150);
        gfx.DrawRect({40, 68, scoreWidth, 10}, MakeColor(255, 255, 255), true);
    }
    
    // Status indicators
    if (sonic) {
        Color groundColor = sonic->IsOnGround() ? MakeColor(0, 255, 0) : MakeColor(255, 0, 0);
        gfx.DrawRect({15, 88, 12, 12}, groundColor, true);
        
        if (sonic->IsInvincible()) {
            gfx.DrawRect({35, 88, 12, 12}, MakeColor(255, 255, 0), true);
        }
        if (sonic->HasShield()) {
            gfx.DrawRect({55, 88, 12, 12}, MakeColor(100, 200, 255), true);
        }
    }
}

void DrawDebugInfo() {
    if (!showDebug || !sonic) return;
    
    auto& gfx = GetGraphics();
    
    gfx.DrawRect({SCREEN_WIDTH - 200, 5, 195, 120}, MakeColor(0, 0, 0, 200), true);
    
    // State text
    const char* stateStr = "UNKNOWN";
    SonicState state = sonic->GetState();
    switch (state) {
        case SonicState::Idle: stateStr = "IDLE"; break;
        case SonicState::Walking: stateStr = "WALKING"; break;
        case SonicState::Running: stateStr = "RUNNING"; break;
        case SonicState::FullSpeed: stateStr = "FULLSPEED"; break;
        case SonicState::Jumping: stateStr = "JUMPING"; break;
        case SonicState::Rolling: stateStr = "ROLLING"; break;
        case SonicState::Hurt: stateStr = "HURT"; break;
        case SonicState::Dead: stateStr = "DEAD"; break;
        default: break;
    }
    gfx.DrawText(stateStr, SCREEN_WIDTH - 190, 10, MakeColor(255, 255, 0));
    
    // Position
    char posStr[64];
    snprintf(posStr, sizeof(posStr), "X:%.0f Y:%.0f", sonic->GetX(), sonic->GetY());
    gfx.DrawText(posStr, SCREEN_WIDTH - 190, 30, MakeColor(200, 200, 200));
    
    // Velocity
    char velStr[64];
    snprintf(velStr, sizeof(velStr), "VX:%.1f VY:%.1f", sonic->GetVelX(), sonic->GetVelY());
    gfx.DrawText(velStr, SCREEN_WIDTH - 190, 50, MakeColor(200, 200, 200));
    
    // Ground speed
    char spdStr[64];
    snprintf(spdStr, sizeof(spdStr), "Speed:%.2f", sonic->GetSpeed());
    gfx.DrawText(spdStr, SCREEN_WIDTH - 190, 70, MakeColor(0, 255, 0));
    
    // Grid toggle hint
    gfx.DrawText("G=Grid F1=Debug", SCREEN_WIDTH - 190, 95, MakeColor(150, 150, 150));
}

void RenderTitleScreen() {
    auto& gfx = GetGraphics();
    
    // Blue gradient background (Sonic sky)
    gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 100, 200), true);
    gfx.DrawRect({0, 0, SCREEN_WIDTH, 100}, MakeColor(0, 60, 160), true);
    
    // Title box
    int boxX = SCREEN_WIDTH/2 - 250;
    int boxY = 40;
    int boxW = 500;
    int boxH = 120;
    
    gfx.DrawRect({boxX, boxY, boxW, boxH}, MakeColor(0, 0, 80, 230), true);
    gfx.DrawRect({boxX, boxY, boxW, boxH}, MakeColor(255, 200, 0), false);
    gfx.DrawRect({boxX + 2, boxY + 2, boxW - 4, boxH - 4}, MakeColor(255, 200, 0), false);
    
    // "SONIC THE HEDGEHOG" title
    gfx.DrawText("SONIC THE HEDGEHOG", boxX + 120, boxY + 30, MakeColor(255, 215, 0));
    
    // "THE CSD CHRONICLES" subtitle  
    gfx.DrawText("The CSD Chronicles", boxX + 160, boxY + 70, MakeColor(200, 200, 255));
    
    // University info box
    int infoX = SCREEN_WIDTH/2 - 280;
    int infoY = 180;
    int infoW = 560;
    int infoH = 160;
    
    gfx.DrawRect({infoX, infoY, infoW, infoH}, MakeColor(0, 0, 0, 200), true);
    gfx.DrawRect({infoX, infoY, infoW, infoH}, MakeColor(100, 100, 100), false);
    
    // University text - exactly as PDF requires (Page 28)
    gfx.DrawText("University of Crete", infoX + 190, infoY + 15, MakeColor(255, 255, 255));
    gfx.DrawText("Department of Computer Science", infoX + 130, infoY + 40, MakeColor(255, 255, 255));
    gfx.DrawText("CS-454. Development of Intelligent Interfaces and Games", 
                 infoX + 50, infoY + 70, MakeColor(200, 200, 200));
    gfx.DrawText("Term Project, Fall Semester 2024", infoX + 140, infoY + 100, MakeColor(200, 200, 200));
    
    // Team names - YOU SHOULD UPDATE THIS WITH YOUR ACTUAL NAMES
    gfx.DrawText("TEAM: [YOUR NAME HERE]", infoX + 170, infoY + 130, MakeColor(255, 255, 0));
    
    // Sonic icon (blue rectangle placeholder)
    gfx.DrawRect({SCREEN_WIDTH/2 - 30, 360, 60, 80}, MakeColor(0, 100, 255), true);
    gfx.DrawRect({SCREEN_WIDTH/2 - 25, 370, 50, 30}, MakeColor(255, 200, 150), true); // Face
    gfx.DrawRect({SCREEN_WIDTH/2 - 15, 355, 30, 20}, MakeColor(0, 50, 200), true); // Spikes
    
    // "Press SPACE to Start" prompt
    int promptY = 450;
    static uint64_t lastBlink = 0;
    static bool showPrompt = true;
    uint64_t now = GetSystemTime();
    if (now - lastBlink > 500) {
        showPrompt = !showPrompt;
        lastBlink = now;
    }
    
    if (showPrompt) {
        gfx.DrawText("PRESS ENTER TO START", SCREEN_WIDTH/2 - 90, promptY, MakeColor(255, 255, 0));
    }
    
    gfx.Present();
}

void Render() {
    // Handle different screens
    if (currentScreen == GameScreen::Title) {
        RenderTitleScreen();
        return;
    }
    
    auto& gfx = GetGraphics();
    
    // Draw parallax background
    parallaxManager.Draw(gfx, viewWindow.x, viewWindow.y, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Draw terrain using tileset if available
    if (actionLayer && actionLayer->GetTileSet()) {
        actionLayer->SetViewWindow(viewWindow);
        actionLayer->Display({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});
    }
    
    // Draw grid overlay ON TOP of tiles when enabled (G key)
    if (showGrid) {
        DrawGridOverlay();
    }
    
    // Draw game objects
    DrawSprings();
    DrawSpikes();
    DrawCheckpoints();
    DrawMonitors();
    
    // Draw rings and enemies
    auto* ringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("ring") : nullptr;
    auto* ringCollectFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("ring_collect") : nullptr;
    ringManager.Render(viewWindow, ringFilm, ringCollectFilm);
    enemyManager.Render(viewWindow);
    animalManager.Render(viewWindow);
    
    // Draw player
    DrawSonicSprite();
    
    // Pause overlay
    if (gamePaused) {
        gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 0, 0, 128), true);
        
        // Pause menu box
        int menuX = SCREEN_WIDTH/2 - 120;
        int menuY = SCREEN_HEIGHT/2 - 100;
        int menuW = 240;
        int menuH = 200;
        
        gfx.DrawRect({menuX, menuY, menuW, menuH}, MakeColor(0, 0, 80, 240), true);
        gfx.DrawRect({menuX, menuY, menuW, menuH}, MakeColor(255, 215, 0), false);
        gfx.DrawRect({menuX + 2, menuY + 2, menuW - 4, menuH - 4}, MakeColor(255, 215, 0), false);
        
        // "PAUSED" title
        gfx.DrawText("PAUSED", menuX + 85, menuY + 20, MakeColor(255, 215, 0));
        
        // Menu options with selection highlight
        const char* menuItems[] = {"CONTINUE", "RESTART", "QUIT"};
        int itemY = menuY + 60;
        
        for (int i = 0; i < PAUSE_MENU_ITEMS; ++i) {
            Color bgColor = (i == pauseMenuSelection) ? MakeColor(255, 215, 0) : MakeColor(60, 60, 100);
            Color textColor = (i == pauseMenuSelection) ? MakeColor(0, 0, 80) : MakeColor(255, 255, 255);
            
            // Draw option background
            gfx.DrawRect({menuX + 40, itemY, 160, 28}, bgColor, true);
            gfx.DrawRect({menuX + 40, itemY, 160, 28}, MakeColor(255, 215, 0), false);
            
            // Draw selection arrow
            if (i == pauseMenuSelection) {
                gfx.DrawText(">", menuX + 20, itemY + 5, MakeColor(255, 255, 0));
            }
            
            // Draw option text
            int textOffset = (i == 0) ? 45 : (i == 1) ? 50 : 65;
            gfx.DrawText(menuItems[i], menuX + textOffset, itemY + 5, textColor);
            
            itemY += 38;
        }
        
        // Instructions
        gfx.DrawText("UP/DOWN: Select  ENTER: Confirm", menuX + 15, menuY + 175, MakeColor(150, 150, 150));
    }
    
    // Game Over screen - also set state
    if (gameHUD.IsGameOver()) {
        currentScreen = GameScreen::GameOver;
        
        gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 0, 0, 200), true);
        
        int boxX = SCREEN_WIDTH/2 - 150;
        int boxY = SCREEN_HEIGHT/2 - 120;
        int boxW = 300;
        int boxH = 240;
        
        // Red "GAME OVER" box
        gfx.DrawRect({boxX, boxY, boxW, boxH}, MakeColor(100, 0, 0, 240), true);
        gfx.DrawRect({boxX, boxY, boxW, boxH}, MakeColor(255, 0, 0), false);
        gfx.DrawRect({boxX + 3, boxY + 3, boxW - 6, boxH - 6}, MakeColor(255, 0, 0), false);
        
        // "GAME OVER" title
        gfx.DrawText("GAME OVER", boxX + 95, boxY + 30, MakeColor(255, 100, 100));
        
        // Final score display
        char scoreStr[32];
        snprintf(scoreStr, sizeof(scoreStr), "%d", gameHUD.score);
        gfx.DrawText("FINAL SCORE:", boxX + 50, boxY + 80, MakeColor(255, 255, 0));
        gfx.DrawText(scoreStr, boxX + 180, boxY + 80, MakeColor(255, 255, 255));
        
        // Time display
        int minutes = gameHUD.timeSeconds / 60;
        int seconds = gameHUD.timeSeconds % 60;
        char timeStr[32];
        snprintf(timeStr, sizeof(timeStr), "%d:%02d", minutes, seconds);
        gfx.DrawText("TIME:", boxX + 50, boxY + 110, MakeColor(255, 215, 0));
        gfx.DrawText(timeStr, boxX + 180, boxY + 110, MakeColor(255, 255, 255));
        
        // Prompt to continue
        static uint64_t lastBlink = 0;
        static bool showPrompt = true;
        uint64_t now = GetSystemTime();
        if (now - lastBlink > 500) {
            showPrompt = !showPrompt;
            lastBlink = now;
        }
        if (showPrompt) {
            gfx.DrawText("PRESS SPACE TO RESTART", boxX + 45, boxY + 160, MakeColor(255, 255, 0));
        }
        
        gfx.DrawText("PRESS Q TO QUIT", boxX + 85, boxY + 195, MakeColor(150, 150, 150));
    }
    
    // Draw HUD
    gameHUD.Draw(gfx, viewWindow.x, viewWindow.y);
    DrawDebugInfo();
    
    gfx.Present();
}

// Load collision grid from CSV and expand to grid elements
bool LoadGridFromCSV(const std::string& path, GridLayer*& grid, int tilesX, int tilesY) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    // The CSV is at tile level (1 cell per tile)
    // We need to expand each cell to GRID_BLOCK_ROWS x GRID_BLOCK_COLUMNS grid elements
    Dim gridRows = static_cast<Dim>(tilesY * GRID_BLOCK_ROWS);
    Dim gridCols = static_cast<Dim>(tilesX * GRID_BLOCK_COLUMNS);
    
    grid = new GridLayer(gridRows, gridCols);
    
    std::string line;
    int tileRow = 0;
    
    while (std::getline(file, line) && tileRow < tilesY) {
        std::stringstream ss(line);
        std::string cell;
        int tileCol = 0;
        
        while (std::getline(ss, cell, ',') && tileCol < tilesX) {
            // Trim whitespace
            size_t start = cell.find_first_not_of(" \t\r\n");
            size_t end = cell.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                cell = cell.substr(start, end - start + 1);
            }
            
            if (!cell.empty()) {
                int val = std::stoi(cell);
                // CSV format: 0=solid, 1=platform (top solid), 2=empty
                GridIndex gridVal = GRID_EMPTY;
                if (val == 0) gridVal = GRID_SOLID;
                else if (val == 1) gridVal = GRID_TOP_SOLID;
                // else: 2 = empty (default)
                
                // Expand this tile cell to GRID_BLOCK_ROWS x GRID_BLOCK_COLUMNS grid elements
                int baseRow = tileRow * GRID_BLOCK_ROWS;
                int baseCol = tileCol * GRID_BLOCK_COLUMNS;
                
                for (int gr = 0; gr < GRID_BLOCK_ROWS; ++gr) {
                    for (int gc = 0; gc < GRID_BLOCK_COLUMNS; ++gc) {
                        grid->SetTile(
                            static_cast<Dim>(baseCol + gc),
                            static_cast<Dim>(baseRow + gr),
                            gridVal
                        );
                    }
                }
            }
            tileCol++;
        }
        tileRow++;
    }
    
    std::cout << "Loaded grid from CSV: " << path << " (" << gridCols << "x" << gridRows << " elements)" << std::endl;
    return true;
}

// Places all game objects - called on level start and reset
void PlaceGameObjects() {
    // Add rings - ground is at y=1664, rings float above
    for (int i = 0; i < 10; ++i) {
        ringManager.AddRing(300.0f + i * 40, 1630.0f);  // Just above ground
    }
    for (int i = 0; i < 5; ++i) {
        ringManager.AddRing(800.0f + i * 35, 1200.0f);  // On platform
    }
    for (int i = 0; i < 5; ++i) {
        ringManager.AddRing(2600.0f + i * 35, 1200.0f);
    }
    for (int i = 0; i < 3; ++i) {
        ringManager.AddRing(1600.0f + i * 40, 900.0f);
    }
    ringManager.AddRing(2850.0f, 1630.0f);
    ringManager.AddRing(2900.0f, 1630.0f);
    
    // ================================================================
    // PDF ENEMIES - The four required enemies from the PDF document:
    // 1. Masher (Piranha) - jumps up/down, max 64px height
    // 2. Crabmeat - moves left/right, shoots curved projectiles
    // 3. Buzz Bomber - flies toward Sonic, shoots at feet, resets
    // 4. Motobug/Motora - moves left/right, pauses at obstacles
    // ================================================================

    // Ground surface is at y=1664
    // Enemies stand ON ground, so position = 1664 - enemy_height (~1632)

    // === MOTOBUG/MOTORA (PDF Enemy #4) ===
    // "Goes left and right, stops for a while and turns at obstacle/cliff"
    enemyManager.AddMotobug(350.0f, 1632.0f);     // Near spawn - easy to test
    enemyManager.AddMotobug(600.0f, 1632.0f);
    enemyManager.AddMotobug(1800.0f, 1632.0f);
    enemyManager.AddMotobug(4000.0f, 1632.0f);

    // === CRABMEAT (PDF Enemy #2) ===
    // "Moves left/right, throws two curved projectiles when Sonic is close"
    enemyManager.AddCrabmeat(450.0f, 1632.0f);    // Near spawn - test projectiles
    enemyManager.AddCrabmeat(1200.0f, 1632.0f);
    enemyManager.AddCrabmeat(3500.0f, 1632.0f);

    // === BUZZ BOMBER (PDF Enemy #3) ===
    // "Flies toward Sonic, fires at feet, flies off screen then flips, resets if Sonic skips"
    enemyManager.AddBuzzBomber(500.0f, 1500.0f, 300.0f);   // Near spawn for testing
    enemyManager.AddBuzzBomber(700.0f, 1500.0f, 300.0f);
    enemyManager.AddBuzzBomber(2000.0f, 1400.0f, 250.0f);
    enemyManager.AddBuzzBomber(3800.0f, 1450.0f, 200.0f);

    // === MASHER (PDF Enemy #1) ===
    // "Piranhas go up/down, max 64px above bridge, lowest just beneath screen"
    enemyManager.AddMasher(550.0f, 1700.0f, 1500);  // Near spawn for easy testing
    enemyManager.AddMasher(2900.0f, 1700.0f, 1500);  // Near first pit
    enemyManager.AddMasher(5700.0f, 1700.0f, 2000);  // Later in level
    
    // === NEW ENEMY TYPES ===
    enemyManager.AddNewtronBlue(1400.0f, 1632.0f);
    enemyManager.AddNewtronGreen(2800.0f, 1632.0f);
    enemyManager.AddBomb(900.0f, 1632.0f);
    enemyManager.AddBomb(4200.0f, 1632.0f);
    enemyManager.AddCaterkiller(1600.0f, 1640.0f);
    enemyManager.AddCaterkiller(3200.0f, 1640.0f);
    enemyManager.AddBatbrain(1000.0f, 1300.0f);
    enemyManager.AddBatbrain(2400.0f, 1200.0f);
    enemyManager.AddBurrobot(1100.0f, 1632.0f);
    enemyManager.AddBurrobot(3000.0f, 1632.0f);
    enemyManager.AddRoller(2200.0f, 1632.0f);
    enemyManager.AddRoller(4500.0f, 1632.0f);
    enemyManager.AddJaws(2950.0f, 1750.0f);
    enemyManager.AddJaws(5750.0f, 1750.0f);
    enemyManager.AddBallHog(3300.0f, 1632.0f);
    enemyManager.AddOrbinaut(3600.0f, 1632.0f);
    enemyManager.AddOrbinaut(5000.0f, 1632.0f);
    
    // Add springs - bounce Sonic to higher platforms
    springManager.Add(500.0f, 1632.0f, SpringDirection::Up, true);
    springManager.Add(1500.0f, 1632.0f, SpringDirection::Up, true);
    springManager.Add(2000.0f, 1200.0f, SpringDirection::Up, false);
    springManager.Add(3200.0f, 1632.0f, SpringDirection::DiagonalUpRight, true);
    
    // Add spikes - hazards
    spikeManager.Add(750.0f, 1632.0f, SpikeDirection::Up);
    spikeManager.Add(782.0f, 1632.0f, SpikeDirection::Up);
    spikeManager.Add(2200.0f, 1632.0f, SpikeDirection::Up);
    spikeManager.Add(2232.0f, 1632.0f, SpikeDirection::Up);
    spikeManager.Add(2264.0f, 1632.0f, SpikeDirection::Up);
    
    // Add checkpoints
    checkpointManager.SetInitialSpawn(150.0f, 1600.0f);
    checkpointManager.Add(1000.0f, 1600.0f);
    checkpointManager.Add(2500.0f, 1600.0f);
    checkpointManager.Add(4500.0f, 1536.0f);
    
    // Add monitors/item boxes
    monitorManager.Add(400.0f, 1620.0f, MonitorType::Ring);
    monitorManager.Add(900.0f, 1180.0f, MonitorType::Shield);
    monitorManager.Add(1700.0f, 1620.0f, MonitorType::SpeedShoes);
    monitorManager.Add(2400.0f, 1180.0f, MonitorType::Invincibility);
    monitorManager.Add(3000.0f, 1620.0f, MonitorType::ExtraLife);
    monitorManager.Add(3800.0f, 1620.0f, MonitorType::Ring);
}

void CreateTestTerrain() {
    const int TILES_X = 80;
    const int TILES_Y = 15;
    
    // Get tileset from resource manager
    engine::BitmapPtr tileset = ResourceManager::Instance().GetTilesSheet();
    
    actionLayer = new TileLayer();
    actionLayer->Create(static_cast<Dim>(TILES_Y), static_cast<Dim>(TILES_X), tileset);
    actionLayer->SetViewWindow({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});
    
    // ================================================================
    // PDF COMPLIANCE NOTE: 
    // The PDF requires loading terrain from CSV exported from Tiled editor.
    // Currently using programmatic terrain for testing.
    // 
    // TODO for full compliance:
    // 1. Create terrain in Tiled editor
    // 2. Export to CSV using TileMapLoader::LoadTileLayerCSV()
    // 3. Export grid layer to separate CSV
    // 4. Load grid using TileMapLoader::LoadGridLayerCSV()
    //
    // The collision_map.csv file exists and is loaded below.
    // ================================================================
    
    // Try to load grid from CSV (PDF requirement)
    bool csvLoaded = LoadGridFromCSV(g_assetPath + "collision_map.csv", gridLayer, TILES_X, TILES_Y);
    
    if (!csvLoaded) {
        std::cout << "CSV grid not found, using programmatic grid" << std::endl;
        // Fallback to programmatic grid creation
        Dim gridRows = static_cast<Dim>(TILES_Y * GRID_BLOCK_ROWS);
        Dim gridCols = static_cast<Dim>(TILES_X * GRID_BLOCK_COLUMNS);
        gridLayer = new GridLayer(gridRows, gridCols);
        
        // Ground
        int groundStartRow = (TILES_Y - 2) * GRID_BLOCK_ROWS;
        for (Dim row = static_cast<Dim>(groundStartRow); row < gridRows; ++row) {
            for (Dim col = 0; col < gridCols; ++col) {
                gridLayer->SetTile(col, row, GRID_SOLID);
            }
        }
    } else {
        std::cout << "Loaded grid from CSV file (PDF compliant)" << std::endl;
    }
    
    // Set up tile indices for the level
    // Tileset is 5 columns x 11 rows at 182x182 pixels
    // Tile index = row * 5 + col (0-based)
    // Index 0 = empty, so we use 1+ for actual tiles
    
    // Looking at tiles_first_map_fixed.png:
    // Row 0 (indices 1-5): Ground with palm trees, slopes
    // Row 1 (indices 6-10): More ground variations  
    // Row 2+ : Various terrain pieces
    
    const TileIndex TILE_EMPTY = 0;           // Empty/sky
    const TileIndex TILE_GROUND_PALM1 = 1;    // Ground with palm tree (top-left of tileset)
    const TileIndex TILE_GROUND_PALM2 = 2;    // Ground with different palm
    const TileIndex TILE_SLOPE_R = 3;         // Slope going right
    const TileIndex TILE_SLOPE_L = 4;         // Slope going left
    const TileIndex TILE_GROUND_EDGE = 5;     // Ground edge
    const TileIndex TILE_CLIFF = 6;           // Row 1, first tile
    const TileIndex TILE_UNDERGROUND = 7;     // Underground fill
    const TileIndex TILE_PLATFORM = 8;        // Platform piece
    const TileIndex TILE_GROUND_FLAT = 11;    // Row 2, second tile - flat ground
    
    (void)TILE_EMPTY;  // Suppress unused warnings
    (void)TILE_SLOPE_R;
    (void)TILE_SLOPE_L;
    (void)TILE_GROUND_EDGE;
    (void)TILE_CLIFF;
    
    // Set ground tiles (last 2 rows)
    for (int col = 0; col < TILES_X; ++col) {
        // Skip pit columns
        if ((col >= 23 && col <= 25) || (col >= 45 && col <= 48)) continue;
        
        // Top of ground (row 13) - alternate between palm variations
        TileIndex groundTile = (col % 3 == 0) ? TILE_GROUND_PALM1 : 
                              (col % 3 == 1) ? TILE_GROUND_PALM2 : TILE_GROUND_FLAT;
        actionLayer->SetTile(static_cast<Dim>(col), 13, groundTile);
        
        // Underground fill (row 14)
        actionLayer->SetTile(static_cast<Dim>(col), 14, TILE_UNDERGROUND);
    }
    
    // Platform tiles (row 11 platforms)
    for (int col = 5; col < 10; ++col) actionLayer->SetTile(static_cast<Dim>(col), 11, TILE_PLATFORM);
    for (int col = 15; col < 22; ++col) actionLayer->SetTile(static_cast<Dim>(col), 11, TILE_PLATFORM);
    for (int col = 30; col < 38; ++col) actionLayer->SetTile(static_cast<Dim>(col), 11, TILE_PLATFORM);
    for (int col = 50; col < 58; ++col) actionLayer->SetTile(static_cast<Dim>(col), 11, TILE_PLATFORM);
    for (int col = 65; col < 72; ++col) actionLayer->SetTile(static_cast<Dim>(col), 11, TILE_PLATFORM);
    
    // Higher platforms (row 9)
    for (int col = 8; col < 14; ++col) actionLayer->SetTile(static_cast<Dim>(col), 9, TILE_PLATFORM);
    for (int col = 25; col < 32; ++col) actionLayer->SetTile(static_cast<Dim>(col), 9, TILE_PLATFORM);
    for (int col = 42; col < 50; ++col) actionLayer->SetTile(static_cast<Dim>(col), 9, TILE_PLATFORM);
    for (int col = 60; col < 68; ++col) actionLayer->SetTile(static_cast<Dim>(col), 9, TILE_PLATFORM);
    
    // Highest platforms (row 7)
    for (int col = 12; col < 18; ++col) actionLayer->SetTile(static_cast<Dim>(col), 7, TILE_PLATFORM);
    for (int col = 35; col < 42; ++col) actionLayer->SetTile(static_cast<Dim>(col), 7, TILE_PLATFORM);
    for (int col = 55; col < 62; ++col) actionLayer->SetTile(static_cast<Dim>(col), 7, TILE_PLATFORM);
    
    // ==== GRID COLLISION LAYER ====
    // Already loaded at start of function - add platforms if using programmatic fallback
    Dim gridRows = gridLayer->GetRows();
    Dim gridCols = gridLayer->GetCols();
    
    // Define groundStartRow for use in both CSV and programmatic paths
    int groundStartRow = (TILES_Y - 2) * GRID_BLOCK_ROWS;
    
    auto makePlatform = [&](int tileRow, int startTileCol, int endTileCol) {
        int row = tileRow * GRID_BLOCK_ROWS;
        for (int col = startTileCol * GRID_BLOCK_COLUMNS; col < endTileCol * GRID_BLOCK_COLUMNS; ++col) {
            if (col >= 0 && col < static_cast<int>(gridCols) && row >= 0 && row < static_cast<int>(gridRows)) {
                gridLayer->SetTile(static_cast<Dim>(col), static_cast<Dim>(row), GRID_TOP_SOLID);
            }
        }
    };
    
    auto makeBlock = [&](int startRow, int endRow, int startCol, int endCol) {
        for (int row = startRow * GRID_BLOCK_ROWS; row < endRow * GRID_BLOCK_ROWS; ++row) {
            for (int col = startCol * GRID_BLOCK_COLUMNS; col < endCol * GRID_BLOCK_COLUMNS; ++col) {
                if (col >= 0 && col < static_cast<int>(gridCols) && row >= 0 && row < static_cast<int>(gridRows)) {
                    gridLayer->SetTile(static_cast<Dim>(col), static_cast<Dim>(row), GRID_SOLID);
                }
            }
        }
    };
    
    // Only add manual adjustments if we loaded from CSV (to fix any issues)
    // Skip these if using programmatic fallback since they're redundant
    
    // Platforms (will overlay on CSV data)
    makePlatform(11, 5, 10);
    makePlatform(11, 15, 22);
    makePlatform(11, 30, 38);
    makePlatform(11, 50, 58);
    makePlatform(11, 65, 72);
    makePlatform(9, 8, 14);
    makePlatform(9, 25, 32);
    makePlatform(9, 42, 50);
    makePlatform(9, 60, 68);
    makePlatform(7, 12, 18);
    makePlatform(7, 35, 42);
    makePlatform(7, 55, 62);
    
    // Obstacles - both visual and collision
    makeBlock(11, 13, 20, 21);
    actionLayer->SetTile(20, 12, TILE_UNDERGROUND);  // Visual wall
    actionLayer->SetTile(20, 11, TILE_UNDERGROUND);
    
    makeBlock(9, 13, 40, 41);
    actionLayer->SetTile(40, 10, TILE_UNDERGROUND);
    actionLayer->SetTile(40, 11, TILE_UNDERGROUND);
    actionLayer->SetTile(40, 12, TILE_UNDERGROUND);
    actionLayer->SetTile(40, 9, TILE_UNDERGROUND);
    
    makeBlock(7, 13, 58, 59);
    for (int row = 7; row < 13; ++row) {
        actionLayer->SetTile(58, static_cast<Dim>(row), TILE_UNDERGROUND);
    }
    
    // Add visible walls at left edge
    for (int row = 0; row < TILES_Y - 2; ++row) {
        actionLayer->SetTile(0, static_cast<Dim>(row), TILE_UNDERGROUND);
    }
    // Add collision for left wall
    for (Dim row = 0; row < gridRows; ++row) {
        gridLayer->SetTile(0, row, GRID_SOLID);
        gridLayer->SetTile(1, row, GRID_SOLID);
    }
    
    // Stairs
    for (int i = 0; i < 5; ++i) {
        makeBlock(13 - i - 1, 13 - i, 70 + i*2, 72 + i*2);
    }
    
    // Pits
    for (Dim row = static_cast<Dim>(groundStartRow); row < gridRows; ++row) {
        for (int col = 23 * GRID_BLOCK_COLUMNS; col < 25 * GRID_BLOCK_COLUMNS; ++col) {
            gridLayer->SetTile(static_cast<Dim>(col), row, GRID_EMPTY);
        }
        for (int col = 45 * GRID_BLOCK_COLUMNS; col < 48 * GRID_BLOCK_COLUMNS; ++col) {
            gridLayer->SetTile(static_cast<Dim>(col), row, GRID_EMPTY);
        }
    }
    
    // Setup managers
    ringManager.SetGridLayer(gridLayer);
    enemyManager.SetGridLayer(gridLayer);
    
    // Place all game objects (extracted to allow reset)
    PlaceGameObjects();
    
    // Setup parallax background using the background sheet
    auto bgSheet = ResourceManager::Instance().GetBackgroundSheet();
    SetupGreenHillBackground(parallaxManager, bgSheet);
    
    ringManager.SetOnRingCollected([](int count) {
        std::cout << "Collected " << count << " ring(s)!" << std::endl;
        gameHUD.AddRings(count);
    });
    enemyManager.SetOnEnemyKilled([](int score) {
        std::cout << "Enemy killed! +" << score << " points" << std::endl;
        gameHUD.AddScore(score);
    });
    enemyManager.SetOnAnimalFreed([](float x, float y) {
        std::cout << "Animal freed at (" << x << ", " << y << ")" << std::endl;
        animalManager.Spawn(x, y);
    });
    
    // Start level timer
    gameHUD.StartLevel(GetSystemTime());
}

int main() {
    std::cout << "=== Sonic Engine - CS454 Project ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left/Right or A/D: Move" << std::endl;
    std::cout << "  Space/Up/W: Jump" << std::endl;
    std::cout << "  Down/S: Crouch (still) or Roll (moving)" << std::endl;
    std::cout << "  Down + Jump: Spindash (charge and release)" << std::endl;
    std::cout << "  Left Click: Look Up (when still)" << std::endl;
    std::cout << "  G: Grid   F1: Debug   ESC: Pause   Q: Quit" << std::endl;
    std::cout << std::endl;
    
    if (!EngineInit(SCREEN_WIDTH, SCREEN_HEIGHT, "Sonic Engine - SFML")) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return 1;
    }
    
    // Detect asset path early (handles running from project root OR bin/)
    g_assetPath = FindAssetPath();
    std::cout << "Using asset path: " << g_assetPath << std::endl;
    
    // Load assets using the detected path
    ResourceManager::Instance().SetAssetPath(g_assetPath);
    assetsLoaded = ResourceManager::Instance().LoadAll();
    
    if (!assetsLoaded) {
        std::cout << "Assets not found - using placeholder graphics" << std::endl;
        std::cout << "To use real sprites, copy sprite sheets to assets/ folder" << std::endl;
    }
    
    CreateTestTerrain();
    // showGrid toggled with G key
    
    sonic = new SonicPlayer();
    sonic->Create(150.0f, 1600.0f, gridLayer);
    
    SonicPlayer::PhysicsConfig config;
    config.acceleration = 0.08f;
    config.deceleration = 0.4f;
    config.friction = 0.06f;
    config.topSpeed = 8.0f;
    config.fullSpeedThreshold = 6.5f;
    config.jumpForce = 11.0f;
    config.gravity = 0.35f;
    config.maxFallSpeed = 16.0f;
    config.airAcceleration = 0.15f;
    sonic->SetPhysicsConfig(config);
    
    sonic->SetOnRingsChanged([](int, int newRings) { gameHUD.rings = newRings; });
    sonic->SetOnLivesChanged([](int, int newLives) { gameHUD.lives = newLives; });
    sonic->SetOnStateChanged([](SonicState state) {
        std::cout << "State: " << StateToString(state);
        // Show which animation will be used
        switch(state) {
            case SonicState::Idle: std::cout << " -> sonic_idle"; break;
            case SonicState::Bored: std::cout << " -> sonic_bored"; break;
            case SonicState::LookingUp: std::cout << " -> sonic_lookup"; break;
            case SonicState::Crouching: std::cout << " -> sonic_crouch"; break;
            case SonicState::Spindash: std::cout << " -> sonic_spindash (revving)"; break;
            case SonicState::Walking: std::cout << " -> sonic_walk"; break;
            case SonicState::Running: std::cout << " -> sonic_walk (fast)"; break;
            case SonicState::FullSpeed: std::cout << " -> sonic_run"; break;
            case SonicState::Jumping: std::cout << " -> sonic_ball"; break;
            case SonicState::Rolling: std::cout << " -> sonic_ball"; break;
            case SonicState::Skidding: std::cout << " -> sonic_skid"; break;
            case SonicState::Balancing: std::cout << " -> sonic_balance"; break;
            case SonicState::Pushing: std::cout << " -> sonic_push"; break;
            case SonicState::Spring: std::cout << " -> sonic_spring/ball (up/down)"; break;
            case SonicState::Landing: std::cout << " -> sonic_idle (landing)"; break;
            case SonicState::Hurt: std::cout << " -> sonic_hurt"; break;
            case SonicState::Dead: std::cout << " -> sonic_death"; break;
            default: break;
        }
        std::cout << std::endl;
    });
    sonic->SetOnScatterRings([](float x, float y, int count) {
        ringManager.ScatterRings(x, y, count);
        std::cout << "Scattered " << count << " rings!" << std::endl;
    });
    sonic->SetOnDeath([]() {
        std::cout << "=== GAME OVER ===" << std::endl;
        currentScreen = GameScreen::GameOver;
    });
    
    GetGame().SetOnPauseResume([]() {
        if (!GetGame().IsPaused()) {
            auto dt = GetSystemTime() - GetGame().GetPauseTime();
            AnimatorManager::Instance().TimeShift(dt);
        }
        gamePaused = GetGame().IsPaused();
    });
    
    GetGame().SetRender(Render);
    GetGame().SetInput(HandleInput);
    GetGame().SetPhysics(Physics);
    GetGame().SetProgressAnimations([]() {
        AnimatorManager::Instance().Progress(GetSystemTime());
    });
    GetGame().SetCollisionChecking([]() {
        CollisionChecker::Instance().Check();
    });
    GetGame().SetCommitDestructions([]() {
        DestructionManager::Instance().Commit();
    });
    GetGame().SetUserCode(UpdateViewWindow);
    GetGame().SetDone([]() { return !gameRunning || EngineShouldQuit(); });
    
    GetGame().MainLoop();
    
    delete sonic;
    delete actionLayer;
    delete gridLayer;
    ResourceManager::Instance().Cleanup();
    EngineShutdown();
    
    std::cout << "Game ended." << std::endl;
    return 0;
}
