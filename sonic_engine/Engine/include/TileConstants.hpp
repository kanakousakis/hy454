#pragma once

#include "Types.hpp"

namespace engine {

// Game tile dimensions (power of 2 for collision grid)
constexpr Dim TILE_WIDTH = 128;
constexpr Dim TILE_HEIGHT = 128;

// Source tileset layout (tiles_first_map_fixed.png is 912x2000 pixels)
// The tileset contains 5x11 tiles at approximately 182x182 pixels each
constexpr Dim TILESET_WIDTH = 5;     // Columns in tileset
constexpr Dim TILESET_HEIGHT = 11;   // Rows in tileset
constexpr Dim TILESET_TILE_WIDTH = 182;   // Actual pixel width of tiles in source image
constexpr Dim TILESET_TILE_HEIGHT = 182;  // Actual pixel height of tiles in source image

// Grid element dimensions (must divide evenly into tile size)
// 128 / 8 = 16 grid elements per tile
constexpr Dim GRID_ELEMENT_WIDTH = 8;
constexpr Dim GRID_ELEMENT_HEIGHT = 8;

// Compile-time validation
static_assert(TILE_WIDTH % GRID_ELEMENT_WIDTH == 0, 
    "TILE_WIDTH must be divisible by GRID_ELEMENT_WIDTH");
static_assert(TILE_HEIGHT % GRID_ELEMENT_HEIGHT == 0, 
    "TILE_HEIGHT must be divisible by GRID_ELEMENT_HEIGHT");

// Derived constants
constexpr Dim GRID_BLOCK_COLUMNS = TILE_WIDTH / GRID_ELEMENT_WIDTH;
constexpr Dim GRID_BLOCK_ROWS = TILE_HEIGHT / GRID_ELEMENT_HEIGHT;
constexpr Dim GRID_ELEMENTS_PER_TILE = GRID_BLOCK_ROWS * GRID_BLOCK_COLUMNS;

// Bit shift values for tile operations (log2 of dimensions)
constexpr int TILE_WIDTH_SHIFT = 7;   // 2^7 = 128
constexpr int TILE_HEIGHT_SHIFT = 7;
constexpr int TILE_WIDTH_MASK = TILE_WIDTH - 1;   // 127
constexpr int TILE_HEIGHT_MASK = TILE_HEIGHT - 1;

// Fast tile coordinate operations using bit shifts
inline int MulTileWidth(int i)  { return i << TILE_WIDTH_SHIFT; }
inline int MulTileHeight(int i) { return i << TILE_HEIGHT_SHIFT; }
inline int DivTileWidth(int i)  { return i >> TILE_WIDTH_SHIFT; }
inline int DivTileHeight(int i) { return i >> TILE_HEIGHT_SHIFT; }
inline int ModTileWidth(int i)  { return i & TILE_WIDTH_MASK; }
inline int ModTileHeight(int i) { return i & TILE_HEIGHT_MASK; }

// Grid element operations (not power of 2, use division)
inline int DivGridWidth(int i)  { return i / GRID_ELEMENT_WIDTH; }
inline int DivGridHeight(int i) { return i / GRID_ELEMENT_HEIGHT; }
inline int MulGridWidth(int i)  { return i * GRID_ELEMENT_WIDTH; }
inline int MulGridHeight(int i) { return i * GRID_ELEMENT_HEIGHT; }

// Grid collision flags
constexpr GridIndex GRID_EMPTY        = 0x00;
constexpr GridIndex GRID_SOLID        = 0x0F;  // All directions
constexpr GridIndex GRID_LEFT_SOLID   = 0x01;
constexpr GridIndex GRID_RIGHT_SOLID  = 0x02;
constexpr GridIndex GRID_TOP_SOLID    = 0x04;  // Platform - can pass from below
constexpr GridIndex GRID_BOTTOM_SOLID = 0x08;
constexpr GridIndex GRID_GROUND       = 0x10;

// Tile index type (supports up to 256 tiles in 16x16 tileset)
using TileIndex = unsigned short;

// Tile index encoding/decoding for 16x16 tileset
inline TileIndex MakeTileIndex(int row, int col) {
    return static_cast<TileIndex>((col << 4) | row);
}
inline int GetTileRow(TileIndex idx) { return idx & 0x0F; }
inline int GetTileCol(TileIndex idx) { return (idx >> 4) & 0x0F; }

// Get pixel position in tileset
inline Dim TileXInSet(TileIndex idx) { return MulTileWidth(GetTileCol(idx)); }
inline Dim TileYInSet(TileIndex idx) { return MulTileHeight(GetTileRow(idx)); }

} // namespace engine
