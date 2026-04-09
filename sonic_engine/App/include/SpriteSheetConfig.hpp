#ifndef SPRITE_SHEET_CONFIG_HPP
#define SPRITE_SHEET_CONFIG_HPP

#include "Engine.hpp"
#include <vector>
#include <string>
#include <map>

namespace app {

//frame rectangle from sprite sheet
struct FrameRect {
    int x, y, w, h;
};

//animation definition with frames
struct AnimationDef {
    std::string id;
    std::vector<FrameRect> frames;
    bool loop = true;
    unsigned delay = 100;  //ms per frame
};

//============================================================
//SONIC SPRITE SHEET - sonic_sheet_fixed.png
//sheet size: 658 x 931 pixels
//background: Green (needs color key)
//layout based on visible labels in sprite sheet
//============================================================

class SonicSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 147;
    static constexpr unsigned char COLOR_KEY_G = 187;
    static constexpr unsigned char COLOR_KEY_B = 148;
    
    static std::vector<AnimationDef> GetAnimations() {
        std::vector<AnimationDef> anims;
        
                        
//=== ROW 0: Idle, Bored, Loop, Looking Up, Crouch, Spring ===
        anims.push_back({"sonic_idle", {{27, 39, 30, 40}}, false, 100});
        
//bored START - wide eyes stare for 1 second, then foot tap loop
        anims.push_back({"sonic_bored_stare", {
            {183, 39, 31, 40}  //wide eyes staring at camera - 1000ms
        }, false, 1700});
        
//bored LOOP - foot tapping (loops after stare completes)
        anims.push_back({"sonic_bored", {
            {113, 39, 31, 40},  //foot tap position 1
            {323, 39, 31, 40}  //foot tap position 2
        }, true, 500});  //500ms per frame
        
        anims.push_back({"sonic_loop", {
            {253, 39, 31, 41},
            {323, 39, 31, 40}
        }, true, 60});
        
        anims.push_back({"sonic_lookup", {{408, 39, 32, 40}}, false, 100});          
//crouch shows Sonic curled into ball shape (this sprite sheet doesn't have a ducking pose)
        anims.push_back({"sonic_crouch", {
            {492, 48, 37, 31}  //curled up ball shape
        }, false, 80});
        
//spindash: alternates between curled pose and ball frames for "revving" effect
        anims.push_back({"sonic_spindash", {
            {492, 48, 37, 31},  //curled up (from Row 0)
            {27, 407, 31, 32},  //ball frame 1
            {97, 407, 32, 31},  //ball frame 2
            {168, 407, 31, 32},  //ball frame 3
        }, true, 60});  //fast loop for revving effect
        
        anims.push_back({"sonic_spring", {{580, 35, 26, 47}}, false, 100});          
//=== ROW 1: Walk (6 frames), Skid (2 frames) ===
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
        
//=== ROW 2: Walk Angled (6 frames), Balance (2 frames) ===
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
        
//=== ROW 3: Full Speed (4 frames), Full Speed Angled (4 frames) ===
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
        
//=== ROW 4: Ball/Jump (5 frames), Push (4 frames) ===
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
        }, true, 400});
        
//=== ROW 5: Pipe Cling (2), Tunnel Spin (5), Air Gasp (1) ===
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
        
//=== ROW 6: Hurt (2), Drown, Death, Continue (4+) ===
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
        
//=== ROW 7: Endings ===
        anims.push_back({"sonic_ending_bad", {
            {27, 711, 31, 41},
            {94, 712, 32, 39}
        }, true, 200});
        
        anims.push_back({"sonic_ending_good", {
            {181, 710, 30, 41},
            {250, 710, 31, 42}
        }, true, 200});
        
//additional loop frames from Row 7 (for spin loops)
        anims.push_back({"sonic_loop_extended", {
            {320, 711, 32, 40},
            {390, 710, 32, 42},
            {460, 710, 32, 41},
            {530, 712, 32, 39}
        }, true, 50});
        
        return anims;
    }
};

