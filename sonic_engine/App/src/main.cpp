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
#include "SoundManager.hpp"
#include "WhirlCircle.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace engine;
using namespace app;

//helper function to find asset path - handles running from project root OR bin/ directory
std::string FindAssetPath() {
    std::vector<std::string> possiblePaths = {
        "assets/",  //running from project root (./bin/SonicGame)
        "../assets/",  //running from bin/ directory (./SonicGame)
        "./assets/",  //explicit current directory
        "sonic_engine/assets/"  //running from parent of project
    };
    
    for (const auto& path : possiblePaths) {
        std::ifstream testFile(path + "sonic_sheet_fixed.png");
        if (testFile.good()) {
            testFile.close();
            return path;
        }
    }
    return "assets/";  //fallback to default
}

//global asset path - detected once at startup
std::string g_assetPath;

//forward declarations
void ResetLevel();
const char* StateToString(SonicState state);

//frustum culling helper - check if object is near view
inline bool IsNearView(float x, float y, const Rect& view, int margin = 200) {
    return x >= view.x - margin && x <= view.x + view.w + margin &&
           y >= view.y - margin && y <= view.y + view.h + margin;
}

//screen constants - Genesis native resolution scaled 2x
//virtual: 320x224 (Sega Genesis native)
//window: 640x448 (2x scale)
constexpr Dim WINDOW_WIDTH = 640;
constexpr Dim WINDOW_HEIGHT = 448;
constexpr Dim VIRTUAL_WIDTH = 320;
constexpr Dim VIRTUAL_HEIGHT = 224;
constexpr Dim SCREEN_WIDTH = VIRTUAL_WIDTH;
constexpr Dim SCREEN_HEIGHT = VIRTUAL_HEIGHT;
//sprites rendered 1:1 at virtual resolution
constexpr float SPRITE_SCALE = 1.0f;

//game state
enum class GameScreen {
    Title,
    Playing,
    GameOver
};

GameScreen currentScreen = GameScreen::Title;
bool gameRunning = true;
uint64_t gameOverStartTime = 0;  //when game over screen appeared (for input delay)
bool gamePaused = false;
bool showGrid = false;
bool showDebug = true;
bool assetsLoaded = false;

//god mode cheat - hold G for 5 seconds
uint64_t gKeyHoldStart = 0;
bool godModeActive = false;
constexpr uint64_t GOD_MODE_HOLD_TIME = 5000;  //5 seconds

//kill/respawn cheat - hold K for 3 seconds
uint64_t kKeyHoldStart = 0;
constexpr uint64_t KILL_HOLD_TIME = 3000;  //3 seconds

//menu state
int pauseMenuSelection = 0;  //0=Continue, 1=Restart, 2=Quit
constexpr int PAUSE_MENU_ITEMS = 3;
uint64_t lastMenuClickTime = 0;  //for double-click detection

//title screen menu
int titleMenuSelection = 0;  //0=Start, 1=Sound Toggle
bool soundEnabled = true;  //sound on/off toggle

const float SCROLL_FACTORS[] = {0.5f, 1.0f, 1.5f, 2.0f};
int scrollFactorIndex = 0;
constexpr int NUM_SCROLL_FACTORS = 4;

//invincibility sparkle state - lighter, smoother effect
struct SparkleEffect {
    float angle = 0.0f;
    float radius = 40.0f;  //sparkle orbit radius
    int numSparkles = 4;
    uint64_t lastUpdate = 0;
    float alpha = 0.7f;  //transparency for smoother look
} sparkleEffect;

uint64_t deathStartTime = 0;  //death animation timing (global to properly reset)

//terrain
TileLayer* actionLayer = nullptr;
GridLayer* gridLayer = nullptr;

//player
SonicPlayer* sonic = nullptr;

//game objects
RingManager ringManager;
EnemyManager enemyManager;
SpringManager springManager;
SpikeManager spikeManager;
CheckpointManager checkpointManager;
MonitorManager monitorManager;
FlowerManager flowerManager;
BigRingManager bigRingManager;
whirl::WhirlCircleManager whirlCircleManager;

//level complete state
bool levelComplete = false;
uint64_t levelCompleteTime = 0;
int levelCompleteSelection = 0;  //0 = Restart, 1 = Last Checkpoint

//background
ParallaxManager parallaxManager;

//HUD
HUD gameHUD;

//view window (follows player)
Rect viewWindow = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

//sprite animation state
int sonicFrame = 0;
uint64_t lastFrameTime = 0;
std::string currentAnim = "sonic_idle";

//forward declare object placement and ground finder
void PlaceGameObjects(bool includeMonitorsAndLevelObjects = true);
int FindGroundY(int pixelX);

//new terrain constants (39×6 tiles at 256×256 = 9984×1536 pixels)
constexpr int MAP_WIDTH = 9984;  //39 tiles × 256
constexpr int MAP_HEIGHT = 1536;  //6 tiles × 256

void ResetLevel() {
    std::cout << "=== RESETTING LEVEL ===" << std::endl;
    
//reset HUD (not lives - that's in Sonic)
    gameHUD.score = 0;
    gameHUD.rings = 0;
    gameHUD.StartLevel(GetSystemTime());
    
//reset Sonic completely first
    if (sonic) {
        sonic->Reset();  //full reset: lives=3, rings=0, state=Idle, NO invincibility
//spawn above ground at X=100 (ground at Y≈1216, spawn above it)
        sonic->SetPosition(100.0f, 1100.0f);
    }
    
//clear all game objects
    ringManager.Clear();
    enemyManager.Clear();
    AnimalManager::Instance().Clear();
    springManager.Clear();
    spikeManager.Clear();
    checkpointManager.Clear();
    monitorManager.Clear();
    flowerManager.Clear();
    bigRingManager.Clear();
    
//reset level complete state
    levelComplete = false;
    levelCompleteTime = 0;
    levelCompleteSelection = 0;
    
//reset death timer
    deathStartTime = 0;
    
//re-place all objects
    PlaceGameObjects();
    
    std::cout << "=== LEVEL RESET COMPLETE ===" << std::endl;
}

void UpdateViewWindow() {
    if (!sonic || !gridLayer) return;

//camera centers on Sonic horizontally, positions Sonic in lower part of screen
//this shows more of the terrain above (palm trees, etc.)
    int targetX = sonic->GetIntX() - SCREEN_WIDTH / 2;
    int targetY = sonic->GetIntY() - (SCREEN_HEIGHT * 3 / 4);  //changed from 2/3 to 3/4 - shows more above

//clamp to actual map bounds - no artificial limits
    if (targetX < 0) targetX = 0;
    if (targetX > MAP_WIDTH - SCREEN_WIDTH) targetX = MAP_WIDTH - SCREEN_WIDTH;
    if (targetY < 0) targetY = 0;
    if (targetY > MAP_HEIGHT - SCREEN_HEIGHT) targetY = MAP_HEIGHT - SCREEN_HEIGHT;

    viewWindow.x = targetX;
    viewWindow.y = targetY;
    viewWindow.w = SCREEN_WIDTH;
    viewWindow.h = SCREEN_HEIGHT;

    if (actionLayer) {
        actionLayer->SetViewWindow(viewWindow);
    }

//update sound system
    SoundManager::Instance().Update();
}

void HandleInput() {
    auto& input = GetInput();
    input.Poll();
    
//title screen input with menu navigation
    if (currentScreen == GameScreen::Title) {
//navigate menu with Up/Down
        if (input.IsKeyJustPressed(KeyCode::Up)) {
            titleMenuSelection = (titleMenuSelection - 1 + 2) % 2;
        }
        if (input.IsKeyJustPressed(KeyCode::Down)) {
            titleMenuSelection = (titleMenuSelection + 1) % 2;
        }
        
//select with Enter/Space
        if (input.IsKeyJustPressed(KeyCode::Enter) || input.IsKeyJustPressed(KeyCode::Space)) {
            if (titleMenuSelection == 0) {
//start Game
                currentScreen = GameScreen::Playing;
                gameHUD.StartLevel(GetSystemTime());
            } else if (titleMenuSelection == 1) {
//toggle Sound
                soundEnabled = !soundEnabled;
                SoundManager::Instance().EnableMusic(soundEnabled);
                SoundManager::Instance().EnableSound(soundEnabled);
//immediately stop or restart music
                if (!soundEnabled) {
                    SoundManager::Instance().StopMusic();
                } else {
                    SoundManager::Instance().PlayMusic(MusicTrack::GreenHillZone);
                }
            }
        }
        
        if (input.IsKeyJustPressed(KeyCode::Q) || input.IsWindowClosed()) {
            gameRunning = false;
        }
        input.Update();
        return;
    }
    
//game Over screen input
    if (currentScreen == GameScreen::GameOver) {
//IMPORTANT: Require 1 second delay before accepting input
//this prevents accidental restarts from held keys during death
        uint64_t now = GetSystemTime();
        bool inputAllowed = (gameOverStartTime > 0 && now - gameOverStartTime > 1000);
        
        if (inputAllowed) {
            if (input.IsKeyJustPressed(KeyCode::Enter) || input.IsKeyJustPressed(KeyCode::Space)) {
//restart game
                std::cout << "=== RESTARTING GAME (player input) ===" << std::endl;
                currentScreen = GameScreen::Playing;
                gameOverStartTime = 0;  //reset for next game over
                ResetLevel();
            }
            if (input.IsKeyJustPressed(KeyCode::Q) || input.IsWindowClosed()) {
                gameRunning = false;
            }
        }
        input.Update();
        return;
    }
    
//normal gameplay input
    if (input.IsKeyJustPressed(KeyCode::Escape)) {
        if (gamePaused) {
//if paused, ESC resumes
            GetGame().Resume();
            SoundManager::Instance().ResumeMusic();
            pauseMenuSelection = 0;
        } else {
            GetGame().Pause(GetSystemTime());
            SoundManager::Instance().PauseMusic();
        }
    }

//M key to toggle music on/off
    if (input.IsKeyJustPressed(KeyCode::M)) {
        soundEnabled = !soundEnabled;
        SoundManager::Instance().EnableMusic(soundEnabled);
        SoundManager::Instance().EnableSound(soundEnabled);
        if (!soundEnabled) {
            SoundManager::Instance().StopMusic();
        } else {
            if (!gamePaused && currentScreen == GameScreen::Playing) {
                SoundManager::Instance().PlayMusic(MusicTrack::GreenHillZone);
            }
        }
    }

//pause menu navigation
    if (gamePaused) {
//mouse selection - convert window coords to virtual coords
        Point mousePos = input.GetMousePosition();
//scale mouse position from window (640x448) to virtual (320x224)
        int virtualMouseX = mousePos.x / 2;
        int virtualMouseY = mousePos.y / 2;
        
//menu positions (must match RenderPauseMenu)
        int menuX = 60;
        int menuY = 30;
        int itemY = menuY + 40;
        
//check if mouse is hovering over any menu item
        for (int i = 0; i < PAUSE_MENU_ITEMS; ++i) {
            int itemLeft = menuX + 30;
            int itemRight = menuX + 170;
            int itemTop = itemY + i * 28;
            int itemBottom = itemTop + 22;
            
            if (virtualMouseX >= itemLeft && virtualMouseX <= itemRight &&
                virtualMouseY >= itemTop && virtualMouseY <= itemBottom) {
                pauseMenuSelection = i;  //hover selects
                
//click or double-click to confirm
                if (input.IsMouseJustPressed(MouseButton::Left)) {
                    uint64_t now = GetSystemTime();
                    bool isDoubleClick = (now - lastMenuClickTime < 400);  //400ms window
                    lastMenuClickTime = now;
                    
                    if (isDoubleClick) {
//double-click confirms immediately
                        switch (pauseMenuSelection) {
                            case 0:
                                GetGame().Resume();
                                SoundManager::Instance().ResumeMusic();
                                pauseMenuSelection = 0;
                                break;
                            case 1:
                                GetGame().Resume();
                                SoundManager::Instance().ResumeMusic();
                                pauseMenuSelection = 0;
                                ResetLevel();
                                break;
                            case 2: gameRunning = false; break;
                        }
                    }
                }
            }
        }
        
//keyboard navigation (still works)
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
                case 0:  //continue
                    GetGame().Resume();
                    SoundManager::Instance().ResumeMusic();
                    pauseMenuSelection = 0;
                    break;
                case 1:  //restart
                    GetGame().Resume();
                    SoundManager::Instance().ResumeMusic();
                    pauseMenuSelection = 0;
                    ResetLevel();
                    break;
                case 2:  //quit
                    gameRunning = false;
                    break;
            }
        }
        input.Update();
        return;
    }
    
