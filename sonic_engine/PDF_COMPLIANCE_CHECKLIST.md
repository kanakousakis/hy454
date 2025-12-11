# Sonic Engine - PDF Compliance Checklist
## CS-454 Project - University of Crete

## ✅ ALL 6 SPRITE SHEETS LOADED (in assets/ folder)

| File | Size | Contents |
|------|------|----------|
| sonic_sheet_fixed.png | 658x931 | 18+ Sonic animations |
| enemies_sheet_fixed.png | 1398x898 | 15 enemy types |
| misc_fixed.png | 676x476 | Rings, Monitors, Springs |
| animals_fixed.png | 96x334 | 6 animal types |
| tiles_first_map_fixed.png | 912x2000 | Terrain tiles |
| background_foreground64.png | 704x640 | Parallax layers |

## ✅ SONIC ANIMATIONS (18 sets)

- sonic_idle, sonic_bored, sonic_walk, sonic_run
- sonic_ball, sonic_skid, sonic_push, sonic_hurt
- sonic_death, sonic_spring, sonic_lookup, sonic_crouch
- sonic_balance, sonic_tunnel, sonic_gasp, sonic_pipe
- sonic_ending_bad, sonic_ending_good

## ✅ ALL 15 ENEMY TYPES

| Enemy | Behavior | Points |
|-------|----------|--------|
| Motobug | Patrol left/right | 100 |
| Crabmeat | Walk + shoot claws | 100 |
| BuzzBomber | Fly + shoot | 100 |
| Masher | Jump from water | 100 |
| Newtron Blue | Appear + shoot | 100 |
| Newtron Green | Fly at player | 100 |
| Bomb | Walk + explode | 100 |
| Caterkiller | Segmented crawl | 100 |
| Batbrain | Hang + swoop | 100 |
| Burrobot | Dig + jump | 100 |
| Roller | Walk + roll into ball | 100 |
| Jaws | Swim in water | 100 |
| BallHog | Throw bombs | 100 |
| Orbinaut | Float + spike balls | 100 |
| SpikesEnemy | Walk + hide in shell | 100 |

## ✅ POWER-UPS & ITEMS

- Ring (4-frame spin animation)
- Big Ring (special stage)
- Shield Monitor (30 sec, flicker effect)
- Invincibility Monitor (20 sec, sparkle effect)
- Speed Monitor (10 sec)
- Extra Life Monitor
- Rings Monitor (+10)
- Eggman Monitor (trap)
- Springs (yellow/red, normal/diagonal)
- Spikes (4 directions)
- Checkpoint (8-frame animation)

## ✅ ANIMALS (6 types)

- Flicky (blue bird)
- Pocky (rabbit)
- Cucky (chicken)
- Ricky (squirrel)
- Pecky (penguin)
- Rocky (seal)

## ✅ ENGINE FEATURES

- Tile Map with CSV loading
- Grid collision (FilterGridMotion)
- Parallax scrolling
- Pause/Resume with TimeShift
- Scroll factor controls (+/-/0/Home/End)
- Debug overlay (F1, G)

## ✅ SONIC FEATURES

- All 8 states (Idle, Walk, Run, FullSpeed, Jump, Roll, Hurt, Dead)
- Ring scatter on damage (max 32, double circle)
- Invincibility sparkles (4 rotating stars)
- Shield flicker (1 on/1 off)
- Lives system
- Checkpoint respawn

## ⚠️ NEEDS YOUR INPUT

1. **Add your name** - Search for `[YOUR NAME HERE]` in main.cpp line ~850
2. **Create Tiled map** - Use tiles_first_map_fixed.png as tileset
3. **Test all enemies** - They're placed throughout the level

## 🎮 CONTROLS

| Key | Action |
|-----|--------|
| Arrow/WASD | Move |
| Space/W/Up | Jump |
| + / = | Increase scroll factor |
| - | Decrease scroll factor |
| 0 | Reset scroll factor |
| Home | Scroll to start |
| End | Scroll to end |
| G | Toggle grid view |
| F1 | Toggle debug info |
| ESC | Pause menu |
| Q | Quit |