//============================================================
//ENEMIES SPRITE SHEET - enemies_sheet_fixed.png
//sheet size: 1398 x 898 pixels
//background color: Dark Green (color key applied)
//LAYOUT (based on visible labels in sprite sheet):
//TOP-LEFT: Crabmeat/Ganigani (red crab) - rows of walk/attack frames
//BELOW CRABMEAT: Buzz Bomber/Beeton (blue wasp)
//RIGHT OF BUZZBOMBER: Motobug/Motora (red ladybug)
//BELOW MOTOBUG: Masher/Batabata (purple piranha)
//CENTER: Various enemies (Caterkiller, Spikes, etc.)
//RIGHT SIDE: Orbinaut, BallHog, Batbrain, Burrobot, etc.
//============================================================

class EnemySpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 128;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
//=== CRABMEAT / GANIGANI ===
//red crab - TOP LEFT of sprite sheet
//each frame is approximately 48x32 pixels
    static std::vector<AnimationDef> GetCrabmeatAnimations() {
        return {
            {"crabmeat_walk", {
                {8, 29, 48, 32},
                {64, 29, 48, 32},
                {120, 29, 48, 32}
            }, true, 180},
            {"crabmeat_attack", {
                {120, 25, 48, 40},
                {176, 25, 48, 40}
            }, true, 150}
        };
    }
    
//=== BUZZ BOMBER / BEETON ===
//blue wasp - BELOW Crabmeat section (starts around y=85)
//each frame approximately 48x32 pixels
    static std::vector<AnimationDef> GetBuzzBomberAnimations() {
        return {
            {"buzzbomber_fly", {
                {8, 174, 48, 32},  //row 1: Wings up
                {64, 174, 48, 32},  //row 1: Wings down
                {8, 214, 48, 24},  //row 2: Wings up (shorter)
                {64, 214, 48, 24}  //row 2: Wings down (shorter)
            }, true, 100},
            {"buzzbomber_shoot", {
                {72, 246, 56, 56}  //stinger out frame - larger to show extended stinger
            }, false, 100}
        };
    }
    
//=== MOTOBUG / MOTORA ===
//red ladybug on wheel - RIGHT of Buzz Bomber (starts around x=155)
//each frame approximately 40x30 pixels
    static std::vector<AnimationDef> GetMotobugAnimations() {
        return {
            {"motobug_move", {
                {157, 102, 40, 32},
                {213, 102, 40, 32}
            }, true, 100}
        };
    }
    
//=== MASHER / BATABATA ===
//purple piranha - BELOW Motobug section (starts around y=188)
//each frame approximately 32x40 pixels
    static std::vector<AnimationDef> GetMasherAnimations() {
        return {
            {"masher_idle", {
                {157, 210, 30, 32}
            }, false, 100},
            {"masher_jump", {
                {157, 210, 30, 32},
                {197, 210, 30, 32}
            }, true, 80}
        };
    }
    
//=== BLUE NEWTRON / MELEON ===
//chameleon that appears - around y=250 area
    static std::vector<AnimationDef> GetNewtronBlueAnimations() {
        return {
            {"newtron_blue_appear", {
                {7, 250, 24, 36},  //appearing frame 1
                {35, 250, 36, 36},  //appearing frame 2
                {75, 250, 48, 36}  //fully appeared
            }, false, 100},
            {"newtron_blue_idle", {
                {75, 250, 48, 36}  //idle pose
            }, false, 100}
        };
    }
    
//=== GREEN NEWTRON ===
//newtron that flies - around y=330 area
    static std::vector<AnimationDef> GetNewtronGreenAnimations() {
        return {
            {"newtron_green_fly", {
                {7, 330, 48, 32},  //fly frame 1
                {57, 330, 48, 32}  //fly frame 2
            }, true, 80}
        };
    }
    