//level complete menu navigation
    if (levelComplete) {
        if (input.IsKeyJustPressed(KeyCode::Up) || input.IsKeyJustPressed(KeyCode::W)) {
            levelCompleteSelection--;
            if (levelCompleteSelection < 0) levelCompleteSelection = 1;
        }
        if (input.IsKeyJustPressed(KeyCode::Down) || input.IsKeyJustPressed(KeyCode::S)) {
            levelCompleteSelection++;
            if (levelCompleteSelection > 1) levelCompleteSelection = 0;
        }
        if (input.IsKeyJustPressed(KeyCode::Enter)) {
            switch (levelCompleteSelection) {
                case 0:  //restart Level
                    ResetLevel();
                    break;
                case 1:  //last Checkpoint
                    levelComplete = false;
                    if (sonic) {
                        float spawnX, spawnY;
                        checkpointManager.GetRespawnPosition(spawnX, spawnY);
                        sonic->SetPosition(spawnX, spawnY);
                        sonic->Reset();  //reset to idle state
                    }
                    break;
            }
        }
        input.Update();
        return;
    }
    
//q to quit - only works when NOT in active gameplay OR when holding for 0.5 seconds
//this prevents accidental Q presses during intense gameplay
    static uint64_t qPressStartTime = 0;
    static const uint64_t HOLD_TO_QUIT_DURATION = 500;  //0.5 seconds
    
    if (input.IsKeyPressed(KeyCode::Q)) {
        if (qPressStartTime == 0) {
            qPressStartTime = GetSystemTime();
        }
        uint64_t heldDuration = GetSystemTime() - qPressStartTime;
        if (heldDuration >= HOLD_TO_QUIT_DURATION) {
            std::cout << "=== QUIT: Q held for " << heldDuration << "ms ===" << std::endl;
            gameRunning = false;
        }
    } else {
        qPressStartTime = 0;  //reset when Q released
    }
    
    if (input.IsWindowClosed()) {
        std::cout << "=== QUIT: Window closed ===" << std::endl;
        gameRunning = false;
    }
    
//g key: tap for grid toggle, hold 5 seconds for god mode
    if (input.IsKeyPressed(KeyCode::G)) {
        uint64_t now = engine::GetSystemTime();
        if (gKeyHoldStart == 0) {
            gKeyHoldStart = now;
        } else if (!godModeActive && (now - gKeyHoldStart >= GOD_MODE_HOLD_TIME)) {
//held for 5 seconds - toggle god mode
            godModeActive = !godModeActive;
            if (sonic) {
                sonic->SetGodMode(godModeActive);
            }
            gKeyHoldStart = now + 10000;  //prevent immediate re-trigger
        }
    } else {
//g key released
        if (gKeyHoldStart != 0) {
            uint64_t holdTime = engine::GetSystemTime() - gKeyHoldStart;
            if (holdTime < 500) {  //quick tap (less than 500ms) = grid toggle
                showGrid = !showGrid;
                std::cout << "Grid overlay: " << (showGrid ? "ON" : "OFF") << std::endl;
            }
            gKeyHoldStart = 0;
        }
    }
    
    if (input.IsKeyJustPressed(KeyCode::F1)) {
        showDebug = !showDebug;
        std::cout << "Debug panel: " << (showDebug ? "ON" : "OFF") << std::endl;
    }
    
//k key: hold for 3 seconds to kill and respawn at checkpoint (works even in god mode)
    if (input.IsKeyPressed(KeyCode::K)) {
        uint64_t now = engine::GetSystemTime();
        if (kKeyHoldStart == 0) {
            kKeyHoldStart = now;
        } else if ((now - kKeyHoldStart >= KILL_HOLD_TIME)) {
//held for 3 seconds - kill and respawn
            if (sonic && !sonic->IsDead()) {
//force death and respawn
                bool wasGodMode = godModeActive;
                godModeActive = false;
                if (sonic) sonic->SetGodMode(false);
                
//respawn at checkpoint
                float respawnX, respawnY;
                checkpointManager.GetRespawnPosition(respawnX, respawnY);
                sonic->SetPosition(respawnX, respawnY - 50);
                sonic->Reset();
                sonic->LoseLife();  //lose a life
                
//restore god mode if it was active
                godModeActive = wasGodMode;
                if (sonic) sonic->SetGodMode(godModeActive);
                
                kKeyHoldStart = now + 5000;  //prevent immediate re-trigger
            }
        }
    } else {
        kKeyHoldStart = 0;
    }
    
//t key: hold for 2 seconds to teleport to tunnel test location
    static uint64_t tKeyHoldStart = 0;
    static constexpr uint64_t TELEPORT_HOLD_TIME = 2000;  //2 seconds
    if (input.IsKeyPressed(KeyCode::T)) {
        uint64_t now = engine::GetSystemTime();
        if (tKeyHoldStart == 0) {
            tKeyHoldStart = now;
        } else if ((now - tKeyHoldStart >= TELEPORT_HOLD_TIME)) {
//held for 2 seconds - teleport to tunnel
            if (sonic) {
                sonic->SetPosition(5825.0f, 904.0f);
                std::cout << "=== TELEPORTED TO TUNNEL TEST (5825, 904) ===" << std::endl;
                tKeyHoldStart = now + 3000;  //prevent immediate re-trigger
            }
        }
    } else {
        tKeyHoldStart = 0;
    }
    
    if (input.IsKeyJustPressed(KeyCode::F11)) {
        GetGraphics().ToggleFullscreen();
        std::cout << "Fullscreen: " << (GetGraphics().IsFullscreen() ? "ON" : "OFF") << std::endl;
    }
    
//+ key: next factor (looped: 2.0 -> 0.5)
    if (input.IsKeyJustPressed(KeyCode::Plus) || input.IsKeyJustPressed(KeyCode::Equals)) {
        scrollFactorIndex = (scrollFactorIndex + 1) % NUM_SCROLL_FACTORS;
        std::cout << "Scroll factor: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
//- key: previous factor (looped: 0.5 -> 2.0)
    if (input.IsKeyJustPressed(KeyCode::Minus)) {
        scrollFactorIndex = (scrollFactorIndex - 1 + NUM_SCROLL_FACTORS) % NUM_SCROLL_FACTORS;
        std::cout << "Scroll factor: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
//0 key: reset to default (0.5)
    if (input.IsKeyJustPressed(KeyCode::Num0)) {
        scrollFactorIndex = 0;  //0.5f
        std::cout << "Scroll factor reset to: " << SCROLL_FACTORS[scrollFactorIndex] << std::endl;
    }
//home key: scroll to top-left
    if (input.IsKeyJustPressed(KeyCode::Home)) {
        viewWindow.x = 0;
        viewWindow.y = 0;
        std::cout << "View: top-left" << std::endl;
    }
//end key: scroll to bottom-right
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
        
//up/W always jumps now
        bool jump = input.IsKeyPressed(KeyCode::Space) || input.IsKeyPressed(KeyCode::Up) || 
                    input.IsKeyPressed(KeyCode::W);
        bool jumpPressed = input.IsKeyJustPressed(KeyCode::Space) || 
                           input.IsKeyJustPressed(KeyCode::Up) ||
                           input.IsKeyJustPressed(KeyCode::W);
        
//mouse click OR U key for look up (only when standing still)
        bool lookUp = input.IsMousePressed(MouseButton::Left) || input.IsKeyPressed(KeyCode::U);
        
        sonic->HandleInput(left, right, jump, jumpPressed, down, lookUp);
    }
    
    input.Update();
}

