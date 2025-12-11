#ifndef SPRITE_SHEET_CONFIG_HPP
#define SPRITE_SHEET_CONFIG_HPP

#include "Engine.hpp"
#include <vector>
#include <string>
#include <map>

namespace app {

// Frame rectangle from sprite sheet
struct FrameRect {
    int x, y, w, h;
};

// Animation definition with frames
struct AnimationDef {
    std::string id;
    std::vector<FrameRect> frames;
    bool loop = true;
    unsigned delay = 100;  // ms per frame
};

// ============================================================
// SONIC SPRITE SHEET - sonic_sheet_fixed.png  
// Sheet size: 658 x 931 pixels
// Background: Green (needs color key)
// Layout based on visible labels in sprite sheet
// ============================================================

class SonicSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 147;
    static constexpr unsigned char COLOR_KEY_G = 187;
    static constexpr unsigned char COLOR_KEY_B = 148;
    
    static std::vector<AnimationDef> GetAnimations() {
        std::vector<AnimationDef> anims;
        
        // CORRECTED coordinates based on actual sprite sheet analysis
        // Scanned using Python/PIL to find exact sprite boundaries
        
        // === ROW 0: Idle, Bored, Loop, Looking Up, Crouch, Spring ===
        anims.push_back({"sonic_idle", {{27, 39, 30, 40}}, false, 100});
        
        anims.push_back({"sonic_bored", {
            {113, 39, 31, 40},
            {183, 39, 31, 40}
        }, true, 400});
        
        anims.push_back({"sonic_loop", {
            {253, 39, 31, 41},
            {323, 39, 31, 40}
        }, true, 60});
        
        anims.push_back({"sonic_lookup", {{408, 39, 32, 40}}, false, 100});  // Fixed x from 383 to 408
        
        // Crouch shows Sonic curled into ball shape (this sprite sheet doesn't have a ducking pose)
        anims.push_back({"sonic_crouch", {
            {492, 48, 37, 31}   // Curled up ball shape
        }, false, 80});
        
        // Spindash: alternates between curled pose and ball frames for "revving" effect
        anims.push_back({"sonic_spindash", {
            {492, 48, 37, 31},   // Curled up (from Row 0)
            {27, 407, 31, 32},   // Ball frame 1
            {97, 407, 32, 31},   // Ball frame 2
            {168, 407, 31, 32},  // Ball frame 3
        }, true, 60});  // Fast loop for revving effect
        
        anims.push_back({"sonic_spring", {{580, 35, 26, 47}}, false, 100});  // Fixed coordinates
        
        // === ROW 1: Walk (6 frames), Skid (2 frames) ===
        anims.push_back({"sonic_walk", {
            {29, 131, 26, 39},
            {94, 129, 38, 39},
            {162, 130, 33, 40},
            {235, 131, 37, 40},
            {304, 129, 39, 39},
            {374, 130, 40, 40}
        }, true, 80});
        
        anims.push_back({"sonic_skid", {
            {464, 132, 31, 38},
            {526, 132, 39, 38}
        }, true, 100});
        
        // === ROW 2: Walk Angled (6 frames), Balance (2 frames) ===
        anims.push_back({"sonic_walk_slope", {
            {24, 221, 40, 41},
            {93, 219, 39, 49},
            {164, 221, 38, 40},
            {234, 222, 40, 40},
            {303, 220, 40, 47},
            {374, 221, 45, 39}
        }, true, 80});
        
        anims.push_back({"sonic_balance", {
            {447, 220, 40, 41},
            {519, 221, 37, 40}
        }, true, 200});
        
        // === ROW 3: Full Speed (4 frames), Full Speed Angled (4 frames) ===
        anims.push_back({"sonic_run", {
            {24, 314, 31, 40},
            {93, 314, 32, 40},
            {164, 315, 31, 39},
            {233, 314, 32, 40}
        }, true, 40});
        
        anims.push_back({"sonic_run_slope", {
            {322, 313, 38, 42},
            {392, 313, 38, 42},
            {462, 313, 39, 41},
            {533, 313, 38, 42}
        }, true, 40});
        
        // === ROW 4: Ball/Jump (5 frames), Push (4 frames) ===
        anims.push_back({"sonic_ball", {
            {27, 407, 31, 32},
            {97, 407, 32, 31},
            {168, 407, 31, 32},
            {237, 407, 32, 32},
            {307, 407, 32, 32}
        }, true, 35});
        
        anims.push_back({"sonic_push", {
            {389, 404, 32, 40},
            {466, 404, 25, 39},
            {528, 404, 33, 40},
            {606, 404, 25, 39}
        }, true, 150});
        
        // === ROW 5: Pipe Cling (2), Tunnel Spin (5), Air Gasp (1) ===
        anims.push_back({"sonic_pipe", {
            {19, 498, 47, 32},
            {88, 499, 48, 31}
        }, true, 100});
        
        anims.push_back({"sonic_tunnel", {
            {181, 501, 46, 25},
            {246, 501, 46, 25},
            {311, 501, 47, 25},
            {382, 501, 39, 25},
            {467, 501, 39, 25}
        }, true, 50});
        
        anims.push_back({"sonic_gasp", {{544, 494, 40, 38}}, false, 100});
        
        // === ROW 6: Hurt (2), Drown, Death, Continue (4+) ===
        anims.push_back({"sonic_hurt", {
            {23, 590, 40, 32},
            {93, 591, 39, 29}
        }, true, 100});
        
        anims.push_back({"sonic_drown", {{180, 580, 39, 48}}, false, 100});
        anims.push_back({"sonic_death", {{265, 577, 39, 50}}, false, 100});
        
        anims.push_back({"sonic_continue", {
            {351, 599, 40, 33},
            {421, 599, 40, 33},
            {491, 599, 40, 33},
            {557, 612, 48, 17}
        }, true, 150});
        
        // === ROW 7: Endings ===
        anims.push_back({"sonic_ending_bad", {
            {27, 711, 31, 41},
            {94, 712, 32, 39}
        }, true, 200});
        
        anims.push_back({"sonic_ending_good", {
            {181, 710, 30, 41},
            {250, 710, 31, 42}
        }, true, 200});
        
        // Additional loop frames from Row 7 (for spin loops)
        anims.push_back({"sonic_loop_extended", {
            {320, 711, 32, 40},
            {390, 710, 32, 42},
            {460, 710, 32, 41},
            {530, 712, 32, 39}
        }, true, 50});
        
        return anims;
    }
};