//=== BOMB ===
//round bomb enemy - around y=590 area (lower section)
    static std::vector<AnimationDef> GetBombAnimations() {
        return {
            {"bomb_walk", {
                {7, 590, 32, 32},  //walk frame 1
                {41, 590, 32, 32},  //walk frame 2
                {75, 590, 32, 32},  //walk frame 3
                {109, 590, 32, 32}  //walk frame 4
            }, true, 120},
            {"bomb_explode", {
                {7, 650, 40, 40},  //explosion 1
                {49, 650, 48, 48},  //explosion 2
                {99, 650, 56, 56}  //explosion 3
            }, false, 80}
        };
    }
    
//=== CATERKILLER / NAL ===
//segmented caterpillar - center area around x=324
    static std::vector<AnimationDef> GetCaterkillerAnimations() {
        return {
            {"caterkiller_move", {
                {324, 10, 64, 24},  //body stretched
                {324, 38, 64, 24}  //body bunched
            }, true, 150},
            {"caterkiller_segment", {
                {324, 66, 16, 16}  //single segment
            }, false, 100}
        };
    }
    
//=== SPIKES / YADORIN ===
//hermit crab - around x=324, y=178
    static std::vector<AnimationDef> GetSpikesEnemyAnimations() {
        return {
            {"spikes_enemy_walk", {
                {324, 178, 32, 32},  //walk frame 1
                {358, 178, 32, 32}  //walk frame 2
            }, true, 150},
            {"spikes_enemy_hide", {
                {392, 178, 24, 24}  //hidden in shell
            }, false, 100}
        };
    }
    
//=== BATBRAIN / BASARAN ===
//bat enemy - around x=484, y=10
    static std::vector<AnimationDef> GetBatbrainAnimations() {
        return {
            {"batbrain_hang", {
                {484, 10, 24, 24}  //hanging
            }, false, 100},
            {"batbrain_fly", {
                {484, 38, 32, 24},  //flying frame 1
                {518, 38, 32, 24}  //flying frame 2
            }, true, 80}
        };
    }
    
//=== BURROBOT / MOGURIN ===
//mole robot - around x=630, y=10
    static std::vector<AnimationDef> GetBurrobotAnimations() {
        return {
            {"burrobot_dig", {
                {630, 10, 32, 32},  //dig frame 1
                {664, 10, 32, 32}  //dig frame 2
            }, true, 100},
            {"burrobot_jump", {
                {630, 46, 32, 40},  //jump frame 1
                {664, 46, 32, 40}  //jump frame 2
            }, true, 80}
        };
    }
    
//=== ROLLER / ARMA ===
//armadillo - around x=484, y=406
    static std::vector<AnimationDef> GetRollerAnimations() {
        return {
            {"roller_walk", {
                {484, 406, 40, 32},  //walk frame 1
                {526, 406, 40, 32}  //walk frame 2
            }, true, 120},
            {"roller_ball", {
                {484, 440, 32, 32},  //ball frame 1
                {518, 440, 32, 32},  //ball frame 2
                {552, 440, 32, 32}  //ball frame 3
            }, true, 50}
        };
    }
    
//=== JAWS / PUKU-PUKU ===
//fish enemy - around x=630, y=462
    static std::vector<AnimationDef> GetJawsAnimations() {
        return {
            {"jaws_swim", {
                {630, 462, 32, 24},  //swim frame 1
                {664, 462, 32, 24}  //swim frame 2
            }, true, 100}
        };
    }
    
//=== ORBINAUT / UNIDUS ===
//floating spiky enemy - around x=772, y=10
    static std::vector<AnimationDef> GetOrbinautAnimations() {
        return {
            {"orbinaut_float", {
                {772, 10, 32, 32},  //float frame 1
                {806, 10, 32, 32}  //float frame 2
            }, true, 120},
            {"orbinaut_spike", {
                {772, 46, 16, 16}  //spike ball
            }, false, 100}
        };
    }
    