void UpdateSonicAnimation() {
    if (!sonic) return;
    
//determine which animation to use based on state
    std::string newAnim = "sonic_idle";
    unsigned frameDelay = 100;
    
    SonicState state = sonic->GetState();
    float speed = std::abs(sonic->GetSpeed());
    
//special handling for active loop - use loop animation
    if (sonic->IsLoopActive()) {
        newAnim = "sonic_ball";  //ball animation during loop
        frameDelay = std::max(25u, static_cast<unsigned>(60 - speed * 3));
        
        static uint64_t lastFrameTime = 0;
        uint64_t now = engine::GetSystemTime();
        if (now - lastFrameTime >= frameDelay) {
            sonicFrame++;
            lastFrameTime = now;
        }
        if (newAnim != currentAnim) {
            currentAnim = newAnim;
            sonicFrame = 0;
        }
        return;  //skip normal animation logic
    }
    
//bored animation phase tracking (file-level for proper reset)
    static uint64_t boredPhaseStart = 0;
    static int boredPhase = 0;  //0=not started, 1=stare, 2=loop
    
//reset bored phase when not in bored state
    if (state != SonicState::Bored) {
        boredPhase = 0;
        boredPhaseStart = 0;
    }
    
    switch (state) {
        case SonicState::Idle:
            newAnim = "sonic_idle";
            frameDelay = 100;
            break;
        case SonicState::Bored:
//bored sequence: stare (1 sec) then foot tap loop
            if (boredPhase == 0) {
//just entered bored - start with stare
                boredPhase = 1;
                boredPhaseStart = GetSystemTime();
                newAnim = "sonic_bored_stare";
                frameDelay = 1000;
                sonicFrame = 0;
            } else if (boredPhase == 1) {
//phase 1: wide eyes staring for 1000ms
                newAnim = "sonic_bored_stare";
                frameDelay = 1000;
                if (GetSystemTime() - boredPhaseStart > 1000) {
                    boredPhase = 2;
                    sonicFrame = 0;
                }
            } else {
//phase 2: looping foot tap
                newAnim = "sonic_bored";
                frameDelay = 500;
            }
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
//spindash uses dedicated spindash animation (curled + ball frames)
            newAnim = "sonic_spindash";
            {
                float charge = sonic ? sonic->GetSpindashCharge() : 0.0f;
//spin speed increases with charge: 60ms at 0, down to 20ms at max charge
                frameDelay = std::max(20u, static_cast<unsigned>(60 - charge * 4));
            }
            break;
        case SonicState::Walking:
//use slope animation based on ground angle
            {
                float angle = sonic->GetGroundAngle();
                if (std::abs(angle) > 12.0f) {
                    newAnim = "sonic_walk_slope";
                } else {
                    newAnim = "sonic_walk";
                }
            }
//faster animation when moving faster
            frameDelay = std::max(40u, static_cast<unsigned>(120 - speed * 15));
            break;
        case SonicState::Running:
//use walk but with faster animation (transitional state)
            {
                float angle = sonic->GetGroundAngle();
                if (std::abs(angle) > 12.0f) {
                    newAnim = "sonic_walk_slope";
                } else {
                    newAnim = "sonic_walk";
                }
            }
            frameDelay = std::max(30u, static_cast<unsigned>(60 - speed * 5));
            break;
        case SonicState::FullSpeed:
//use slope animation based on ground angle
            {
                float angle = sonic->GetGroundAngle();
                if (std::abs(angle) > 12.0f) {
                    newAnim = "sonic_run_slope";
                } else {
                    newAnim = "sonic_run";
                }
            }
            frameDelay = 40;
            break;
        case SonicState::Jumping:
            newAnim = "sonic_ball";
//spin faster at higher speeds
            frameDelay = std::max(25u, static_cast<unsigned>(50 - speed * 3));
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
//simple spring animation:
//going UP (velY < 0): arms spread (bounced up sprite)
//coming DOWN (velY >= 0): ball form (can kill enemies)
            if (sonic->GetVelY() < 0) {
                newAnim = "sonic_spring";  //arms spread going up
            } else {
                newAnim = "sonic_ball";  //ball form falling
            }
            frameDelay = 80;
            break;
        case SonicState::Landing:
//brief landing pose - use idle for smooth transition
            newAnim = "sonic_idle";
            frameDelay = 50;
            break;
        case SonicState::Hurt:
            newAnim = "sonic_hurt";
            frameDelay = 100;
            break;
        case SonicState::Dead:
//use death animation (falling pose with arms up)
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
//use current movement animation with sparkle effects
            if (speed > 6.0f) newAnim = "sonic_run";
            else if (speed > 0.5f) newAnim = "sonic_walk";
            else newAnim = "sonic_idle";
            frameDelay = 60;
            break;
        default:
            break;
    }
    
//handle animation changes with smooth transitions
    if (newAnim != currentAnim) {
//for walk<->run transitions, preserve animation phase
        bool wasWalkOrRun = (currentAnim == "sonic_walk" || currentAnim == "sonic_run");
        bool isWalkOrRun = (newAnim == "sonic_walk" || newAnim == "sonic_run");
        
        if (wasWalkOrRun && isWalkOrRun) {
//preserve frame progress proportionally
//walk has 6 frames, Run has 4 frames
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
    
//advance frame
    uint64_t now = GetSystemTime();
    if (now - lastFrameTime >= frameDelay) {
        sonicFrame++;
        lastFrameTime = now;
    }
}

void Physics() {
//only run physics when playing
    if (currentScreen != GameScreen::Playing) return;
    
    uint64_t currentTime = GetSystemTime();
    
    if (gamePaused) return;
    
//CRITICAL: If Sonic is dead, handle death animation specially
//NO collision checks, NO damage, but YES to animation updates and respawn logic
    if (sonic && sonic->IsDead()) {
        sonic->Update();  //allow physics for falling death animation
        UpdateSonicAnimation();
        
//handle death state - wait for animation then respawn or game over
        if (currentScreen == GameScreen::Playing) {
//track when death started
            if (deathStartTime == 0) {
                deathStartTime = currentTime;
                SoundManager::Instance().OnDeath();
            }

//wait 2 seconds for death animation, or until Sonic falls off screen
            uint64_t deathDuration = currentTime - deathStartTime;
            bool timeExpired = deathDuration > 2000;  //2 second death animation
            bool fellOffScreen = sonic->GetY() > 2100.0f;

            if (timeExpired || fellOffScreen) {
                deathStartTime = 0;  //reset for next death

                int livesNow = sonic->GetLives();

                if (livesNow <= 0) {
                    currentScreen = GameScreen::GameOver;
                    gameOverStartTime = currentTime;  //start input delay timer
                    SoundManager::Instance().OnGameOver();
                } else {
//respawn at checkpoint
                    float respawnX, respawnY;
                    checkpointManager.GetRespawnPosition(respawnX, respawnY);

//CRITICAL: Respawn collectibles and enemies (per classic Sonic behavior)
//monitors/springs/spikes/checkpoints persist (don't respawn)
                    ringManager.Clear();
                    enemyManager.Clear();
                    AnimalManager::Instance().Clear();
                    flowerManager.Clear();
                    bigRingManager.Clear();
                    PlaceGameObjects(false);  //respawn rings/enemies/flowers/big rings ONLY

                    sonic->Respawn(respawnX, respawnY);
                    gameHUD.rings = 0;  //reset rings on death
                }
            }
        }
        return;  //skip ALL collision/damage checks
    }
    
    if (sonic && !GetGame().IsPaused() && !levelComplete) {
        sonic->Update();
        whirlCircleManager.Update();  //check for whirl circle entry and teleports
        UpdateSonicAnimation();

        gameHUD.Update(currentTime);
        gameHUD.rings = sonic->GetRings();
//NOTE: lives removed - HUD.Draw() gets lives directly from sonic pointer
        gameHUD.score = sonic->GetScore();
        
        ringManager.Update();
        
        Rect playerBox = sonic->GetBoundingBox();
        int collected = ringManager.CheckCollision(playerBox);
        if (collected > 0) {
            sonic->AddRings(collected);
        }
        
        enemyManager.Update(sonic->GetX(), sonic->GetY(), viewWindow);
        
        AnimalManager::Instance().Update();
        
        bool inBall = sonic->IsBallState();
        bool canKillOnContact = sonic->HasPowerUpInvincibility();  //power-up kills on contact
        bool damageImmune = sonic->IsInvincible();  //any invincibility prevents damage
        bool inPostDamage = sonic->IsInPostDamageInvincibility();  //post-damage can't kill
        
//during post-damage invincibility, ball state should NOT kill enemies
//only power-up invincibility allows killing on contact
        bool canKillInBall = inBall && !inPostDamage;
        
        int result = enemyManager.CheckCollision(playerBox, canKillInBall, canKillOnContact, damageImmune);
        
        if (result > 0) {
            sonic->AddScore(result);
            gameHUD.AddScore(result, sonic->GetX(), sonic->GetY());
        } else if (result < 0 && !sonic->IsDead() && !godModeActive) {
//CRITICAL: Only take damage if not already dead and not in god mode
            sonic->TakeDamage();
        }
        
        springManager.Update(currentTime);
        float bounceVelX = 0, bounceVelY = 0;
        if (springManager.CheckCollision(playerBox, bounceVelX, bounceVelY, currentTime)) {
            SoundManager::Instance().PlaySound(SoundEffect::Spring, 1.5f);  //150% volume boost
            sonic->ApplyBounce(bounceVelX, bounceVelY);
            std::cout << "Spring bounce!" << std::endl;
        }
        
//check spikes (only damage if not invincible AND not dead AND not god mode)
        if (!sonic->IsInvincible() && !sonic->IsDead() && !godModeActive) {
            float sonicSpeed = sonic->GetSpeed();
            float sonicVelY = sonic->GetVelY();
            if (spikeManager.CheckCollision(playerBox, sonicSpeed, sonicVelY)) {
                sonic->TakeDamage();
                std::cout << "Spike damage! velY=" << sonicVelY << " speed=" << sonicSpeed << std::endl;
            }
        }
        
        checkpointManager.Update(currentTime);
        if (checkpointManager.CheckCollision(playerBox, currentTime)) {
            std::cout << "Checkpoint activated!" << std::endl;
            SoundManager::Instance().OnCheckpoint();
        }
        
        monitorManager.Update(currentTime);
        flowerManager.Update(currentTime);
        bigRingManager.Update(currentTime);
        float sonicVelY = sonic->GetVelY();
        int monitorResult = monitorManager.CheckCollision(playerBox, inBall, sonicVelY, currentTime);
        if (monitorResult >= 0) {
            MonitorType type = static_cast<MonitorType>(monitorResult);
            switch (type) {
                case MonitorType::Ring:
                    sonic->AddRings(10);
                    SoundManager::Instance().OnRingCollect();
                    std::cout << "+10 Rings!" << std::endl;
                    break;
                case MonitorType::Shield:
                    sonic->GiveShield(30000);  //30 seconds
                    SoundManager::Instance().OnRingCollect();
                    std::cout << "Shield!" << std::endl;
                    break;
                case MonitorType::Invincibility:
                    sonic->GiveInvincibility(20000);  //20 seconds
                    SoundManager::Instance().OnRingCollect();
                    std::cout << "Invincibility!" << std::endl;
                    break;
                case MonitorType::SpeedShoes:
                    sonic->GiveSpeedBoost(10000);  //10 seconds
                    SoundManager::Instance().OnRingCollect();
                    std::cout << "Speed Shoes!" << std::endl;
                    break;
                case MonitorType::ExtraLife:
                    sonic->AddLife();
                    gameHUD.GainLife();
                    SoundManager::Instance().OnRingCollect();
                    std::cout << "Extra Life!" << std::endl;
                    break;
                case MonitorType::Eggman:
                    SoundManager::Instance().OnMonitorBreak();
                    if (!godModeActive) {
                        sonic->TakeDamage();
                        std::cout << "Eggman trap!" << std::endl;
                    } else {
                        std::cout << "Eggman trap blocked by god mode!" << std::endl;
                    }
                    break;
            }
            gameHUD.AddScore(100, sonic->GetX(), sonic->GetY());
        }
        
//check big ring collision - level complete!
        if (!levelComplete && bigRingManager.CheckCollision(playerBox)) {
            levelComplete = true;
            levelCompleteTime = currentTime;
            levelCompleteSelection = 0;
            SoundManager::Instance().OnActClear();
//add bonus score for rings
            gameHUD.AddScore(sonic->GetRings() * 100, sonic->GetX(), sonic->GetY());
        }
        
//check for death - touching GRID_DEATH tiles (dark red tiles)
//this replaces the old height-based void check
//god mode: can walk on death tiles without dying
        if (!sonic->IsDead() && gridLayer && !godModeActive) {
            engine::Rect playerBox = sonic->GetBoundingBox();
            if (gridLayer->IsInDeath(playerBox)) {
                std::cout << "DEATH TILE at (" << sonic->GetX() << ", " << sonic->GetY() << ")" << std::endl;
                sonic->Die();
            }
        }
        
//NOTE: Death state handling is now at the TOP of Physics() to ensure
//animation updates happen even when dead
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
                switch (tile) {
                    case GRID_SOLID:    c = MakeColor(255, 255, 255, 180); break;  //white - solid
                    case GRID_PLATFORM: c = MakeColor(0, 255, 0, 160); break;  //green - platform
                    case GRID_SLOPE:    c = MakeColor(255, 255, 0, 160); break;  //yellow - slope
                    case GRID_LOOP:     c = MakeColor(255, 165, 0, 160); break;  //orange - loop
                    case GRID_TUNNEL:   c = MakeColor(0, 255, 255, 160); break;  //cyan - tunnel
                    case GRID_DEATH:    c = MakeColor(139, 0, 0, 180); break;  //dark red - death
                    default:            c = MakeColor(128, 128, 128, 100); break;  //gray - unknown
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
    
//skip if completely off screen
    if (screenX + box.w < 0 || screenX > SCREEN_WIDTH ||
        screenY + box.h < 0 || screenY > SCREEN_HEIGHT) {
        return;
    }
    
//draw collision box when grid view is enabled (magenta outline only)
    if (showGrid) {
        gfx.DrawRect({screenX, screenY, box.w, box.h}, MakeColor(255, 0, 255), false);
    }
    
//center position for effects
    int centerX = screenX + box.w / 2;
    int centerY = screenY + box.h / 2;
    
//draw Shield (smooth rotation animation around Sonic)
    if (sonic->HasShield() && assetsLoaded) {
        auto* shieldFilm = ResourceManager::Instance().GetFilm("shield");
        if (shieldFilm && shieldFilm->GetTotalFrames() > 0) {
            uint64_t now = engine::GetSystemTime();
            int shieldFrame = (now / 100) % shieldFilm->GetTotalFrames();
            float pulsePhase = (now % 2000) / 2000.0f;
            float pulseScale = SPRITE_SCALE * (1.0f + std::sin(pulsePhase * 3.14159f * 2.0f) * 0.03f);
            int baseOffsetX = static_cast<int>(23.5f * pulseScale);
            int baseOffsetY = static_cast<int>(23.5f * pulseScale);
            int shieldX = centerX - baseOffsetX;
            int shieldY = centerY - baseOffsetY;
            shieldFilm->DisplayFrameScaled({shieldX, shieldY}, static_cast<byte>(shieldFrame), SPRITE_SCALE);
        }
    }
    
//draw Sonic sprite SCALED UP to match world size
    if (assetsLoaded) {
        auto* film = ResourceManager::Instance().GetFilm(currentAnim);
        if (film && film->GetTotalFrames() > 0) {
            int frameIdx = sonicFrame % film->GetTotalFrames();
            Rect frameBox = film->GetFrameBox(static_cast<byte>(frameIdx));
            
//calculate scaled dimensions
            int scaledW = static_cast<int>(frameBox.w * SPRITE_SCALE);
            int scaledH = static_cast<int>(frameBox.h * SPRITE_SCALE);
            
//center sprite horizontally on collision box, align bottom
            int drawX = screenX + (box.w - scaledW) / 2;
            int drawY = screenY + box.h - scaledH;
            
//flip sprite based on facing direction
            bool flipX = (sonic->GetFacing() == FacingDirection::Left);
            
            if (flipX) {
                film->DisplayFrameScaledFlipped({drawX, drawY}, static_cast<byte>(frameIdx), SPRITE_SCALE);
            } else {
                film->DisplayFrameScaled({drawX, drawY}, static_cast<byte>(frameIdx), SPRITE_SCALE);
            }
        }
    } else {
//simple fallback - just draw a blue rectangle if sprites fail
        gfx.DrawRect({screenX + 2, screenY + 2, box.w - 4, box.h - 4}, MakeColor(0, 80, 200), true);
        gfx.DrawRect({screenX + 2, screenY + 2, box.w - 4, box.h - 4}, MakeColor(0, 40, 160), false);
    }
    
//draw Invincibility sparkles (stars rotating around Sonic)
    if (sonic->IsInvincible() && assetsLoaded) {
        uint64_t now = engine::GetSystemTime();
        auto* sparkleFilm = ResourceManager::Instance().GetFilm("invincibility");
        if (sparkleFilm && sparkleFilm->GetTotalFrames() > 0) {
            if (now - sparkleEffect.lastUpdate > 25) {
                sparkleEffect.angle += 0.12f;
                sparkleEffect.lastUpdate = now;
            }
            
//draw sparkles in an orbit around Sonic - scaled up
            for (int i = 0; i < sparkleEffect.numSparkles; i++) {
                float baseAngle = sparkleEffect.angle + (i * 3.14159f * 2.0f / sparkleEffect.numSparkles);
                float vertOffset = std::sin(baseAngle * 2.0f) * 5.0f * SPRITE_SCALE;
                
//vary radius slightly per sparkle for depth
                float radius = sparkleEffect.radius + (i % 2 == 0 ? 5.0f : -5.0f);
                
                int starX = centerX + static_cast<int>(std::cos(baseAngle) * radius) - 8;
                int starY = centerY + static_cast<int>(std::sin(baseAngle) * radius * 0.7f + vertOffset) - 8;
                
//cycle through frames, but offset each sparkle so they're not all the same
                int frameIdx = (now / 60 + i * 2) % sparkleFilm->GetTotalFrames();
                sparkleFilm->DisplayFrame({starX, starY}, static_cast<byte>(frameIdx));
            }
            
//removed subtle glow/halo effect to eliminate transparent box during invincibility
        }
    }
}

void DrawSprings() {
    auto& gfx = GetGraphics();
    
    auto* yellowSpringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("spring_yellow") : nullptr;
    auto* redSpringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("spring_red") : nullptr;
    auto* sideSpringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("spring_side") : nullptr;
    
//scaled heights for 1280x896 resolution
    static const int NORMAL_HEIGHT = static_cast<int>(16 * SPRITE_SCALE);
    static const int EXTENDED_HEIGHT = static_cast<int>(32 * SPRITE_SCALE);
    static const int HEIGHT_DIFF = EXTENDED_HEIGHT - NORMAL_HEIGHT;
    
    for (const auto& spring : springManager.GetSprings()) {
        int screenX = static_cast<int>(spring.x) - viewWindow.x;
        int screenY = static_cast<int>(spring.y) - viewWindow.y;
        
//scale spring dimensions for collision check
        int scaledW = static_cast<int>(spring.width * SPRITE_SCALE);
        int scaledH = static_cast<int>(spring.height * SPRITE_SCALE);
        
        if (screenX + scaledW < 0 || screenX > SCREEN_WIDTH ||
            screenY + scaledH < 0 || screenY > SCREEN_HEIGHT) continue;
        
        int frameIdx = spring.GetAnimFrameIndex();
        
//use side spring for diagonal directions
        bool isDiagonal = (spring.direction == SpringDirection::DiagonalUpLeft || 
                          spring.direction == SpringDirection::DiagonalUpRight ||
                          spring.direction == SpringDirection::Left ||
                          spring.direction == SpringDirection::Right);
        
        if (isDiagonal && sideSpringFilm && sideSpringFilm->GetTotalFrames() >= 2) {
//side spring: frame 0 = normal, frame 1 = extended
            int sideFrame = (frameIdx == 2) ? 1 : 0;
            
//flip horizontally for left-facing springs
            if (spring.direction == SpringDirection::DiagonalUpLeft || 
                spring.direction == SpringDirection::Left) {
                sideSpringFilm->DisplayFrameScaledFlipped({screenX, screenY}, 
                    static_cast<byte>(sideFrame), SPRITE_SCALE);
            } else {
                sideSpringFilm->DisplayFrameScaled({screenX, screenY}, 
                    static_cast<byte>(sideFrame), SPRITE_SCALE);
            }
        } else {
//vertical springs (Up)
            auto* springFilm = spring.isYellow ? yellowSpringFilm : redSpringFilm;
            
            if (springFilm && springFilm->GetTotalFrames() >= 3) {
                int drawY = screenY;
                if (frameIdx == 2) {
                    drawY = screenY - HEIGHT_DIFF;
                }
                springFilm->DisplayFrameScaled({screenX, drawY}, static_cast<byte>(frameIdx), SPRITE_SCALE);
            } else {
//fallback placeholder
                Color springColor = spring.isYellow ? MakeColor(255, 215, 0) : MakeColor(255, 50, 50);
                gfx.DrawRect({screenX, screenY, scaledW, scaledH}, springColor, true);
            }
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
        
//get spike sprite frame from misc sheet
        bool drewSprite = false;
        if (assetsLoaded) {
            auto& rm = ResourceManager::Instance();
            auto miscBmp = rm.GetMiscSheet();
            if (miscBmp) {
                auto frame = MiscSpriteConfig::GetSpikesUp();
                gfx.DrawTexture(miscBmp->GetTexture(), 
                               {frame.x, frame.y, frame.w, frame.h},
                               {screenX, screenY});
                drewSprite = true;
            }
        }
        
//fallback placeholder only if sprite didn't load
        if (!drewSprite) {
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
        }
        
//debug: draw hitbox when grid is shown
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
    
//get checkpoint film if loaded
    auto* checkpointFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("checkpoint") : nullptr;
    
    for (const auto& cp : checkpointManager.GetCheckpoints()) {
        int screenX = static_cast<int>(cp.x) - viewWindow.x;
        int screenY = static_cast<int>(cp.y) - viewWindow.y;
        
        if (screenX + cp.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + cp.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        if (checkpointFilm && checkpointFilm->GetTotalFrames() > 0) {
//use animated frames based on state
            int frameIdx = cp.activated ? (cp.animFrame % checkpointFilm->GetTotalFrames()) : 0;
            checkpointFilm->DisplayFrame({screenX, screenY - 32}, static_cast<byte>(frameIdx));
        } else {
//fallback placeholder
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
        int screenX = static_cast<int>(monitor.x) - viewWindow.x;
        int screenY = static_cast<int>(monitor.y) - viewWindow.y;
        
        if (screenX + monitor.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + monitor.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        if (miscBmp) {
            if (monitor.state == MonitorState::Breaking || monitor.state == MonitorState::Destroyed) {
//draw broken box debris (shifted down to align at bottom)
//broken sprite is 32x16, original is 32x32, so shift down by 16
                auto frame = MiscSpriteConfig::GetMonitorBroken();
                gfx.DrawTexture(miscBmp->GetTexture(),
                               {frame.x, frame.y, frame.w, frame.h},
                               {screenX, screenY + 16});
            } else {
//draw unbroken monitor box
                auto boxFrame = MiscSpriteConfig::GetMonitorUnbroken();
                gfx.DrawTexture(miscBmp->GetTexture(),
                               {boxFrame.x, boxFrame.y, boxFrame.w, boxFrame.h},
                               {screenX, screenY});
                
//draw icon on top (centered, always visible when not broken)
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
//center icon in monitor
                int iconX = screenX + (monitor.width - iconFrame.w) / 2;
                int iconY = screenY + (monitor.height - iconFrame.h) / 2 - 2;
                gfx.DrawTexture(miscBmp->GetTexture(),
                               {iconFrame.x, iconFrame.y, iconFrame.w, iconFrame.h},
                               {iconX, iconY});
            }
        } else {
//fallback placeholder
            if (monitor.state == MonitorState::Breaking) {
                gfx.DrawRect({screenX - 5, screenY - 5, monitor.width + 10, monitor.height + 10}, 
                             MakeColor(255, 200, 100), true);
            } else {
                gfx.DrawRect({screenX, screenY, monitor.width, monitor.height}, 
                             MakeColor(80, 80, 80), true);
                gfx.DrawRect({screenX + 3, screenY + 3, monitor.width - 6, monitor.height - 10}, 
                             MakeColor(50, 80, 120), true);
//always show icon when not broken
                Color iconColor = monitor.GetIconColor();
                int iconSize = 12;
                int iconX = screenX + monitor.width/2 - iconSize/2;
                int iconY = screenY + monitor.height/2 - iconSize/2 - 2;
                gfx.DrawRect({iconX, iconY, iconSize, iconSize}, iconColor, true);
            }
        }
        
//power-up popup (flying icon)
        if (monitor.showPopup) {
            int popupY = static_cast<int>(monitor.popupY) - viewWindow.y;

            if (miscBmp) {
//get the same icon sprite as shown in the monitor
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
//use original sprite size
                float iconScale = 1.0f;
                int scaledW = static_cast<int>(iconFrame.w * iconScale);
//center popup icon horizontally over monitor
                int popupX = screenX + (monitor.width - scaledW) / 2;

//draw scaled icon using sprite
                sf::Sprite iconSprite(miscBmp->GetTexture());
                iconSprite.setTextureRect(sf::IntRect(iconFrame.x, iconFrame.y, iconFrame.w, iconFrame.h));
                iconSprite.setScale(iconScale, iconScale);
                iconSprite.setPosition(static_cast<float>(popupX), static_cast<float>(popupY));
                gfx.DrawSprite(iconSprite);
            } else {
//fallback placeholder (larger size)
                int iconSize = 24;  //1.5x larger than default 16
                int popupX = screenX + monitor.width/2 - iconSize/2;
                Color iconColor = monitor.GetIconColor();
                gfx.DrawRect({popupX, popupY, iconSize, iconSize}, iconColor, true);
                gfx.DrawRect({popupX, popupY, iconSize, iconSize}, MakeColor(255, 255, 255), false);
            }
        }
    }
}

//draw animated flowers (decorative background elements from tiles sheet)
void DrawFlowers() {
    if (!assetsLoaded) return;
    
    auto* flowerTallFilm = ResourceManager::Instance().GetFilm("flower_tall");
    auto* flowerShortFilm = ResourceManager::Instance().GetFilm("flower_short");
    
//offset to align flower sprites with stems drawn in tiles
    constexpr int FLOWER_OFFSET_X = -15;  //slightly left to center on stem
    constexpr int FLOWER_OFFSET_Y = -20;  //move up to sit on stem
    
    for (const auto& flower : flowerManager.GetFlowers()) {
        int screenX = static_cast<int>(flower.x) - viewWindow.x + FLOWER_OFFSET_X;
        int screenY = static_cast<int>(flower.y) - viewWindow.y + FLOWER_OFFSET_Y;
        
//skip if off screen
        if (screenX + flower.width < 0 || screenX > SCREEN_WIDTH ||
            screenY + flower.height < 0 || screenY > SCREEN_HEIGHT) continue;
        
        auto* film = (flower.type == FlowerType::Tall) ? flowerTallFilm : flowerShortFilm;
        if (film && film->GetTotalFrames() > 0) {
            int frameIdx = flower.animFrame % film->GetTotalFrames();
            film->DisplayFrame({screenX, screenY}, static_cast<byte>(frameIdx));
        }
    }
}

void DrawBigRings() {
    if (!assetsLoaded) return;
    
    auto* bigRingFilm = ResourceManager::Instance().GetFilm("big_ring");
    
//frame widths for centering (frames have different widths)
    static const int frameWidths[] = {63, 47, 23, 47};
    static const int maxWidth = 63;
    
    for (const auto& ring : bigRingManager.GetRings()) {
        if (!ring.active || ring.collected) continue;
        
        int screenX = static_cast<int>(ring.x) - viewWindow.x;
        int screenY = static_cast<int>(ring.y) - viewWindow.y;
        
        int scaledW = static_cast<int>(maxWidth * ring.scale);
        int scaledH = static_cast<int>(ring.height * ring.scale);
        
//skip if off screen
        if (screenX + scaledW < 0 || screenX > SCREEN_WIDTH ||
            screenY + scaledH < 0 || screenY > SCREEN_HEIGHT) continue;
        
        if (bigRingFilm && bigRingFilm->GetTotalFrames() >= 4) {
            int frameIdx = ring.animFrame % 4;
            
//center the frame horizontally (frames have different widths)
            int frameW = frameWidths[frameIdx];
            int offsetX = static_cast<int>((maxWidth - frameW) * ring.scale / 2.0f);
            
            bigRingFilm->DisplayFrameScaled({screenX + offsetX, screenY}, static_cast<engine::byte>(frameIdx), ring.scale);
        } else {
//fallback: gold rectangle
            auto& gfx = GetGraphics();
            gfx.DrawRect({screenX, screenY, scaledW, scaledH}, MakeColor(255, 215, 0), true);
        }
    }
}

void DrawHUD() {
    auto& gfx = GetGraphics();
    
//HUD for 160x112 resolution
    if (sonic) {
//score
        char scoreStr[16];
        snprintf(scoreStr, sizeof(scoreStr), "%06d", sonic->GetScore());
        gfx.DrawText(scoreStr, 2, 1, MakeColor(255, 255, 0));
        
//time
        int minutes = gameHUD.timeSeconds / 60;
        int seconds = gameHUD.timeSeconds % 60;
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%d:%02d", minutes, seconds);
        gfx.DrawText(timeStr, 2, 10, MakeColor(255, 255, 0));
        
//rings
        int rings = sonic->GetRings();
        char ringStr[12];
        snprintf(ringStr, sizeof(ringStr), "R:%02d", rings);
        Color ringColor = (rings == 0 && (GetSystemTime() / 250) % 2 == 0) 
                         ? MakeColor(255, 0, 0) : MakeColor(255, 255, 0);
        gfx.DrawText(ringStr, 2, 19, ringColor);
        
//lives
        char livesStr[8];
        snprintf(livesStr, sizeof(livesStr), "x%d", sonic->GetLives());
        gfx.DrawText(livesStr, 2, SCREEN_HEIGHT - 10, MakeColor(255, 255, 255));
    }
}

void DrawDebugInfo() {
}

//native HUD - draws directly to window at 640x448
void DrawNativeHUD() {
    if (!sonic) return;
    auto& gfx = GetGraphics();
    
//HUD at native window resolution (640x448)
//background box
    gfx.DrawNativeRect({8, 8, 130, 75}, MakeColor(0, 0, 0, 180), true);
    
//SCORE
    gfx.DrawNativeText("SCORE", 12, 12, MakeColor(255, 255, 0));
    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr), "%06d", sonic->GetScore());
    gfx.DrawNativeText(scoreStr, 70, 12, MakeColor(255, 255, 255));
    
//TIME
    gfx.DrawNativeText("TIME", 12, 32, MakeColor(255, 255, 0));
    int minutes = gameHUD.timeSeconds / 60;
    int seconds = gameHUD.timeSeconds % 60;
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%d:%02d", minutes, seconds);
    gfx.DrawNativeText(timeStr, 70, 32, MakeColor(255, 255, 255));
    
//RINGS
    int rings = sonic->GetRings();
    Color ringColor = (rings == 0 && (GetSystemTime() / 250) % 2 == 0) 
                     ? MakeColor(255, 0, 0) : MakeColor(255, 255, 0);
    gfx.DrawNativeText("RINGS", 12, 52, ringColor);
    char ringStr[16];
    snprintf(ringStr, sizeof(ringStr), "%03d", rings);
    gfx.DrawNativeText(ringStr, 70, 52, MakeColor(255, 255, 255));
    
//lives - bottom left (window coords) with Sonic icon
    gfx.DrawNativeRect({8, gfx.GetNativeHeight() - 28, 50, 20}, MakeColor(0, 0, 0, 180), true);

//draw Sonic life icon
    auto miscBmp = assetsLoaded ? ResourceManager::Instance().GetMiscSheet() : nullptr;
    if (miscBmp) {
        FrameRect iconFrame = MiscSpriteConfig::GetIconLife();
        sf::Sprite lifeIcon(miscBmp->GetTexture());
        lifeIcon.setTextureRect(sf::IntRect(iconFrame.x, iconFrame.y, iconFrame.w, iconFrame.h));
        lifeIcon.setScale(1.0f, 1.0f);  //original size (16x16)
        gfx.DrawNativeSprite(lifeIcon, 11, gfx.GetNativeHeight() - 26);
    }

//draw lives count to the right of icon
    char livesStr[16];
    snprintf(livesStr, sizeof(livesStr), "x %d", sonic->GetLives());
    gfx.DrawNativeText(livesStr, 30, gfx.GetNativeHeight() - 24, MakeColor(255, 255, 255));
}

void DrawNativeDebug() {
    if (!sonic) return;
    auto& gfx = GetGraphics();
    
//debug panel at native resolution - top right (native coords)
    int panelX = gfx.GetNativeWidth() - 145;
    int panelH = godModeActive ? 110 : 90;  //taller if god mode active
    gfx.DrawNativeRect({panelX, 8, 135, panelH}, MakeColor(0, 0, 0, 200), true);
    
    const char* stateStr = StateToString(sonic->GetState());
    gfx.DrawNativeText(stateStr, panelX + 5, 12, MakeColor(255, 255, 0));
    
    char posStr[32];
    snprintf(posStr, sizeof(posStr), "X:%.0f Y:%.0f", sonic->GetX(), sonic->GetY());
    gfx.DrawNativeText(posStr, panelX + 5, 32, MakeColor(200, 200, 200));
    
    char velStr[32];
    snprintf(velStr, sizeof(velStr), "VX:%.1f VY:%.1f", sonic->GetVelX(), sonic->GetVelY());
    gfx.DrawNativeText(velStr, panelX + 5, 52, MakeColor(200, 200, 200));
    
    char spdStr[24];
    snprintf(spdStr, sizeof(spdStr), "Speed:%.2f", sonic->GetSpeed());
    gfx.DrawNativeText(spdStr, panelX + 5, 72, MakeColor(0, 255, 0));
    
//god mode indicator
    if (godModeActive) {
        gfx.DrawNativeText("GOD MODE", panelX + 5, 92, MakeColor(255, 0, 0));
    }
}

void DrawNativePauseMenu() {
    auto& gfx = GetGraphics();

//native coordinates are scaled based on current window size
    int gameW = gfx.GetNativeWidth();   //virtualWidth * scale
    int gameH = gfx.GetNativeHeight();  //virtualHeight * scale

//pause box - centered in the native coordinate space
    int boxW = 280;
    int boxH = 220;
    int boxX = (gameW - boxW) / 2;
    int boxY = (gameH - boxH) / 2;
    
//background with double border
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(0, 40, 120, 245), true);
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(255, 215, 0), false);
    gfx.DrawNativeRect({boxX + 3, boxY + 3, boxW - 6, boxH - 6}, MakeColor(255, 215, 0), false);
    
//title - centered
    gfx.DrawNativeText("PAUSED", boxX + (boxW - 70) / 2, boxY + 20, MakeColor(255, 215, 0));
    
    const char* menuItems[] = {"CONTINUE", "RESTART", "QUIT"};
    int buttonW = 200;
    int buttonH = 35;
    int buttonX = boxX + (boxW - buttonW) / 2;
    int itemY = boxY + 60;
    
    for (int i = 0; i < PAUSE_MENU_ITEMS; ++i) {
        Color bgColor = (i == pauseMenuSelection) ? MakeColor(255, 215, 0) : MakeColor(60, 60, 120);
        Color textColor = (i == pauseMenuSelection) ? MakeColor(0, 0, 80) : MakeColor(255, 255, 255);
        
        gfx.DrawNativeRect({buttonX, itemY, buttonW, buttonH}, bgColor, true);
        gfx.DrawNativeRect({buttonX, itemY, buttonW, buttonH}, MakeColor(255, 255, 255), false);
        
        if (i == pauseMenuSelection) {
            gfx.DrawNativeText(">", buttonX - 18, itemY + 10, MakeColor(255, 255, 0));
        }
        
//center text in button
        int textX = buttonX + (buttonW - 80) / 2;
        gfx.DrawNativeText(menuItems[i], textX, itemY + 10, textColor);
        itemY += 45;
    }
    
    gfx.DrawNativeText("ENTER = Select", boxX + (boxW - 130) / 2, boxY + boxH - 25, MakeColor(180, 180, 180));
}

void DrawLevelCompleteScreen() {
    auto& gfx = GetGraphics();

//native coordinates are scaled based on current window size
    int centerX = gfx.GetNativeWidth() / 2;
    int centerY = gfx.GetNativeHeight() / 2;

//level complete box - fixed dimensions for better proportions
    int boxW = 480;
    int boxH = 400;
    int boxX = centerX - boxW / 2;
    int boxY = centerY - boxH / 2;
    
//background with triple border
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(0, 40, 120, 250), true);
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(255, 215, 0), false);
    gfx.DrawNativeRect({boxX + 3, boxY + 3, boxW - 6, boxH - 6}, MakeColor(255, 215, 0), false);
    gfx.DrawNativeRect({boxX + 6, boxY + 6, boxW - 12, boxH - 12}, MakeColor(100, 150, 255), false);
    
//title - centered
    gfx.DrawNativeText("LEVEL COMPLETE!", boxX + (boxW - 160) / 2, boxY + 25, MakeColor(255, 215, 0));
    gfx.DrawNativeText("CONGRATULATIONS!", boxX + (boxW - 170) / 2, boxY + 50, MakeColor(255, 255, 255));
    
//stats - centered
    if (sonic) {
        int statsX = boxX + (boxW - 140) / 2;
        
        char scoreStr[32];
        snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", sonic->GetScore());
        gfx.DrawNativeText(scoreStr, statsX, boxY + 90, MakeColor(255, 255, 255));
        
        char ringStr[32];
        snprintf(ringStr, sizeof(ringStr), "RINGS: %d", sonic->GetRings());
        gfx.DrawNativeText(ringStr, statsX, boxY + 115, MakeColor(255, 255, 255));
        
        char timeStr[32];
        int minutes = gameHUD.timeSeconds / 60;
        int seconds = gameHUD.timeSeconds % 60;
        snprintf(timeStr, sizeof(timeStr), "TIME: %d:%02d", minutes, seconds);
        gfx.DrawNativeText(timeStr, statsX, boxY + 140, MakeColor(255, 255, 255));
        
//ring bonus - highlighted
        char bonusStr[32];
        snprintf(bonusStr, sizeof(bonusStr), "RING BONUS: %d", sonic->GetRings() * 100);
        gfx.DrawNativeText(bonusStr, statsX - 25, boxY + 170, MakeColor(255, 215, 0));
    }
    
//menu options - bigger buttons, centered
    const char* menuItems[] = {"RESTART LEVEL", "LAST CHECKPOINT"};
    int buttonW = 240;
    int buttonH = 38;
    int buttonX = boxX + (boxW - buttonW) / 2;
    int itemY = boxY + 210;
    
    for (int i = 0; i < 2; ++i) {
        Color bgColor = (i == levelCompleteSelection) ? MakeColor(255, 215, 0) : MakeColor(60, 60, 120);
        Color textColor = (i == levelCompleteSelection) ? MakeColor(0, 0, 80) : MakeColor(255, 255, 255);
        
        gfx.DrawNativeRect({buttonX, itemY, buttonW, buttonH}, bgColor, true);
        gfx.DrawNativeRect({buttonX, itemY, buttonW, buttonH}, MakeColor(255, 255, 255), false);
        
        if (i == levelCompleteSelection) {
            gfx.DrawNativeText(">", buttonX - 18, itemY + 11, MakeColor(255, 255, 0));
        }
        
//center text in button
        int textX = buttonX + (buttonW - 140) / 2;
        gfx.DrawNativeText(menuItems[i], textX, itemY + 11, textColor);
        itemY += 48;
    }
    
    gfx.DrawNativeText("UP/DOWN + ENTER", boxX + (boxW - 150) / 2, boxY + boxH - 22, MakeColor(180, 180, 180));
}

void RenderTitleScreen() {
    auto& gfx = GetGraphics();
    
//clear with blue background (will be scaled)
    gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 80, 180), true);
    
//present the blue background
    gfx.Present();
    
//now draw title text at native resolution (scaled)
//all UI centered in window
    int centerX = gfx.GetNativeWidth() / 2;
    int centerY = gfx.GetNativeHeight() / 2;

//title box - fixed width for better proportions
    int titleW = 500;
    int titleH = 90;
    int titleX = centerX - titleW / 2;
    int titleY = centerY - 180;

    gfx.DrawNativeRect({titleX, titleY, titleW, titleH}, MakeColor(0, 0, 60, 230), true);
    gfx.DrawNativeRect({titleX, titleY, titleW, titleH}, MakeColor(255, 200, 0), false);
    gfx.DrawNativeText("SONIC THE HEDGEHOG", centerX - 95, titleY + 20, MakeColor(255, 215, 0));
    gfx.DrawNativeText("The CSD Chronicles", centerX - 80, titleY + 55, MakeColor(200, 200, 255));

//info box
    int infoY = titleY + titleH + 20;
    int infoH = 100;
    gfx.DrawNativeRect({titleX, infoY, titleW, infoH}, MakeColor(0, 0, 0, 200), true);
    gfx.DrawNativeText("University of Crete", centerX - 80, infoY + 20, MakeColor(255, 255, 255));
    gfx.DrawNativeText("CS-454 Fall 2024", centerX - 70, infoY + 50, MakeColor(200, 200, 200));
    gfx.DrawNativeText("NIKOLAOS KANAKOUSAKIS", centerX - 100, infoY + 75, MakeColor(255, 255, 0));

//menu buttons
    int menuW = 320;
    int menuX = centerX - menuW / 2;
    int menuY = infoY + infoH + 25;
    
    int buttonH = 50;
    int buttonSpacing = 18;

    bool startSelected = (titleMenuSelection == 0);
    gfx.DrawNativeRect({menuX, menuY, menuW, buttonH}, startSelected ? MakeColor(0, 100, 200) : MakeColor(0, 0, 80, 200), true);
    gfx.DrawNativeRect({menuX, menuY, menuW, buttonH}, startSelected ? MakeColor(255, 255, 0) : MakeColor(100, 100, 100), false);
    gfx.DrawNativeText("START GAME", centerX - 50, menuY + 13, startSelected ? MakeColor(255, 255, 0) : MakeColor(200, 200, 200));

    bool soundSelected = (titleMenuSelection == 1);
    int soundY = menuY + buttonH + buttonSpacing;
    gfx.DrawNativeRect({menuX, soundY, menuW, buttonH}, soundSelected ? MakeColor(0, 100, 200) : MakeColor(0, 0, 80, 200), true);
    gfx.DrawNativeRect({menuX, soundY, menuW, buttonH}, soundSelected ? MakeColor(255, 255, 0) : MakeColor(100, 100, 100), false);
    char soundText[32];
    snprintf(soundText, sizeof(soundText), "SOUND: %s", soundEnabled ? "ON" : "OFF");
    gfx.DrawNativeText(soundText, centerX - 45, soundY + 13, soundSelected ? MakeColor(255, 255, 0) : MakeColor(200, 200, 200));

//instructions - positioned at bottom of screen
    int instructY = centerY + 200;  //below centered content
    gfx.DrawNativeText("UP/DOWN + ENTER", centerX - 70, instructY, MakeColor(150, 150, 150));
    
    gfx.FinalDisplay();
}

void RenderGameOverScreen() {
    auto& gfx = GetGraphics();
    
//dark overlay (scaled)
    gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 0, 0, 200), true);
    gfx.Present();
    
