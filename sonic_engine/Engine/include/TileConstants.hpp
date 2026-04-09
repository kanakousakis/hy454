#pragma once

#include "Types.hpp"

namespace engine {

//============================================================================
//VISUAL TILE DIMENSIONS (256×256 with 8px margin and spacing)
//============================================================================
constexpr Dim TILE_WIDTH = 256;
constexpr Dim TILE_HEIGHT = 256;

//source tileset layout (Map_Tilesheet.png)
//image: 1328×2912 pixels
//tiles: 256×256 with 8px margin and 8px spacing
//layout: 5 columns × 11 rows = 55 tiles
constexpr Dim TILESET_WIDTH = 5;  //columns in tileset
constexpr Dim TILESET_HEIGHT = 11;  //rows in tileset
constexpr Dim TILESET_TILE_WIDTH = 256;  //pixel width of each tile
constexpr Dim TILESET_TILE_HEIGHT = 256;  //pixel height of each tile
constexpr Dim TILESET_MARGIN = 8;  //margin from edge of image
constexpr Dim TILESET_SPACING = 8;  //spacing between tiles

//============================================================================
//COLLISION GRID DIMENSIONS (8×8 pixels per cell)
//============================================================================
constexpr Dim GRID_ELEMENT_WIDTH = 8;
constexpr Dim GRID_ELEMENT_HEIGHT = 8;

//derived: grid cells per visual tile
constexpr Dim GRID_CELLS_PER_TILE_X = TILE_WIDTH / GRID_ELEMENT_WIDTH;  //32
constexpr Dim GRID_CELLS_PER_TILE_Y = TILE_HEIGHT / GRID_ELEMENT_HEIGHT;  //32

//compile-time validation
static_assert(TILE_WIDTH % GRID_ELEMENT_WIDTH == 0, 
    "TILE_WIDTH must be divisible by GRID_ELEMENT_WIDTH");
static_assert(TILE_HEIGHT % GRID_ELEMENT_HEIGHT == 0, 
    "TILE_HEIGHT must be divisible by GRID_ELEMENT_HEIGHT");

//============================================================================
//COLLISION TYPES (from Tiled collision_tiles.png)
//============================================================================
//tiled CSV uses 1-based indices, -1 = empty
//after conversion: 0=empty, 1=solid, 2=platform, 3=slope, 4=loop, 5=tunnel, 6=death
constexpr GridIndex GRID_EMPTY     = 0;  //air - pass through
constexpr GridIndex GRID_SOLID     = 1;  //solid block - blocks all directions
constexpr GridIndex GRID_PLATFORM  = 2;  //platform - pass from below, solid on top
constexpr GridIndex GRID_SLOPE     = 3;  //slope - smooth walk, step up/down
constexpr GridIndex GRID_LOOP      = 4;  //loop - special gravity handling
constexpr GridIndex GRID_TUNNEL    = 5;  //tunnel - force ball mode
constexpr GridIndex GRID_DEATH     = 6;  //death pit - kill after delay

//legacy flags for compatibility (can be removed later)
constexpr GridIndex GRID_LEFT_SOLID   = 0x01;
constexpr GridIndex GRID_RIGHT_SOLID  = 0x02;
constexpr GridIndex GRID_TOP_SOLID    = 0x04;
constexpr GridIndex GRID_BOTTOM_SOLID = 0x08;
constexpr GridIndex GRID_GROUND       = 0x10;

//============================================================================
//STEP UP/DOWN THRESHOLD
//============================================================================
//1 grid cell = 8 pixels - small lips/edges should be ignored
constexpr int STEP_HEIGHT = 8;  //1 grid cell

//============================================================================
//BIT SHIFT OPERATIONS (for 256-pixel tiles, 2^8 = 256)
//============================================================================
constexpr int TILE_WIDTH_SHIFT = 8;
constexpr int TILE_HEIGHT_SHIFT = 8;
constexpr int TILE_WIDTH_MASK = TILE_WIDTH - 1;  //255
constexpr int TILE_HEIGHT_MASK = TILE_HEIGHT - 1;

//fast tile coordinate operations
inline int MulTileWidth(int i)  { return i << TILE_WIDTH_SHIFT; }
inline int MulTileHeight(int i) { return i << TILE_HEIGHT_SHIFT; }
inline int DivTileWidth(int i)  { return i >> TILE_WIDTH_SHIFT; }
inline int DivTileHeight(int i) { return i >> TILE_HEIGHT_SHIFT; }
inline int ModTileWidth(int i)  { return i & TILE_WIDTH_MASK; }
inline int ModTileHeight(int i) { return i & TILE_HEIGHT_MASK; }

//grid element operations (8 = 2^3, can use shifts)
constexpr int GRID_WIDTH_SHIFT = 3;
constexpr int GRID_HEIGHT_SHIFT = 3;
constexpr int GRID_WIDTH_MASK = GRID_ELEMENT_WIDTH - 1;
constexpr int GRID_HEIGHT_MASK = GRID_ELEMENT_HEIGHT - 1;

inline int DivGridWidth(int i)  { return i >> GRID_WIDTH_SHIFT; }
inline int DivGridHeight(int i) { return i >> GRID_HEIGHT_SHIFT; }
inline int MulGridWidth(int i)  { return i << GRID_WIDTH_SHIFT; }
inline int MulGridHeight(int i) { return i << GRID_HEIGHT_SHIFT; }

//tile index type
using TileIndex = unsigned short;

//get tile source position in tileset (with margin and spacing)
inline int GetTileSourceX(int tileCol) {
    return TILESET_MARGIN + tileCol * (TILESET_TILE_WIDTH + TILESET_SPACING);
}
inline int GetTileSourceY(int tileRow) {
    return TILESET_MARGIN + tileRow * (TILESET_TILE_HEIGHT + TILESET_SPACING);
}

}  //namespace engine