//=== BALL HOG / TON-TON ===
//pig that throws bombs - around x=772, y=482
    static std::vector<AnimationDef> GetBallHogAnimations() {
        return {
            {"ballhog_idle", {
                {772, 482, 32, 40}  //standing idle
            }, false, 100},
            {"ballhog_throw", {
                {806, 482, 40, 40},  //throw frame 1
                {848, 482, 40, 40}  //throw frame 2
            }, false, 100}
        };
    }
    
//=== PROJECTILES ===
//small projectile sprites used by shooting enemies
    static FrameRect GetCrabmeatProjectile() { return {65, 52, 8, 8}; }
    static FrameRect GetBuzzBomberProjectile() { return {110, 130, 12, 12}; }
    static FrameRect GetBombProjectile() { return {145, 590, 16, 16}; }
    
//=== EXPLOSION (shared by all enemies) ===
    static AnimationDef GetExplosionAnimation() {
        return {"explosion", {
            {7, 720, 32, 32},  //small puff
            {41, 720, 40, 40},  //medium puff
            {83, 720, 48, 48},  //large puff
            {133, 720, 48, 48}  //final puff
        }, false, 60};
    }
};

//============================================================
//MISC SPRITE SHEET - misc_fixed.png
//sheet size: 676 x 476 pixels
//contains: Rings, Spikes, Monitors, Checkpoints, Springs, etc.
//background: RGB(0, 128, 0) green
//============================================================

class MiscSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 255;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
//=== RINGS (from original Sonic sprite sheet) ===
//first 4 frames: Rotation animation (loops continuously)
    static AnimationDef GetRingAnimation() {
        return {"ring", {
            {8, 25, 16, 16},  //frame 1 - full circle (front view)
            {32, 25, 16, 16},  //frame 2 - tilted ellipse
            {56, 25, 8, 16},  //frame 3 - very narrow (side view, 8px wide)
            {72, 25, 16, 16}  //frame 4 - tilted other direction
        }, true, 80};  //loop forever, 80ms per frame
    }

//last 4 frames: Collection sparkle (super fast glow effect)
//plays once when ring is collected, then disappears
    static AnimationDef GetRingCollectAnimation() {
        return {"ring_collect", {
            {96, 25, 16, 16},  //glow frame 1
            {120, 25, 16, 16},  //glow frame 2
            {144, 25, 16, 16},  //glow frame 3
            {168, 25, 16, 16}  //glow frame 4
        }, false, 60};  //don't loop, 60ms per frame = 240ms total (0.24 sec)
    }
    
//=== BIG RING / OKINA RING (Special stage entrance / Level complete) ===
//coordinates from sprite sheet - 4 frame spinning animation
    static AnimationDef GetBigRingAnimation() {
        return {"big_ring", {
            {8, 66, 63, 63},  //frame 1: full ring facing front
            {80, 66, 47, 63},  //frame 2: turning
            {136, 66, 23, 63},  //frame 3: edge view (thin)
            {168, 66, 47, 63}  //frame 4: turning back
        }, true, 150};  //loop, 150ms per frame
    }
    
//=== SPIKES (Terrain hazards) ===
    static FrameRect GetSpikesUp()    { return {308, 25, 40, 32}; }
    static FrameRect GetSpikesDown()  { return {308, 25, 40, 32}; }
    static FrameRect GetSpikesLeft()  { return {308, 25, 32, 40}; }
    static FrameRect GetSpikesRight() { return {308, 25, 32, 40}; }
    
//=== SCORE POPUPS ===
    static FrameRect GetScore100()   { return {258, 170, 24, 12}; }
    static FrameRect GetScore200()   { return {290, 170, 24, 12}; }
    static FrameRect GetScore500()   { return {322, 170, 24, 12}; }
    static FrameRect GetScore1000()  { return {354, 170, 32, 12}; }
    static FrameRect GetScore10000() { return {394, 170, 40, 12}; }
    