//draw at native resolution (scaled)
    int centerX = gfx.GetNativeWidth() / 2;
    int centerY = gfx.GetNativeHeight() / 2;
    int boxW = 380;  //fixed width for better proportions
    int boxH = 240;  //fixed height
    int boxX = centerX - boxW / 2;
    int boxY = centerY - boxH / 2;
    
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(100, 0, 0, 240), true);
    gfx.DrawNativeRect({boxX, boxY, boxW, boxH}, MakeColor(255, 0, 0), false);

    gfx.DrawNativeText("GAME OVER", centerX - 45, boxY + boxH / 6, MakeColor(255, 100, 100));

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", gameHUD.score);
    gfx.DrawNativeText(scoreStr, centerX - 50, centerY - 20, MakeColor(255, 255, 0));

    int minutes = gameHUD.timeSeconds / 60;
    int seconds = gameHUD.timeSeconds % 60;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "TIME: %d:%02d", minutes, seconds);
    gfx.DrawNativeText(timeStr, centerX - 50, centerY + 10, MakeColor(255, 215, 0));

    uint64_t now = GetSystemTime();
    bool inputReady = (gameOverStartTime > 0 && now - gameOverStartTime > 1000);

    if (inputReady && (now / 500) % 2 == 0) {
        gfx.DrawNativeText("ENTER = RESTART", centerX - 70, boxY + boxH - 60, MakeColor(255, 255, 0));
    }
    gfx.DrawNativeText("Q = QUIT", centerX - 35, boxY + boxH - 30, MakeColor(150, 150, 150));
    
    gfx.FinalDisplay();
}

