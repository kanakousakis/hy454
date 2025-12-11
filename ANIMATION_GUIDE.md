# Sonic Animation System Guide - v14

## Controls
| Key | Action |
|-----|--------|
| Left/Right or A/D | Move |
| Space, Up, W | Jump |
| Down/S (still) | Crouch |
| Down/S (moving) | Roll |
| Down + Jump (crouching) | Spindash charge |
| Release Down (spindash) | Release spindash |
| Left Mouse Click (still) | Look Up |
| G | Toggle grid |
| F1 | Toggle debug |
| ESC | Pause |
| Q | Quit |

## Major Fix in v14: Spring Landing Bug

### The Problem
Sonic was getting stuck in Walk→Idle loop after landing from springs.
He couldn't move and would oscillate between states.

### Root Cause
The collision system treated ground tiles as walls!

When Sonic stood on GRID_SOLID ground, his bounding box overlapped the 
ground tile at his feet. Moving horizontally detected the solid ground 
as a "wall" and zeroed groundSpeed.

### The Fix
Use a **shorter collision box** for horizontal checks that excludes the 
bottom 12 pixels (feet area). Applied in 3 locations:

1. `UpdatePhysics()` - Main movement filtering
2. `ResolveWallOverlap()` - Wall push-out function
3. `CheckWallCollision()` - Additional wall check

This prevents ground tiles from being detected as horizontal walls while
still allowing proper wall collision with actual vertical walls.

## Other Fixes in v13-v14

1. **Monitors breakable by walking** (not just jumping)
2. **Death animation** uses `sonic_death` (not continue)
3. **Landing state** can be interrupted by input
4. **Balance detection** more aggressive

## All States and Animations

| State | Animation | Description |
|-------|-----------|-------------|
| Idle | sonic_idle | Standing still |
| Bored | sonic_bored | After 5 seconds |
| LookingUp | sonic_lookup | Mouse click |
| Crouching | sonic_crouch | Down key |
| Spindash | sonic_spindash | Charging |
| Walking | sonic_walk | Low speed |
| Running | sonic_walk | Medium speed |
| FullSpeed | sonic_run | Top speed |
| Jumping | sonic_ball | In air |
| Rolling | sonic_ball | Ball on ground |
| Skidding | sonic_skid | Braking |
| Balancing | sonic_balance | On edge |
| Pushing | sonic_push | Against wall |
| Spring | sonic_spring | Bounced up |
| Landing | sonic_idle | Brief transition |
| Hurt | sonic_hurt | Taking damage |
| Dead | sonic_death | Death pose |

## Known Limitation

The collision fix is a workaround. Sonic might clip into very low walls 
(<12 pixels tall). The proper fix would be to use GRID_TOP_SOLID for 
ground tiles instead of GRID_SOLID, or implement sensor-based collision 
like the original Sonic games.