//=== INVINCIBILITY SPARKLES (Stars rotating around Sonic) ===
    static AnimationDef GetInvincibilityAnimation() {
        return {"invincibility", {
            {469, 25, 16, 16},
            {493, 25, 16, 16},
            {517, 25, 16, 16},
            {541, 25, 16, 16}
        }, true, 50};
    }
    
//=== SHIELD BUBBLE (3 frames in a row that cycle) ===
    static AnimationDef GetShieldAnimation() {
        return {"shield", {
            {456, 110, 47, 47},  //frame 1: first shield
            {512, 110, 47, 47},  //frame 2: second shield
            {568, 110, 47, 47}  //frame 3: third shield
        }, true, 100};  //100ms per frame for smooth rotation
    }
    
//=== MONITORS (Dami-Karv verified coordinates) ===
//unbroken box: 8, 312, 32, 32
//broken box: 48, 328, 32, 16
    static FrameRect GetMonitorUnbroken()   { return {8, 312, 32, 32}; }
    static FrameRect GetMonitorBroken()     { return {48, 328, 32, 16}; }
    
//monitor icons (drawn on top of box)
    static FrameRect GetIconRing()       { return {80, 367, 16, 16}; }
    static FrameRect GetIconSpeed()      { return {8, 391, 16, 16}; }
    static FrameRect GetIconShield()     { return {32, 391, 16, 16}; }
    static FrameRect GetIconInvincible() { return {56, 391, 16, 16}; }
    static FrameRect GetIconLife()       { return {80, 391, 16, 16}; }
    static FrameRect GetIconEggman()     { return {32, 431, 16, 14}; }  //eggman trap icon from misc.png
    