void Render() {
    static bool shownTerrainInfo = false;  //only show once
    auto& gfx = GetGraphics();

//CRITICAL: Clear backbuffer at start of EVERY frame to prevent ghosting
    gfx.Clear(MakeColor(0, 156, 252));  //clear with sky blue

//handle different screens
    if (currentScreen == GameScreen::Title) {
        gfx.Clear();  //title screen uses different color
        RenderTitleScreen();
        return;
    }

    if (currentScreen == GameScreen::GameOver) {
        gfx.Clear();  //game over overlays on game state
        RenderGameOverScreen();
        return;
    }

//draw parallax background (will overdraw the clear color)
    parallaxManager.Draw(gfx, viewWindow.x, viewWindow.y, SCREEN_WIDTH, SCREEN_HEIGHT);
    
//draw terrain - prefer terrain image over tileset
    extern engine::BitmapPtr terrainImage;
    
    if (terrainImage) {
//draw portion of terrain image matching the view window
        Rect srcRect = {viewWindow.x, viewWindow.y, SCREEN_WIDTH, SCREEN_HEIGHT};
        gfx.DrawTexture(terrainImage->GetTexture(), srcRect, {0, 0});
    } else if (actionLayer && actionLayer->GetTileSet()) {
//FALLBACK: Using tile-based rendering (will show borders!)
        if (!shownTerrainInfo) {
            std::cerr << "⚠ WARNING: Using tile-based rendering (tiles have borders!)" << std::endl;
            std::cerr << "⚠ sonic_terrain.png not loaded - you'll see magenta lines" << std::endl;
            shownTerrainInfo = true;
        }
        
        actionLayer->SetViewWindow(viewWindow);
        actionLayer->Display({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});
    } else {
        if (!shownTerrainInfo) {
            std::cerr << "✗ ERROR: No terrain rendering available!" << std::endl;
            shownTerrainInfo = true;
        }
    }
    
//draw grid overlay ON TOP of tiles when enabled (G key)
    if (showGrid) {
        DrawGridOverlay();
    }
    
//draw game objects (order matters for layering)
    DrawSprings();
    DrawCheckpoints();
    DrawFlowers();  //background decorations first
    DrawMonitors();  //powerups
    DrawSpikes();  //spikes on top of flowers and powerups
    DrawBigRings();
    
//draw rings and enemies
    auto* ringFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("ring") : nullptr;
    auto* ringCollectFilm = assetsLoaded ? ResourceManager::Instance().GetFilm("ring_collect") : nullptr;
    ringManager.Render(viewWindow, ringFilm, ringCollectFilm, SPRITE_SCALE);
    enemyManager.Render(viewWindow);
    AnimalManager::Instance().Render(viewWindow);
    
//draw player
    DrawSonicSprite();
    
//pause darkening (will be drawn native after Present)
    if (gamePaused) {
        gfx.DrawRect({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, MakeColor(0, 0, 0, 128), true);
    }

//present game graphics (scaled from virtual res to window)
    gfx.Present();
    
//draw HUD at native resolution (crisp, not scaled)
    DrawNativeHUD();
    if (showDebug) DrawNativeDebug();
    
//draw pause menu at native resolution if paused
    if (gamePaused) {
        DrawNativePauseMenu();
    }
    
//draw level complete screen
    if (levelComplete) {
        DrawLevelCompleteScreen();
    }
    
    gfx.FinalDisplay();
}

//note: Collision grid loading moved to GridLayer::LoadCSV

//helper function to find ground Y at a given X position by sampling the grid
//returns the Y pixel position of the ground surface, or -1 if no ground (pit)
int FindGroundY(int pixelX) {
    if (!gridLayer) return 1200;  //fallback near bottom
    
    int gridCol = pixelX / GRID_ELEMENT_WIDTH;
    if (gridCol < 0 || gridCol >= static_cast<int>(gridLayer->GetCols())) return -1;
    
//debug: print first few non-empty tiles in this column
    static bool debugPrinted = false;
    if (!debugPrinted && pixelX == 100) {
        std::cout << "DEBUG FindGroundY at X=" << pixelX << " (col " << gridCol << "):" << std::endl;
        for (int row = 0; row < static_cast<int>(gridLayer->GetRows()); ++row) {
            GridIndex tile = gridLayer->GetTile(static_cast<Dim>(gridCol), static_cast<Dim>(row));
            if (tile != GRID_EMPTY) {
                std::cout << "  Row " << row << " (Y=" << (row * GRID_ELEMENT_HEIGHT) << "): tile=" << static_cast<int>(tile) << std::endl;
            }
        }
        debugPrinted = true;
    }
    
//scan from top to find first ground tile (solid, platform, or slope)
    for (int row = 0; row < static_cast<int>(gridLayer->GetRows()); ++row) {
        GridIndex tile = gridLayer->GetTile(static_cast<Dim>(gridCol), static_cast<Dim>(row));
//new collision types: GRID_SOLID, GRID_PLATFORM, GRID_SLOPE are all ground
        if (tile == GRID_SOLID || tile == GRID_PLATFORM || tile == GRID_SLOPE) {
            return row * GRID_ELEMENT_HEIGHT;
        }
    }
    return -1;  //no ground found (pit)
}

//places all game objects on the terrain
//uses FindGroundY to place objects at correct heights based on actual collision grid
void PlaceGameObjects(bool includeMonitorsAndLevelObjects) {
//NEW TERRAIN: 9984×1536 pixels (39 tiles × 6 tiles at 256×256)
//grid: 1248×192 cells at 8×8 pixels per cell
//ground level varies significantly - use FindGroundY for placement

    std::cout << "Placing game objects (monitors/level objects=" << (includeMonitorsAndLevelObjects ? "yes" : "no") << ")..." << std::endl;
    
//=== RINGS ===
//ring trails along ground - place 20px above ground level
    auto placeRingTrail = [](float startX, int count, float spacing) {
        for (int i = 0; i < count; ++i) {
            float x = startX + i * spacing;
            int groundY = FindGroundY(static_cast<int>(x));
            if (groundY > 0) {
                ringManager.AddRing(x, static_cast<float>(groundY - 30));
            }
        }
    };
    
    placeRingTrail(150.0f, 8, 40.0f);
    
//trail 2: After first hill
    placeRingTrail(550.0f, 6, 35.0f);
    
//trail 3: Mid-level section
    placeRingTrail(900.0f, 5, 40.0f);
    
//trail 4: Before loop area
    placeRingTrail(1350.0f, 5, 40.0f);
    
//trail 5: After pit
    placeRingTrail(1600.0f, 8, 45.0f);
    
//trail 6: High platform area
    placeRingTrail(2400.0f, 6, 40.0f);
    
//trail 7: Late section (lower ground)
    placeRingTrail(3400.0f, 8, 50.0f);
    
//trail 8: Near end
    placeRingTrail(4200.0f, 6, 45.0f);
    
//vertical ring arrangements (floating - for jumps)
    int groundAt1000 = FindGroundY(1000);
    if (groundAt1000 > 0) {
        for (int i = 0; i < 4; ++i) {
            ringManager.AddRing(1000.0f, static_cast<float>(groundAt1000 - 50 - i * 35));
        }
    }
    
    int groundAt2800 = FindGroundY(2800);
    if (groundAt2800 > 0) {
        for (int i = 0; i < 3; ++i) {
            ringManager.AddRing(2800.0f, static_cast<float>(groundAt2800 - 50 - i * 35));
        }
    }
    
//=== ENEMIES ===
//place enemies ON ground (their Y = groundY - enemyHeight)
    auto placeMotobug = [](float x) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
            enemyManager.AddMotobug(x, static_cast<float>(groundY - 32));
        }
    };
    
    auto placeCrabmeat = [](float x) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
            enemyManager.AddCrabmeat(x, static_cast<float>(groundY - 32));
        }
    };
    