// ============================================================
// ENEMIES SPRITE SHEET - enemies_sheet_fixed.png
// Sheet size: 1398 x 898 pixels
// Background color: RGB(0, 128, 0) - green (color key)
// Contains ALL Green Hill Zone enemies
// ============================================================

class EnemySpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 128;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
    // === CRABMEAT / GANIGANI (Top-left corner) ===
    static std::vector<AnimationDef> GetCrabmeatAnimations() {
        return {
            {"crabmeat_walk", {
                {4, 10, 48, 32},
                {56, 10, 48, 32},
                {108, 10, 48, 32}
            }, true, 150},
            {"crabmeat_attack", {
                {4, 46, 64, 32}
            }, false, 200}
        };
    }
    
    // === BUZZ BOMBER / BEETON (Below Crabmeat) ===
    static std::vector<AnimationDef> GetBuzzBomberAnimations() {
        return {
            {"buzzbomber_fly", {
                {4, 90, 48, 36},
                {56, 90, 48, 36}
            }, true, 80},
            {"buzzbomber_shoot", {
                {108, 90, 48, 36},
                {4, 130, 48, 36}
            }, false, 100}
        };
    }
    
    // === MOTOBUG / MOTORA (Right of Crabmeat) ===
    static std::vector<AnimationDef> GetMotobugAnimations() {
        return {
            {"motobug_move", {
                {168, 90, 44, 32},
                {216, 90, 44, 32},
                {264, 90, 44, 32},
                {312, 90, 44, 32}
            }, true, 100}
        };
    }
    
    // === MASHER / BATABATA (Piranha, below Motobug) ===
    static std::vector<AnimationDef> GetMasherAnimations() {
        return {
            {"masher_idle", {{168, 196, 24, 40}}, false, 100},
            {"masher_jump", {
                {168, 196, 24, 40},
                {196, 196, 24, 40}
            }, true, 80}
        };
    }
    
    // === BLUE NEWTRON / MELEON (Chameleon that appears/shoots) ===
    static std::vector<AnimationDef> GetNewtronBlueAnimations() {
        return {
            {"newtron_blue_appear", {
                {4, 330, 48, 32},
                {56, 330, 48, 32},
                {108, 330, 48, 32}
            }, false, 100},
            {"newtron_blue_idle", {{108, 330, 48, 32}}, false, 100}
        };
    }
    
    // === GREEN NEWTRON (Flies toward player) ===
    static std::vector<AnimationDef> GetNewtronGreenAnimations() {
        return {
            {"newtron_green_fly", {
                {4, 410, 48, 32},
                {56, 410, 48, 32}
            }, true, 80}
        };
    }
    
    // === BOMB (Round bomb enemy, walks and explodes) ===
    static std::vector<AnimationDef> GetBombAnimations() {
        return {
            {"bomb_walk", {
                {4, 598, 32, 32},
                {40, 598, 32, 32},
                {76, 598, 32, 32},
                {112, 598, 32, 32}
            }, true, 120},
            {"bomb_explode", {
                {4, 650, 48, 48},
                {56, 650, 56, 56},
                {116, 650, 64, 64}
            }, false, 80}
        };
    }
    
    // === CATERKILLER / NAL (Segmented caterpillar) ===
    static std::vector<AnimationDef> GetCaterkillerAnimations() {
        return {
            {"caterkiller_move", {
                {324, 10, 64, 24},
                {324, 38, 64, 24},
                {324, 66, 64, 24}
            }, true, 150},
            {"caterkiller_segment", {{324, 94, 16, 16}}, false, 100}
        };
    }
    
    // === SPIKES / YADORIN (Hermit crab with spike shell) ===
    static std::vector<AnimationDef> GetSpikesEnemyAnimations() {
        return {
            {"spikes_enemy_walk", {
                {324, 178, 32, 32},
                {360, 178, 32, 32}
            }, true, 150},
            {"spikes_enemy_hide", {{396, 178, 24, 24}}, false, 100}
        };
    }
    
    // === BATBRAIN / BASARAN (Bat that drops down) ===
    static std::vector<AnimationDef> GetBatbrainAnimations() {
        return {
            {"batbrain_hang", {{484, 10, 24, 24}}, false, 100},
            {"batbrain_fly", {
                {484, 38, 32, 24},
                {520, 38, 32, 24}
            }, true, 80}
        };
    }
    
    // === BURROBOT / MOGURIN (Mole that pops from ground) ===
    static std::vector<AnimationDef> GetBurrobotAnimations() {
        return {
            {"burrobot_dig", {
                {630, 10, 32, 32},
                {666, 10, 32, 32}
            }, true, 100},
            {"burrobot_jump", {
                {630, 46, 32, 40},
                {666, 46, 32, 40}
            }, true, 80}
        };
    }
    
    // === ROLLER / ARMA (Armadillo that rolls) ===
    static std::vector<AnimationDef> GetRollerAnimations() {
        return {
            {"roller_walk", {
                {484, 406, 40, 32},
                {528, 406, 40, 32}
            }, true, 120},
            {"roller_ball", {
                {484, 442, 32, 32},
                {520, 442, 32, 32},
                {556, 442, 32, 32}
            }, true, 60}
        };
    }
    
    // === JAWS / PUKU-PUKU (Fish that swims in water) ===
    static std::vector<AnimationDef> GetJawsAnimations() {
        return {
            {"jaws_swim", {
                {630, 462, 32, 24},
                {666, 462, 32, 24}
            }, true, 100}
        };
    }
    
    // === ORBINAUT / UNIDUS (Floating enemy with spike balls) ===
    static std::vector<AnimationDef> GetOrbinautAnimations() {
        return {
            {"orbinaut_float", {
                {772, 10, 32, 32},
                {808, 10, 32, 32}
            }, true, 100},
            {"orbinaut_spike", {{772, 46, 16, 16}}, false, 100}
        };
    }
    
    // === BALL HOG / TON-TON (Pig that throws bombs) ===
    static std::vector<AnimationDef> GetBallHogAnimations() {
        return {
            {"ballhog_idle", {{772, 482, 32, 40}}, false, 100},
            {"ballhog_throw", {
                {808, 482, 40, 40},
                {852, 482, 40, 40}
            }, false, 100}
        };
    }
    
    // === PROJECTILES ===
    static FrameRect GetCrabmeatProjectile() { return {4, 52, 8, 8}; }
    static FrameRect GetBuzzBomberProjectile() { return {56, 52, 12, 12}; }
    static FrameRect GetBombProjectile() { return {148, 598, 16, 16}; }
    
    // === EXPLOSION (shared by all enemies) ===
    static AnimationDef GetExplosionAnimation() {
        return {"explosion", {
            {4, 720, 32, 32},
            {40, 720, 40, 40},
            {84, 720, 48, 48},
            {136, 720, 48, 48}
        }, false, 60};
    }
};