//legacy compatibility
    static FrameRect GetMonitorEmpty()      { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorRing()       { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorShield()     { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorInvincible() { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorSpeed()      { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorLife()       { return GetMonitorUnbroken(); }
    static FrameRect GetMonitorEggman()     { return GetMonitorUnbroken(); }
    
//=== CHECKPOINT (Sign post) - 16 frames
//row 1 (Y=313): 8 frames, Row 2 (Y=387): 8 frames
    static AnimationDef GetCheckpointAnimation() {
        return {"checkpoint", {
//row 1 (Y = 313)
            {142, 313, 32, 64},
            {182, 313, 32, 64},
            {222, 313, 40, 64},
            {270, 313, 40, 64},
            {318, 313, 40, 64},
            {366, 313, 40, 64},
            {414, 313, 40, 64},
            {462, 313, 32, 64},
//row 2 (Y = 387)
            {142, 387, 32, 64},
            {182, 387, 32, 64},
            {222, 387, 40, 64},
            {270, 387, 40, 64},
            {318, 387, 40, 64},
            {366, 387, 40, 64},
            {414, 387, 40, 64},
            {462, 387, 32, 64}
        }, false, 80};  //false = don't loop (plays once when activated)
    }
    
//=== SPRINGS ===
//yellow spring - 2 states: Normal (compressed) and Extended
    static AnimationDef GetYellowSpringAnimation() {
        return {"spring_yellow", {
            {554, 313, 32, 16},  //normal/Compressed
            {554, 313, 32, 16},  //compressed (same as normal for transition)
            {554, 337, 32, 32}  //extended
        }, false, 50};
    }
    
//red spring - stronger bounce
    static AnimationDef GetRedSpringAnimation() {
        return {"spring_red", {
            {588, 313, 32, 16},  //normal/Compressed
            {588, 313, 32, 16},  //compressed (same as normal for transition)
            {588, 337, 32, 32}  //extended
        }, false, 50};
    }
    
//side launch spring (horizontal) - from misc.png
//512,417 to 527,448 (not extended) = x:512, y:417, w:15, h:31
//536,417 to 567,448 (extended) = x:536, y:417, w:31, h:31
    static AnimationDef GetSideSpringAnimation() {
        return {"spring_side", {
            {512, 417, 16, 32},  //normal (not extended)
            {536, 417, 32, 32}  //extended
        }, false, 50};
    }
    
//diagonal springs
    static FrameRect GetDiagonalSpringYellowUpR() { return {634, 313, 32, 32}; }
    static FrameRect GetDiagonalSpringRedUpR()    { return {634, 353, 32, 32}; }
    
//=== EGGMAN TRAP (from misc.png) ===
//32,431 to 47,444 = x:32, y:431, w:15, h:13
    static FrameRect GetEggmanTrap() { return {32, 431, 16, 14}; }
    
//=== SWITCH (Button) ===
    static FrameRect GetSwitchUp()   { return {258, 233, 32, 16}; }
    static FrameRect GetSwitchDown() { return {258, 257, 32, 8}; }
    
//=== GAME OVER / TIME OVER TEXT ===
    static FrameRect GetGameOverText() { return {469, 193, 128, 16}; }
    static FrameRect GetTimeOverText() { return {469, 217, 128, 16}; }
    
//=== RING LOSS (Scattered ring - smaller frames) ===
    static AnimationDef GetRingLossAnimation() {
        return {"ring_loss", {
            {8, 25, 16, 16},
            {32, 25, 16, 16}
        }, true, 50};
    }
};

//============================================================
//ANIMALS SPRITE SHEET - animals_fixed.png
//sheet size: 96 x 334 pixels
//small animals freed when enemies are destroyed
//background: RGB(0, 128, 0) green
//============================================================

//============================================================
//ANIMALS - enemies_sheet_fixed.png
//all animals are 16x24 pixels, from the enemies sprite sheet
//released when enemies are destroyed
//============================================================

class AnimalSpriteConfig {
public:
    static constexpr unsigned char COLOR_KEY_R = 0;
    static constexpr unsigned char COLOR_KEY_G = 255;
    static constexpr unsigned char COLOR_KEY_B = 0;
    
//frame sizes calculated from your exact pixel coordinates
    
//=== FLICKY (Blue bird) - Row 1 ===
//based on: 17,37 to 32,60 | 41,41 to 56,56 | 65,41 to 80,56
    static AnimationDef GetFlickyAnimation() {
        return {"flicky", {
            {17, 37, 15, 23},  //frame 1: spawn pose (32-17=15, 60-37=23)
            {41, 41, 15, 15},  //frame 2: flying (56-41=15, 56-41=15)
            {65, 41, 15, 15}  //frame 3: flying
        }, true, 80};
    }

//=== CUCKY (Yellow chicken) - Row 2 ===
//same dimensions as Flicky, starts at y:81
    static AnimationDef GetCuckyAnimation() {
        return {"cucky", {
            {17, 81, 15, 23},  //frame 1: spawn pose
            {41, 85, 15, 15},  //frame 2: running
            {65, 85, 15, 15}  //frame 3: running
        }, true, 80};
    }

//=== POCKY (Rabbit) - Row 3 ===
//based on: 16,125 to 31,148 | 40,125 to 55,148 | 64,125 to 79,148
    static AnimationDef GetPockyAnimation() {
        return {"pocky", {
            {16, 125, 15, 23},  //frame 1: spawn pose (31-16=15, 148-125=23)
            {40, 125, 15, 23},  //frame 2: hopping
            {64, 125, 15, 23}  //frame 3: hopping
        }, true, 100};
    }

//=== RICKY (Squirrel/Crab - Row 4) ===
//based on: 8,169 to 23,192 | 32,173 to 55,188 | 64,173 to 87,188
    static AnimationDef GetRickyAnimation() {
        return {"ricky", {
            {8, 169, 15, 23},  //frame 1: spawn pose (23-8=15, 192-169=23)
            {32, 173, 23, 15},  //frame 2: running (55-32=23, 188-173=15)
            {64, 173, 23, 15}  //frame 3: running
        }, true, 100};
    }

//=== PECKY (Penguin) - Row 5 ===
//based on: 13,213 to 28,236 | 37,213 to 52,236 | 61,213 to 76,236
    static AnimationDef GetPeckyAnimation() {
        return {"pecky", {
            {13, 213, 15, 23},  //frame 1: spawn pose (28-13=15, 236-213=23)
            {37, 213, 15, 23},  //frame 2: waddling
            {61, 213, 15, 23}  //frame 3: waddling
        }, true, 100};
    }

    static AnimationDef GetRockyAnimation() {
        return {"rocky", {
            {13, 257, 15, 23},  //estimated row 6
            {37, 257, 15, 23},
            {61, 257, 15, 23}
        }, true, 100};
    }

    static AnimationDef GetPickyAnimation() {
        return {"picky", {
            {13, 301, 15, 23},  //estimated row 7
            {37, 301, 15, 23},
            {61, 301, 15, 23}
        }, true, 100};
    }
    
//get all animal animation definitions
    static std::vector<AnimationDef> GetAllAnimalAnimations() {
        return {
            GetFlickyAnimation(),
            GetPockyAnimation(),
            GetCuckyAnimation(),
            GetPeckyAnimation(),
            GetRockyAnimation(),
            GetPickyAnimation(),
            GetRickyAnimation()
        };
    }
};

//============================================================
//TILES - tiles_first_map_fixed.png
//sheet size: 912 x 2000 pixels
//green Hill Zone terrain tiles (approximately 5x11 grid)
//each tile is ~182x182 pixels (to be scaled to 128x128)
//pink/magenta areas are transparency
//============================================================

class TileConfig {
public:
//tile dimensions in the sprite sheet
    static constexpr int SHEET_TILE_SIZE = 182;
//target tile size for game (power of 2)
    static constexpr int GAME_TILE_SIZE = 128;
//grid layout
    static constexpr int TILES_PER_ROW = 5;
    static constexpr int TILES_PER_COL = 11;
    
//get source rectangle for a tile index
    static FrameRect GetTileRect(int index) {
        int col = index % TILES_PER_ROW;
        int row = index / TILES_PER_ROW;
        return {col * SHEET_TILE_SIZE, row * SHEET_TILE_SIZE, 
                SHEET_TILE_SIZE, SHEET_TILE_SIZE};
    }
    
//=== TILE INDICES (based on visual inspection of tileset) ===
//row 0: Ground variations with grass and palm trees
    static constexpr int TILE_GROUND_PALM_1 = 0;  //ground with palm tree
    static constexpr int TILE_GROUND_PALM_2 = 1;  //ground with different palm
    static constexpr int TILE_SLOPE_DOWN_R = 2;  //slope going down-right
    static constexpr int TILE_SLOPE_DOWN_L = 3;  //slope going down-left
    static constexpr int TILE_GROUND_EDGE = 4;  //ground right edge
    
//row 1: More terrain
    static constexpr int TILE_CLIFF_TOP = 5;
    static constexpr int TILE_UNDERGROUND = 6;  //brown checkered fill
    static constexpr int TILE_CLIFF_BOTTOM = 7;
    static constexpr int TILE_SLOPE_UP_R = 8;
    static constexpr int TILE_CORNER_OUT = 9;
    
//row 2: Platforms and edges
    static constexpr int TILE_PLATFORM_L = 10;
    static constexpr int TILE_PLATFORM_M = 11;
    static constexpr int TILE_PLATFORM_R = 12;
    static constexpr int TILE_GROUND_FLAT = 13;
    static constexpr int TILE_WATERFALL = 14;
    
//row 3: Loop sections (top parts)
    static constexpr int TILE_LOOP_TOP_L = 15;
    static constexpr int TILE_LOOP_TOP_M = 16;
    static constexpr int TILE_LOOP_TOP_R = 17;
    static constexpr int TILE_LOOP_SIDE = 18;
    static constexpr int TILE_LOOP_INNER = 19;
    
//row 4: Loop sections (bottom parts)
    static constexpr int TILE_LOOP_BOT_L = 20;
    static constexpr int TILE_LOOP_BOT_M = 21;
    static constexpr int TILE_LOOP_BOT_R = 22;
    static constexpr int TILE_TUNNEL = 23;
    static constexpr int TILE_BRIDGE = 24;
    
//remaining rows: More variations, decorations
    static constexpr int TILE_GRASS_TUFT = 50;
    static constexpr int TILE_FLOWERS = 51;
    
//transparency color (pink/magenta in the tileset)
    static constexpr unsigned char TRANS_R = 255;
    static constexpr unsigned char TRANS_G = 0;
    static constexpr unsigned char TRANS_B = 255;
    
//=== FLOWER ANIMATIONS (from flowers.png) ===
//these are decorative elements that appear at transparent tile positions
    
//type 1: Tall purple stem flowers (3 frames) - ROW 2 of flowers.png
    static AnimationDef GetFlowerTallAnimation() {
        return {"flower_tall", {
            {48, 48, 32, 40},  //frame 1
            {88, 48, 32, 40},  //frame 2
            {128, 48, 32, 40}  //frame 3
        }, true, 150};  //faster animation to prevent lag appearance
    }
    
//type 2: Short round sunflowers (2 frames) - ROW 1 of flowers.png
    static AnimationDef GetFlowerShortAnimation() {
        return {"flower_short", {
            {48, 8, 32, 32},  //frame 1
            {88, 8, 32, 32}  //frame 2
        }, true, 150};  //faster animation
    }
};

//============================================================
//BACKGROUND - background_foreground64.png
//sheet size: 704 x 640 pixels
//parallax background layers for Green Hill Zone
//multiple horizontal strips representing different depths
//============================================================

class BackgroundConfig {
public:
//layer definitions (from top to bottom of image)
//each layer is a horizontal strip that tiles/repeats
    
//sky with clouds (slowest movement)
    static FrameRect GetSkyLayer()        { return {0, 0, 640, 112}; }
    static constexpr float SKY_SCROLL = 0.1f;
    
//distant mountains/city silhouette
    static FrameRect GetMountainLayer()   { return {0, 112, 640, 48}; }
    static constexpr float MOUNTAIN_SCROLL = 0.2f;
    
//mid-ground hills with bushes
    static FrameRect GetHillsLayer()      { return {0, 160, 640, 64}; }
    static constexpr float HILLS_SCROLL = 0.4f;
    
//near hills (faster parallax)
    static FrameRect GetNearHillsLayer()  { return {0, 224, 640, 64}; }
    static constexpr float NEAR_HILLS_SCROLL = 0.6f;
    
//checkerboard/ground decoration layer
    static FrameRect GetGroundDecor()     { return {0, 288, 640, 64}; }
    static constexpr float GROUND_DECOR_SCROLL = 0.8f;
    
//water surface (for water areas)
    static FrameRect GetWaterSurface()    { return {0, 352, 640, 32}; }
    
//water body (animated)
    static AnimationDef GetWaterAnimation() {
        return {"water", {
            {0, 384, 640, 64},
            {0, 448, 640, 64},
            {0, 512, 640, 64}
        }, true, 150};
    }
    
//extra tiles on right side
    static FrameRect GetWaterTile()       { return {640, 0, 64, 64}; }
    static FrameRect GetSkyTile()         { return {640, 64, 64, 64}; }
};

//============================================================
//HELPER: All sprite sheet file paths
//============================================================

class SpriteSheetPaths {
public:
    static constexpr const char* SONIC = "assets/sonic_sheet_fixed.png";
    static constexpr const char* ENEMIES = "assets/enemies_sheet_fixed.png";
    static constexpr const char* MISC = "assets/misc_fixed.png";
    static constexpr const char* ANIMALS = "assets/animals_fixed.png";
    static constexpr const char* TILES = "assets/tiles_first_map_fixed.png";
    static constexpr const char* BACKGROUND = "assets/background_foreground64.png";
};

}  //namespace app

#endif  //SPRITE_SHEET_CONFIG_HPP