//motobugs
    placeMotobug(400.0f);
    placeMotobug(750.0f);
    placeMotobug(1500.0f);
    placeMotobug(2000.0f);
    placeMotobug(2600.0f);
    placeMotobug(3600.0f);
    placeMotobug(4300.0f);
    
//crabmeat
    placeCrabmeat(600.0f);
    placeCrabmeat(1100.0f);
    placeCrabmeat(2300.0f);
    placeCrabmeat(3200.0f);
    placeCrabmeat(4000.0f);
    
//buzzBombers (flying - place 150px above ground)
    auto placeBuzzBomber = [](float x, float patrolRange) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
            enemyManager.AddBuzzBomber(x, static_cast<float>(groundY - 150), patrolRange);
        }
    };
    
    placeBuzzBomber(500.0f, 200.0f);
    placeBuzzBomber(1200.0f, 250.0f);
    placeBuzzBomber(1900.0f, 200.0f);
    placeBuzzBomber(2900.0f, 220.0f);
    placeBuzzBomber(3800.0f, 180.0f);
    
//batbrain (hanging enemies - 100px above ground)
    auto placeBatbrain = [](float x) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
            enemyManager.AddBatbrain(x, static_cast<float>(groundY - 120));
        }
    };
    
    placeBatbrain(850.0f);
    placeBatbrain(1700.0f);
    placeBatbrain(2500.0f);
    