// ============================================================
// MISC SPRITE SHEET - misc_fixed.png  
// Sheet size: 676 x 476 pixels
// Contains: Rings, Spikes, Monitors, Checkpoints, Springs, etc.
// Background: RGB(0, 128, 0) green
// ============================================================

class MiscSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 128;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
    // === RINGS (Dami-Karv verified coordinates) ===
    // Spinning animation frames
    static AnimationDef GetRingAnimation() {
        return {"ring", {
            {8, 25, 16, 16},    // Frame 1
            {32, 25, 16, 16},   // Frame 2
            {56, 25, 16, 16},   // Frame 3 (was 8px wide - fixed to 16)
            {72, 25, 16, 16}    // Frame 4
        }, true, 70};
    }
    
    // Ring collection sparkle animation (uses same ring frames but faster)
    // Creates a quick "pop" effect as the ring is collected
    static AnimationDef GetRingCollectAnimation() {
        return {"ring_collect", {
            {8, 25, 16, 16},   // Same as ring frames
            {32, 25, 16, 16},
            {56, 25, 16, 16},
            {72, 25, 16, 16}
        }, false, 30};  // Very fast animation (30ms per frame)
    }
    
    // === BIG RING / OKINA RING (Special stage entrance) ===
    static AnimationDef GetBigRingAnimation() {
        return {"big_ring", {
            {8, 49, 64, 64},
            {80, 49, 64, 64},
            {152, 49, 64, 64},
            {224, 49, 64, 64}
        }, true, 100};
    }
    
    // === SPIKES (Terrain hazards) ===
    static FrameRect GetSpikesUp()    { return {258, 73, 32, 32}; }
    static FrameRect GetSpikesDown()  { return {258, 106, 32, 32}; }
    static FrameRect GetSpikesLeft()  { return {291, 73, 16, 48}; }
    static FrameRect GetSpikesRight() { return {308, 73, 16, 48}; }
    
    // === SCORE POPUPS ===
    static FrameRect GetScore100()   { return {258, 170, 24, 12}; }
    static FrameRect GetScore200()   { return {290, 170, 24, 12}; }
    static FrameRect GetScore500()   { return {322, 170, 24, 12}; }
    static FrameRect GetScore1000()  { return {354, 170, 32, 12}; }
    static FrameRect GetScore10000() { return {394, 170, 40, 12}; }
    
    // === INVINCIBILITY SPARKLES (Stars rotating around Sonic) ===
    static AnimationDef GetInvincibilityAnimation() {
        return {"invincibility", {
            {469, 25, 16, 16},
            {493, 25, 16, 16},
            {517, 25, 16, 16},
            {541, 25, 16, 16}
        }, true, 50};
    }
    
    // === SHIELD BUBBLE (with empty frame for flicker) ===
    static AnimationDef GetShieldAnimation() {
        return {"shield", {
            {469, 105, 32, 32},
            {509, 105, 32, 32},
            {549, 105, 32, 32}
        }, true, 100};
    }
    
    // === MONITORS (Dami-Karv verified coordinates) ===
    // Unbroken box: 8, 312, 32, 32
    // Broken box: 48, 328, 32, 16
    static FrameRect GetMonitorUnbroken()   { return {8, 312, 32, 32}; }
    static FrameRect GetMonitorBroken()     { return {48, 328, 32, 16}; }
    
    // Monitor icons (drawn on top of box)
    static FrameRect GetIconRing()       { return {80, 367, 16, 16}; }
    static FrameRect GetIconShield()     { return {104, 367, 16, 16}; }
    static FrameRect GetIconInvincible() { return {128, 367, 16, 16}; }
    static FrameRect GetIconSpeed()      { return {152, 367, 16, 16}; }
    static FrameRect GetIconLife()       { return {176, 367, 16, 16}; }
    static FrameRect GetIconEggman()     { return {200, 367, 16, 16}; }
    
    // Legacy compatibility
    static FrameRect GetMonitorEmpty()      { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorRing()       { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorShield()     { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorInvincible() { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorSpeed()      { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorLife()       { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorEggman()     { return GetMonitorUnbroken(); }
    
    // === CHECKPOINT (Sign post) ===
    static AnimationDef GetCheckpointAnimation() {
        return {"checkpoint", {
            {170, 313, 24, 48},
            {202, 313, 24, 48},
            {234, 313, 24, 48},
            {266, 313, 24, 48},
            {298, 313, 24, 48},
            {330, 313, 24, 48},
            {362, 313, 24, 48},
            {394, 313, 24, 48}
        }, true, 80};
    }
    
    // === SPRINGS (Dami-Karv verified coordinates) ===
    // Yellow spring - Normal: 554, 313, 32, 16  Extended: 554, 337, 32, 32
    static AnimationDef GetYellowSpringAnimation() {
        return {"spring_yellow", {
            {554, 313, 32, 16},   // Normal (compressed)
            {554, 337, 32, 32}    // Extended (bouncing)
        }, false, 50};
    }
    
    // Red spring (same pattern, different column)
    static AnimationDef GetRedSpringAnimation() {
        return {"spring_red", {
            {594, 313, 32, 16},   // Normal
            {594, 337, 32, 32}    // Extended
        }, false, 50};
    }
    
    // Diagonal springs
    static FrameRect GetDiagonalSpringYellowUpR() { return {634, 313, 32, 32}; }
    static FrameRect GetDiagonalSpringRedUpR()    { return {634, 353, 32, 32}; }
    
    // === SWITCH (Button) ===
    static FrameRect GetSwitchUp()   { return {258, 233, 32, 16}; }
    static FrameRect GetSwitchDown() { return {258, 257, 32, 8}; }
    
    // === GAME OVER / TIME OVER TEXT ===
    static FrameRect GetGameOverText() { return {469, 193, 128, 16}; }
    static FrameRect GetTimeOverText() { return {469, 217, 128, 16}; }
    
    // === RING LOSS (Scattered ring - smaller frames) ===
    static AnimationDef GetRingLossAnimation() {
        return {"ring_loss", {
            {8, 25, 16, 16},
            {32, 25, 16, 16}
        }, true, 50};
    }
};

// ============================================================
// ANIMALS SPRITE SHEET - animals_fixed.png
// Sheet size: 96 x 334 pixels  
// Small animals freed when enemies are destroyed
// Background: RGB(0, 128, 0) green
// ============================================================

class AnimalSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 128;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
    // === FLICKY (Blue bird) - Green Hill Zone ===
    static AnimationDef GetFlickyAnimation() {
        return {"flicky", {
            {8, 34, 16, 16},
            {32, 34, 20, 16},
            {56, 34, 16, 16}
        }, true, 80};
    }
    
    // === POCKY (Tan rabbit) - Green Hill Zone ===
    static AnimationDef GetPockyAnimation() {
        return {"pocky", {
            {8, 82, 16, 24},
            {32, 82, 16, 24},
            {56, 82, 20, 24}
        }, true, 100};
    }
    
    // === CUCKY (Yellow chicken) - Marble Zone ===
    static AnimationDef GetCuckyAnimation() {
        return {"cucky", {
            {8, 130, 16, 16},
            {32, 130, 20, 16}
        }, true, 80};
    }
    
    // === RICKY (Brown squirrel) - Marble/Spring Yard ===
    static AnimationDef GetRickyAnimation() {
        return {"ricky", {
            {8, 178, 16, 20},
            {32, 178, 20, 20}
        }, true, 100};
    }
    
    // === PECKY (Blue penguin) - Labyrinth Zone ===
    static AnimationDef GetPeckyAnimation() {
        return {"pecky", {
            {8, 226, 16, 20},
            {32, 226, 16, 20}
        }, true, 100};
    }
    
    // === ROCKY (Gray seal) - Labyrinth Zone ===
    static AnimationDef GetRockyAnimation() {
        return {"rocky", {
            {8, 274, 20, 16},
            {36, 274, 20, 16}
        }, true, 100};
    }
};

// ============================================================
// TILES - tiles_first_map_fixed.png
// Sheet size: 912 x 2000 pixels
// Green Hill Zone terrain tiles (approximately 5x11 grid)
// Each tile is ~182x182 pixels (to be scaled to 128x128)
// Pink/magenta areas are transparency
// ============================================================

class TileConfig {
public:
    // Tile dimensions in the sprite sheet
    static constexpr int SHEET_TILE_SIZE = 182;
    // Target tile size for game (power of 2)
    static constexpr int GAME_TILE_SIZE = 128;
    // Grid layout
    static constexpr int TILES_PER_ROW = 5;
    static constexpr int TILES_PER_COL = 11;
    
    // Get source rectangle for a tile index
    static FrameRect GetTileRect(int index) {
        int col = index % TILES_PER_ROW;
        int row = index / TILES_PER_ROW;
        return {col * SHEET_TILE_SIZE, row * SHEET_TILE_SIZE, 
                SHEET_TILE_SIZE, SHEET_TILE_SIZE};
    }
    
    // === TILE INDICES (based on visual inspection of tileset) ===
    // Row 0: Ground variations with grass and palm trees
    static constexpr int TILE_GROUND_PALM_1 = 0;     // Ground with palm tree
    static constexpr int TILE_GROUND_PALM_2 = 1;     // Ground with different palm
    static constexpr int TILE_SLOPE_DOWN_R = 2;      // Slope going down-right
    static constexpr int TILE_SLOPE_DOWN_L = 3;      // Slope going down-left
    static constexpr int TILE_GROUND_EDGE = 4;       // Ground right edge
    
    // Row 1: More terrain
    static constexpr int TILE_CLIFF_TOP = 5;
    static constexpr int TILE_UNDERGROUND = 6;       // Brown checkered fill
    static constexpr int TILE_CLIFF_BOTTOM = 7;
    static constexpr int TILE_SLOPE_UP_R = 8;
    static constexpr int TILE_CORNER_OUT = 9;
    
    // Row 2: Platforms and edges
    static constexpr int TILE_PLATFORM_L = 10;
    static constexpr int TILE_PLATFORM_M = 11;
    static constexpr int TILE_PLATFORM_R = 12;
    static constexpr int TILE_GROUND_FLAT = 13;
    static constexpr int TILE_WATERFALL = 14;
    
    // Row 3: Loop sections (top parts)
    static constexpr int TILE_LOOP_TOP_L = 15;
    static constexpr int TILE_LOOP_TOP_M = 16;
    static constexpr int TILE_LOOP_TOP_R = 17;
    static constexpr int TILE_LOOP_SIDE = 18;
    static constexpr int TILE_LOOP_INNER = 19;
    
    // Row 4: Loop sections (bottom parts)
    static constexpr int TILE_LOOP_BOT_L = 20;
    static constexpr int TILE_LOOP_BOT_M = 21;
    static constexpr int TILE_LOOP_BOT_R = 22;
    static constexpr int TILE_TUNNEL = 23;
    static constexpr int TILE_BRIDGE = 24;
    
    // Remaining rows: More variations, decorations
    static constexpr int TILE_GRASS_TUFT = 50;
    static constexpr int TILE_FLOWERS = 51;
    
    // Transparency color (pink/magenta in the tileset)
    static constexpr unsigned char TRANS_R = 255;
    static constexpr unsigned char TRANS_G = 0;
    static constexpr unsigned char TRANS_B = 255;
};

// ============================================================
// BACKGROUND - background_foreground64.png
// Sheet size: 704 x 640 pixels
// Parallax background layers for Green Hill Zone
// Multiple horizontal strips representing different depths
// ============================================================

class BackgroundConfig {
public:
    // Layer definitions (from top to bottom of image)
    // Each layer is a horizontal strip that tiles/repeats
    
    // Sky with clouds (slowest movement)
    static FrameRect GetSkyLayer()        { return {0, 0, 640, 112}; }
    static constexpr float SKY_SCROLL = 0.1f;
    
    // Distant mountains/city silhouette
    static FrameRect GetMountainLayer()   { return {0, 112, 640, 48}; }
    static constexpr float MOUNTAIN_SCROLL = 0.2f;
    
    // Mid-ground hills with bushes
    static FrameRect GetHillsLayer()      { return {0, 160, 640, 64}; }
    static constexpr float HILLS_SCROLL = 0.4f;
    
    // Near hills (faster parallax)
    static FrameRect GetNearHillsLayer()  { return {0, 224, 640, 64}; }
    static constexpr float NEAR_HILLS_SCROLL = 0.6f;
    
    // Checkerboard/ground decoration layer
    static FrameRect GetGroundDecor()     { return {0, 288, 640, 64}; }
    static constexpr float GROUND_DECOR_SCROLL = 0.8f;
    
    // Water surface (for water areas)
    static FrameRect GetWaterSurface()    { return {0, 352, 640, 32}; }
    
    // Water body (animated)
    static AnimationDef GetWaterAnimation() {
        return {"water", {
            {0, 384, 640, 64},
            {0, 448, 640, 64},
            {0, 512, 640, 64}
        }, true, 150};
    }
    
    // Extra tiles on right side
    static FrameRect GetWaterTile()       { return {640, 0, 64, 64}; }
    static FrameRect GetSkyTile()         { return {640, 64, 64, 64}; }
};

// ============================================================
// HELPER: All sprite sheet file paths
// ============================================================

class SpriteSheetPaths {
public:
    static constexpr const char* SONIC = "assets/sonic_sheet_fixed.png";
    static constexpr const char* ENEMIES = "assets/enemies_sheet_fixed.png";
    static constexpr const char* MISC = "assets/misc_fixed.png";
    static constexpr const char* ANIMALS = "assets/animals_fixed.png";
    static constexpr const char* TILES = "assets/tiles_first_map_fixed.png";
    static constexpr const char* BACKGROUND = "assets/background_foreground64.png";
};

} // namespace app

#endif // SPRITE_SHEET_CONFIG_HPP