//masher (fish enemies - jump from water pits)
//ONLY spawn in actual water areas (Y < 1300)
//y > 1300 is void/death zone, not water!
    
//upper water pit mashers (if these are actual water pits)
//only add if Y < 1300
    if (1050.0f < 1300.0f) {
        enemyManager.AddMasher(2400.0f, 1050.0f);
        enemyManager.AddMasher(2480.0f, 1050.0f);
        
        enemyManager.AddMasher(7780.0f, 1050.0f);
        enemyManager.AddMasher(7860.0f, 1050.0f);
    }
    
//NO mashers in the void (Y >= 1300) - that's death zone not water!
    
    std::cout << "  Added Masher enemies in water areas (Y < 1300 only)" << std::endl;

//=== SPRINGS, SPIKES, CHECKPOINTS, MONITORS ===
//only place these on full level reset, not on checkpoint respawn
    if (includeMonitorsAndLevelObjects) {
//=== SPRINGS ===
    auto placeSpring = [](float x, SpringDirection dir, bool yellow) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
//springs sit on ground - use smaller offset so they don't float
            springManager.Add(x, static_cast<float>(groundY - 16), dir, yellow);
        }
    };
    
    placeSpring(300.0f, SpringDirection::Up, true);
    placeSpring(800.0f, SpringDirection::Up, true);
    placeSpring(1400.0f, SpringDirection::DiagonalUpRight, true);
    placeSpring(1860.0f, SpringDirection::Up, false);  //red spring (moved left)
    placeSpring(2700.0f, SpringDirection::Up, true);

    //spring at 3450 - manually placed lower
    {
        int groundY = FindGroundY(3450);
        if (groundY > 0) {
            springManager.Add(3450.0f, static_cast<float>(groundY - 12), SpringDirection::Up, true);
        }
    }

    placeSpring(8440.0f, SpringDirection::Up, true);  //dead end area - helps get up
    
//=== SPIKES ===
//only place spikes on very flat ground (check multiple points with strict tolerance)
    auto placeSpikesOnFlat = [](float x) {
//check 5 points across a wider area for flatness
        int groundY1 = FindGroundY(static_cast<int>(x - 32));
        int groundY2 = FindGroundY(static_cast<int>(x));
        int groundY3 = FindGroundY(static_cast<int>(x + 32));
        int groundY4 = FindGroundY(static_cast<int>(x + 64));
        int groundY5 = FindGroundY(static_cast<int>(x - 64));
        
//all points must exist and be within 4 pixels (very flat)
        if (groundY1 > 0 && groundY2 > 0 && groundY3 > 0 && groundY4 > 0 && groundY5 > 0) {
            int minY = std::min({groundY1, groundY2, groundY3, groundY4, groundY5});
            int maxY = std::max({groundY1, groundY2, groundY3, groundY4, groundY5});
            if ((maxY - minY) <= 4) {  //very strict - only truly flat ground
                spikeManager.Add(x, static_cast<float>(groundY2 - 32), SpikeDirection::Up);
                spikeManager.Add(x + 32.0f, static_cast<float>(groundY2 - 32), SpikeDirection::Up);
                std::cout << "Placed spikes at X:" << x << " Y:" << (groundY2 - 32) << std::endl;
            }
        }
    };
    
//try many positions - only truly flat ones will get spikes
    for (float x = 500.0f; x < 9500.0f; x += 800.0f) {
        // Skip area near whirl circle (around x=5260)
        if (x >= 5000.0f && x <= 5500.0f) continue;
        placeSpikesOnFlat(x);
    }
    
//=== CHECKPOINTS ===
    int spawnGroundY = FindGroundY(100);
    float spawnY = (spawnGroundY > 0) ? static_cast<float>(spawnGroundY - 50) : 1100.0f;
    checkpointManager.SetInitialSpawn(100.0f, spawnY);
    std::cout << "Initial spawn set at (100, " << spawnY << ") ground=" << spawnGroundY << std::endl;
    
    auto placeCheckpoint = [](float x) {
        int groundY = FindGroundY(static_cast<int>(x));
        if (groundY > 0) {
            checkpointManager.Add(x, static_cast<float>(groundY - 64));
        }
    };
    
    placeCheckpoint(2000.0f);  //early checkpoint
    placeCheckpoint(4000.0f);  //mid-level
    placeCheckpoint(6000.0f);  //later section
    placeCheckpoint(8000.0f);  //near end
    
//=== MONITORS (Power-ups) ===
//only place on flat ground, avoid springs/checkpoints/spikes
//springs at: 300, 800, 1400, 1900, 2700, 3500
//checkpoints at: 2000, 4000, 6000, 8000
    auto placeMonitorOnFlat = [](float x, MonitorType type) -> bool {
//check 3 points for flatness
        int groundY1 = FindGroundY(static_cast<int>(x - 20));
        int groundY2 = FindGroundY(static_cast<int>(x));
        int groundY3 = FindGroundY(static_cast<int>(x + 20));
        
        if (groundY1 > 0 && groundY2 > 0 && groundY3 > 0) {
            int minY = std::min({groundY1, groundY2, groundY3});
            int maxY = std::max({groundY1, groundY2, groundY3});
//only place if ground is flat (within 6 pixels)
            if ((maxY - minY) <= 6) {
                monitorManager.Add(x, static_cast<float>(groundY2 - 35), type);
                std::cout << "Placed monitor at X:" << x << std::endl;
                return true;
            }
        }
        return false;
    };
    
//try to place each monitor type - if position fails, try nearby positions
    auto tryPlaceMonitor = [&placeMonitorOnFlat](float preferredX, MonitorType type) {
//try preferred position first
        if (placeMonitorOnFlat(preferredX, type)) return;
//try nearby positions
        for (float offset = 50.0f; offset <= 200.0f; offset += 50.0f) {
            if (placeMonitorOnFlat(preferredX + offset, type)) return;
            if (placeMonitorOnFlat(preferredX - offset, type)) return;
        }
        std::cout << "Could not place monitor near X:" << preferredX << std::endl;
    };
    
//RING monitors (common)
    tryPlaceMonitor(250.0f, MonitorType::Ring);
    tryPlaceMonitor(2300.0f, MonitorType::Ring);
    tryPlaceMonitor(3800.0f, MonitorType::Ring);
    tryPlaceMonitor(4500.0f, MonitorType::Ring);
    tryPlaceMonitor(5700.0f, MonitorType::Ring);
    tryPlaceMonitor(6500.0f, MonitorType::Ring);
    tryPlaceMonitor(9200.0f, MonitorType::Ring);
    
//SHIELD monitors (3-4)
    tryPlaceMonitor(500.0f, MonitorType::Shield);
    tryPlaceMonitor(3100.0f, MonitorType::Shield);
    tryPlaceMonitor(5400.0f, MonitorType::Shield);
    tryPlaceMonitor(7500.0f, MonitorType::Shield);

//INVINCIBILITY monitors (3-4)
    tryPlaceMonitor(1050.0f, MonitorType::Invincibility);
    tryPlaceMonitor(2950.0f, MonitorType::Invincibility);
    tryPlaceMonitor(6350.0f, MonitorType::Invincibility);
    tryPlaceMonitor(9050.0f, MonitorType::Invincibility);

//SPEED SHOES monitors (3-4)
    tryPlaceMonitor(1700.0f, MonitorType::SpeedShoes);
    tryPlaceMonitor(4250.0f, MonitorType::SpeedShoes);
    tryPlaceMonitor(7150.0f, MonitorType::SpeedShoes);
    tryPlaceMonitor(8650.0f, MonitorType::SpeedShoes);

//EXTRA LIFE monitors (2 - rare)
    tryPlaceMonitor(3350.0f, MonitorType::ExtraLife);
    tryPlaceMonitor(7350.0f, MonitorType::ExtraLife);

//EGGMAN monitor (1 - trap)
    tryPlaceMonitor(5150.0f, MonitorType::Eggman);
    }  //end if (includeMonitorsAndLevelObjects)

//=== FLOWERS (decorative) ===
//placed from Tiled map objects: circles = Sunflower (Tall), rectangles = Flower (Short)
//y offset needed: Tiled gives base position, but sprite draws from top-left
//tall flowers are 40px tall, Short are 32px - offset by ~32 to plant them in grass
    
//sunflowers (Tall) - from circles in Tiled (add 32 to Y to plant in grass)
    flowerManager.Add(15.0f, 1157.0f, FlowerType::Short);
    flowerManager.Add(270.67f, 1158.0f, FlowerType::Short);
    flowerManager.Add(368.0f, 1077.33f, FlowerType::Short);
    flowerManager.Add(527.75f, 1158.0f, FlowerType::Short);
    flowerManager.Add(1167.25f, 1110.25f, FlowerType::Short);
    flowerManager.Add(1551.75f, 1158.0f, FlowerType::Short);
    flowerManager.Add(1952.33f, 1077.33f, FlowerType::Short);
    flowerManager.Add(2831.75f, 901.5f, FlowerType::Short);
    flowerManager.Add(3088.18f, 901.64f, FlowerType::Short);
    flowerManager.Add(3823.75f, 837.5f, FlowerType::Short);
    flowerManager.Add(4255.5f, 822.0f, FlowerType::Short);
    flowerManager.Add(4367.5f, 790.0f, FlowerType::Short);
    flowerManager.Add(4879.33f, 901.0f, FlowerType::Short);
    flowerManager.Add(5407.5f, 884.5f, FlowerType::Short);
    flowerManager.Add(6000.5f, 342.0f, FlowerType::Short);
    flowerManager.Add(6159.5f, 326.5f, FlowerType::Short);
    flowerManager.Add(6415.5f, 326.5f, FlowerType::Short);
    flowerManager.Add(6511.5f, 342.0f, FlowerType::Short);
    flowerManager.Add(6928.0f, 1413.5f, FlowerType::Short);
    flowerManager.Add(7312.73f, 1109.82f, FlowerType::Short);
    flowerManager.Add(8144.33f, 1109.33f, FlowerType::Short);
    flowerManager.Add(8175.67f, 1094.33f, FlowerType::Short);
    flowerManager.Add(8463.33f, 1157.67f, FlowerType::Short);
    flowerManager.Add(8559.0f, 1078.33f, FlowerType::Short);
    flowerManager.Add(8975.0f, 1413.5f, FlowerType::Short);
    flowerManager.Add(9231.0f, 1414.0f, FlowerType::Short);
    flowerManager.Add(9488.0f, 1414.0f, FlowerType::Short);
    flowerManager.Add(9744.0f, 1413.5f, FlowerType::Short);
    
//regular Flowers (Short) - from rectangles in Tiled (add 24 to Y to plant in grass)
    flowerManager.Add(79.0f, 1190.0f, FlowerType::Tall);
    flowerManager.Add(110.36f, 1189.82f, FlowerType::Tall);
    flowerManager.Add(1070.0f, 1125.75f, FlowerType::Tall);
    flowerManager.Add(1102.25f, 1126.25f, FlowerType::Tall);
    flowerManager.Add(1198.25f, 1125.25f, FlowerType::Tall);
    flowerManager.Add(1839.5f, 1125.5f, FlowerType::Tall);
    flowerManager.Add(2894.25f, 933.5f, FlowerType::Tall);
    flowerManager.Add(2926.0f, 933.75f, FlowerType::Tall);
    flowerManager.Add(3790.5f, 869.75f, FlowerType::Tall);
    flowerManager.Add(4143.5f, 869.0f, FlowerType::Tall);
    flowerManager.Add(4415.0f, 805.5f, FlowerType::Tall);
    flowerManager.Add(4687.0f, 869.67f, FlowerType::Tall);
    flowerManager.Add(4830.0f, 933.5f, FlowerType::Tall);
    flowerManager.Add(4941.5f, 933.0f, FlowerType::Tall);
    flowerManager.Add(4974.0f, 933.5f, FlowerType::Tall);
    flowerManager.Add(5967.0f, 357.0f, FlowerType::Tall);
    flowerManager.Add(6062.0f, 358.0f, FlowerType::Tall);
    flowerManager.Add(6111.5f, 358.0f, FlowerType::Tall);
    flowerManager.Add(6222.0f, 357.5f, FlowerType::Tall);
    flowerManager.Add(6318.0f, 358.5f, FlowerType::Tall);
    flowerManager.Add(6366.5f, 358.0f, FlowerType::Tall);
    flowerManager.Add(6478.5f, 358.0f, FlowerType::Tall);
    flowerManager.Add(6574.5f, 359.0f, FlowerType::Tall);
    flowerManager.Add(6735.5f, 1382.0f, FlowerType::Tall);
    flowerManager.Add(6879.0f, 1446.0f, FlowerType::Tall);
    flowerManager.Add(7214.55f, 1126.0f, FlowerType::Tall);
    flowerManager.Add(7245.82f, 1126.36f, FlowerType::Tall);
    flowerManager.Add(7342.0f, 1126.0f, FlowerType::Tall);
    flowerManager.Add(8782.33f, 1382.0f, FlowerType::Tall);
    flowerManager.Add(8926.0f, 1446.5f, FlowerType::Tall);
    flowerManager.Add(9038.5f, 1446.0f, FlowerType::Tall);
    flowerManager.Add(9070.0f, 1446.5f, FlowerType::Tall);
    flowerManager.Add(9294.0f, 1446.0f, FlowerType::Tall);
    flowerManager.Add(9326.0f, 1445.5f, FlowerType::Tall);
    flowerManager.Add(9550.0f, 1445.5f, FlowerType::Tall);
    flowerManager.Add(9584.0f, 1446.0f, FlowerType::Tall);
    flowerManager.Add(9806.0f, 1446.0f, FlowerType::Tall);
    flowerManager.Add(9838.5f, 1446.5f, FlowerType::Tall);
    
//=== BIG RING (Level Goal) ===
//place at end of level - spinning ring that completes the level
//map is 9984 pixels wide, place ring near the end
//ring is 64x64 base * 2.0 scale = 128x128 pixels
    int endGroundY = FindGroundY(9640);  //moved left ~60 pixels
    if (endGroundY > 0) {
//place ring above ground - it's 128 pixels tall so offset by 140
        bigRingManager.Add(9640.0f, static_cast<float>(endGroundY - 140), 2.0f);
        std::cout << "Big ring placed at (9640, " << (endGroundY - 140) << ") - 128x128 pixels" << std::endl;
    } else {
//fallback position
        bigRingManager.Add(9640.0f, 1300.0f, 2.0f);
        std::cout << "Big ring placed at fallback (9640, 1300)" << std::endl;
    }
    
    std::cout << "Game objects placed based on terrain." << std::endl;
}

//terrain image for rendering (will be loaded from Sonic_Terrain.png)
engine::BitmapPtr terrainImage = nullptr;

void CreateTestTerrain() {
//================================================================
//NEW TERRAIN SYSTEM (Tiled-based):
//- Visual map: Sonic_Terrain.csv (39×6 tiles at 256×256)
//- Collision grid: Collision_Map.csv (1248×192 cells at 8×8)
//- Total pixels: 9984×1536
//- Screen viewport: 320×224 pixels (original Sonic Genesis resolution)
//================================================================
    
    std::cout << "=== Loading Tiled Terrain System ===" << std::endl;
    
//load collision grid from Collision_Map.csv
    gridLayer = new GridLayer();
    bool gridLoaded = gridLayer->LoadCSV(g_assetPath + "Collision_Map.csv");
    
    if (!gridLoaded) {
        std::cerr << "CRITICAL: Failed to load Collision_Map.csv!" << std::endl;
        std::cerr << "Creating fallback grid..." << std::endl;
        
//fallback: create a simple flat ground grid
//9984/8 = 1248 cols, 1536/8 = 192 rows
        constexpr int GRID_COLS = 1248;
        constexpr int GRID_ROWS = 192;
        gridLayer->Create(GRID_ROWS, GRID_COLS);
        
//add ground at bottom (roughly last 20% of height)
        int groundStartRow = static_cast<int>(GRID_ROWS * 0.8);
        for (int row = groundStartRow; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                gridLayer->SetTile(static_cast<Dim>(col), static_cast<Dim>(row), GRID_SOLID);
            }
        }
        std::cout << "Created fallback grid: " << GRID_COLS << "x" << GRID_ROWS << std::endl;
    }
    
//load terrain pre-rendered image (optional - for direct rendering without tile borders)
    auto& loader = engine::BitmapLoader::Instance();
    terrainImage = loader.Load(g_assetPath + "Sonic_Terrain.png");
    
    if (terrainImage) {
        std::cout << "✓ Loaded terrain image: " << terrainImage->GetWidth() << "x" << terrainImage->GetHeight() << " pixels" << std::endl;
    } else {
        std::cout << "Note: Sonic_Terrain.png not found - using tile-based rendering" << std::endl;
    }
    
//load tileset for tile-based rendering
    engine::BitmapPtr tileset = loader.Load(g_assetPath + "Map_Tilesheet.png");
    if (!tileset) {
        std::cerr << "WARNING: Map_Tilesheet.png not found!" << std::endl;
        tileset = ResourceManager::Instance().GetTilesSheet();  //fallback
    } else {
        std::cout << "✓ Loaded tileset: " << tileset->GetWidth() << "x" << tileset->GetHeight() << " pixels" << std::endl;
    }
    
//create tile layer (39 tiles wide × 6 tiles tall at 256×256)
    constexpr int TILES_X = 39;
    constexpr int TILES_Y = 6;
    
    actionLayer = new TileLayer();
    actionLayer->Create(static_cast<Dim>(TILES_Y), static_cast<Dim>(TILES_X), tileset);
    
//load visual map from CSV
    bool mapLoaded = actionLayer->LoadCSV(g_assetPath + "Sonic_Terrain.csv");
    if (mapLoaded) {
        std::cout << "✓ Loaded visual map from Sonic_Terrain.csv" << std::endl;
    } else {
        std::cerr << "WARNING: Failed to load Sonic_Terrain.csv!" << std::endl;
    }
    
    actionLayer->SetViewWindow({0, 0, SCREEN_WIDTH, SCREEN_HEIGHT});
    
    std::cout << "Terrain dimensions: " << MAP_WIDTH << "x" << MAP_HEIGHT << " pixels" << std::endl;
    std::cout << "Visual map: " << actionLayer->GetCols() << "x" << actionLayer->GetRows() << " tiles" << std::endl;
    std::cout << "Collision grid: " << gridLayer->GetCols() << "x" << gridLayer->GetRows() << " cells" << std::endl;
    std::cout << "Grid pixel size: " << gridLayer->GetPixelWidth() << "x" << gridLayer->GetPixelHeight() << std::endl;
    
//debug: count special tiles
    int tunnelCount = 0, loopCount = 0, slopeCount = 0, deathCount = 0;
    for (Dim row = 0; row < gridLayer->GetRows(); ++row) {
        for (Dim col = 0; col < gridLayer->GetCols(); ++col) {
            GridIndex tile = gridLayer->GetTile(col, row);
            if (tile == GRID_TUNNEL) tunnelCount++;
            else if (tile == GRID_LOOP) loopCount++;
            else if (tile == GRID_SLOPE) slopeCount++;
            else if (tile == GRID_DEATH) deathCount++;
        }
    }
    std::cout << "=== Special tile counts ===" << std::endl;
    std::cout << "  Tunnels: " << tunnelCount << std::endl;
    std::cout << "  Loops: " << loopCount << std::endl;
    std::cout << "  Slopes: " << slopeCount << std::endl;
    std::cout << "  Death: " << deathCount << std::endl;
    
//setup managers
    ringManager.SetGridLayer(gridLayer);
    enemyManager.SetGridLayer(gridLayer);
    AnimalManager::Instance().SetGridLayer(gridLayer);
    
//place all game objects
    PlaceGameObjects();
    
//setup parallax background
    auto bgSheet = ResourceManager::Instance().GetBackgroundSheet();
    SetupGreenHillBackground(parallaxManager, bgSheet);
    
//setup callbacks
    ringManager.SetOnRingCollected([](int count) {
        std::cout << "Collected " << count << " ring(s)!" << std::endl;
        gameHUD.AddRings(count);
        SoundManager::Instance().OnRingCollect();
    });
    enemyManager.SetOnEnemyKilled([](int score) {
        std::cout << "Enemy killed! +" << score << " points" << std::endl;
        gameHUD.AddScore(score);
    });
    enemyManager.SetOnAnimalFreed([](float x, float y) {
        std::cout << "Animal freed at (" << x << ", " << y << ")" << std::endl;
        AnimalManager::Instance().Spawn(x, y);
    });
    
//start level timer
    gameHUD.StartLevel(GetSystemTime());
    
    std::cout << "=== Terrain Load Complete ===" << std::endl;
}

int main() {
    std::cout << "=== Sonic Engine - CS454 Project ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left/Right or A/D: Move" << std::endl;
    std::cout << "  Space/Up/W: Jump" << std::endl;
    std::cout << "  Down/S: Crouch (still) or Roll (moving)" << std::endl;
    std::cout << "  Down + Jump: Spindash (charge and release)" << std::endl;
    std::cout << "  Left Click: Look Up (when still)" << std::endl;
    std::cout << "  G: Grid   F1: Debug   ESC: Pause   M: Music   Q: Quit" << std::endl;
    std::cout << std::endl;
    
//initialize window at 640×448
    if (!EngineInit(WINDOW_WIDTH, WINDOW_HEIGHT, "Sonic Engine - SFML")) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return 1;
    }
    
//set virtual resolution for classic Sonic "zoomed in" look
//renders at 320×224 and scales 2x to fill 640×448 window
    GetGraphics().SetVirtualResolution(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    
    std::cout << "=== RESOLUTION SETTINGS ===" << std::endl;
    std::cout << "Window size: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    std::cout << "Virtual (render) resolution: " << VIRTUAL_WIDTH << "x" << VIRTUAL_HEIGHT << std::endl;
    std::cout << "Scale factor: 2x (classic Sonic zoom)" << std::endl;
    std::cout << "Backbuffer size: " << GetGraphics().GetBackBuffer().getSize().x << "x" << GetGraphics().GetBackBuffer().getSize().y << std::endl;
    std::cout << "===========================" << std::endl;
    
//detect asset path early (handles running from project root OR bin/)
    g_assetPath = FindAssetPath();
    std::cout << "Using asset path: " << g_assetPath << std::endl;
    
//load assets using the detected path
    ResourceManager::Instance().SetAssetPath(g_assetPath);
    assetsLoaded = ResourceManager::Instance().LoadAll();

    if (!assetsLoaded) {
        std::cout << "Assets not found - using placeholder graphics" << std::endl;
        std::cout << "To use real sprites, copy sprite sheets to assets/ folder" << std::endl;
    }

//initialize sound system
    SoundManager::Instance().SetAssetPath(g_assetPath);
    SoundManager::Instance().LoadAll();
    SoundManager::Instance().PlayMusic(MusicTrack::GreenHillZone);  //start background music

    CreateTestTerrain();
//showGrid toggled with G key
    
    sonic = new SonicPlayer();
//spawn at start position above ground (ground at Y≈1216 at X=100)
    float sonicSpawnY = 1100.0f;  //above ground level, physics settles
    sonic->Create(100.0f, sonicSpawnY, gridLayer);  
    std::cout << "Sonic spawned at (100, " << sonicSpawnY << ")" << std::endl;
    
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
//NOTE: Removed lives callback - HUD gets lives directly from sonic pointer
//state logging disabled to improve performance
    sonic->SetOnStateChanged([](SonicState) {
//logging disabled for performance
    });
    sonic->SetOnScatterRings([](float x, float y, int count) {
        ringManager.ScatterRings(x, y, count);
        SoundManager::Instance().PlaySound(SoundEffect::RingLoss);
        std::cout << "Scattered " << count << " rings!" << std::endl;
    });
//NOTE: onDeath callback removed - was causing dual game over logic
//game over is now handled ONLY in the death state checking code
//sonic->SetOnDeath([]() {
//std::cout << "=== GAME OVER ===" << std::endl;
//currentScreen = GameScreen::GameOver;
//});

//load and initialize whirl circles
    std::string collisionMapPath = g_assetPath + "Collision_Map.tmj";
    if (whirlCircleManager.LoadFromFile(collisionMapPath.c_str())) {
        whirlCircleManager.Initialize(sonic);
        std::cout << "Whirl circles loaded and initialized" << std::endl;
    } else {
        std::cout << "WARNING: Failed to load whirl circles from " << collisionMapPath << std::endl;
    }

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
