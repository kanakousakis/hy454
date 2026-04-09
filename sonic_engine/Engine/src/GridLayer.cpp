#include "GridLayer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace engine {

GridLayer::GridLayer(Dim rows, Dim cols) {
    Create(rows, cols);
}

bool GridLayer::Create(Dim rows, Dim cols) {
    totalRows = rows;
    totalCols = cols;
    grid.resize(rows * cols, GRID_EMPTY);
    return true;
}

void GridLayer::SetTile(Dim col, Dim row, GridIndex idx) {
    if (col < totalCols && row < totalRows) {
        grid[row * totalCols + col] = idx;
    }
}

GridIndex GridLayer::GetTile(Dim col, Dim row) const {
    if (col < totalCols && row < totalRows) {
        return grid[row * totalCols + col];
    }
//out of bounds: sides = solid, below = empty (fall into void)
    if (row >= totalRows) {
        return GRID_EMPTY;
    }
    return GRID_SOLID;
}

GridIndex GridLayer::GetTileType(int col, int row) const {
    return GetTile(static_cast<Dim>(col), static_cast<Dim>(row));
}

bool GridLayer::IsSolidAt(int col, int row) const {
    GridIndex tile = GetTileType(col, row);
//solid blocks horizontal and vertical movement
    return tile == GRID_SOLID;
}

bool GridLayer::IsGroundAt(int col, int row) const {
    GridIndex tile = GetTileType(col, row);
//ground = solid, platform (from above), slope, loop
//TUNNEL is NOT ground - Sonic passes through it
    return tile == GRID_SOLID || tile == GRID_PLATFORM || tile == GRID_SLOPE ||
           tile == GRID_LOOP;
}

//find the Y position of ground below a given pixel position
int GridLayer::FindGroundY(int pixelX, int startY, int maxDist) const {
    int col = DivGridWidth(pixelX);
    int startRow = DivGridHeight(startY);
    int maxRow = DivGridHeight(startY + maxDist);
    
    for (int row = startRow; row <= maxRow && row < static_cast<int>(totalRows); ++row) {
        if (IsGroundAt(col, row)) {
            return MulGridHeight(row);  //top of this grid cell
        }
    }
    return startY + maxDist;  //no ground found
}

int GridLayer::FindGroundBelow(int pixelX, int pixelY, int maxDist) const {
    return FindGroundY(pixelX, pixelY, maxDist);
}

//check if we can step up (small obstacle in front, empty space above it)
bool GridLayer::CanStepUp(const Rect& r, int dx, int stepHeight) const {
    int testX = (dx > 0) ? (r.x + r.w + dx) : (r.x + dx);
    int col = DivGridWidth(testX);
    
//check if there's a wall at feet level
    int feetRow = DivGridHeight(r.y + r.h - 1);
    bool wallAtFeet = IsSolidAt(col, feetRow);
    
    if (!wallAtFeet) return false;  //no step to climb
    
//check if space is clear above the step (up to stepHeight)
    int stepTopRow = DivGridHeight(r.y + r.h - 1 - stepHeight);
    for (int row = stepTopRow; row < feetRow; ++row) {
        if (IsSolidAt(col, row)) {
            return false;  //wall extends too high - can't step up
        }
    }
    
//also check head clearance at new position
    int headRow = DivGridHeight(r.y - stepHeight);
    return !IsSolidAt(col, headRow);
}

//check if we need to step down (walking off edge onto lower ground)
bool GridLayer::CanStepDown(const Rect& r, int dx, int stepHeight) const {
    int testX = (dx > 0) ? (r.x + r.w + dx) : (r.x + dx);
    int col = DivGridWidth(testX);
    int feetRow = DivGridHeight(r.y + r.h);
    
//check if current position is on ground but next position has no ground
    bool noGroundAhead = !IsGroundAt(col, feetRow);
    if (!noGroundAhead) return false;
    
//look for ground within step distance below
    for (int i = 1; i <= stepHeight / GRID_ELEMENT_HEIGHT; ++i) {
        if (IsGroundAt(col, feetRow + i)) {
            return true;
        }
    }
    return false;
}

void GridLayer::FilterGridMotion(const Rect& r, int* dx, int* dy) const {
//=== HORIZONTAL MOVEMENT ===
    if (*dx != 0) {
        int x1 = r.x;
        int x2 = r.x + r.w - 1;
        int testX = (*dx > 0) ? (x2 + *dx) : (x1 + *dx);
        
//bounds check
        if (testX < 0) {
            *dx = -x1;
        } else if (testX >= GetPixelWidth()) {
            *dx = GetPixelWidth() - 1 - x2;
        }
        
//check for walls at the destination column
        int col = DivGridWidth((*dx > 0) ? (x2 + *dx) : (x1 + *dx));
        
//check body area (exclude bottom row for step-up tolerance)
        int topRow = DivGridHeight(r.y);
        int feetRow = DivGridHeight(r.y + r.h - 1);
        int bodyBottomRow = DivGridHeight(r.y + r.h - STEP_HEIGHT - 1);
        if (bodyBottomRow < topRow) bodyBottomRow = topRow;
        
//count how many rows of the BODY (not feet) hit walls
//only SOLID, LOOP are walls - SLOPE and TUNNEL are passable horizontally
//TUNNEL needs to be passable so Sonic can enter the tunnel!
        int wallHits = 0;
        for (int row = topRow; row <= bodyBottomRow; ++row) {
            GridIndex tile = GetTileType(col, row);
            if (tile == GRID_SOLID || tile == GRID_LOOP) {
                wallHits++;
            }
        }
        
//check what's at foot level - if it's slope, allow movement
        GridIndex feetTile = GetTileType(col, feetRow);
        GridIndex belowFeetTile = GetTileType(col, feetRow + 1);
        bool slopeAtFeet = (feetTile == GRID_SLOPE || belowFeetTile == GRID_SLOPE);
        
//block if multiple wall tiles hit the body (full wall)
//single tile at bottom is step-up, allow it
//slopes at feet override wall detection (Sonic can walk up slopes)
        bool isFullWall = (wallHits >= 2) || (wallHits == 1 && bodyBottomRow == topRow);
        
        if (isFullWall && !slopeAtFeet) {
            if (*dx > 0) {
                *dx = MulGridWidth(col) - x2 - 1;
            } else {
                *dx = MulGridWidth(col + 1) - x1;
            }
            if (*dx < 0 && *dx > -1) *dx = 0;
            if (*dx > 0 && *dx < 1) *dx = 0;
        }
    }
    
//=== VERTICAL MOVEMENT ===
    if (*dy != 0) {
        int y1 = r.y;
        int y2 = r.y + r.h - 1;
        
        if (*dy < 0) {
//moving up (jumping)
            int testY = y1 + *dy;
            if (testY < 0) {
                *dy = -y1;
            } else {
                int row = DivGridHeight(testY);
                int startCol = DivGridWidth(r.x);
                int endCol = DivGridWidth(r.x + r.w - 1);
                
                for (int col = startCol; col <= endCol; ++col) {
                    GridIndex tile = GetTileType(col, row);
//only solid blocks ceiling (not slopes/platforms/loops/tunnels)
                    if (tile == GRID_SOLID) {
                        *dy = MulGridHeight(row + 1) - y1;
                        break;
                    }
                }
            }
        } else {
//moving down (falling/walking)
            int testY = y2 + *dy;
            if (testY >= GetPixelHeight()) {
                *dy = GetPixelHeight() - 1 - y2;
            } else {
                int row = DivGridHeight(testY);
                int startCol = DivGridWidth(r.x);
                int endCol = DivGridWidth(r.x + r.w - 1);
                
                for (int col = startCol; col <= endCol; ++col) {
                    GridIndex tile = GetTileType(col, row);
//stop on solid, platform, slope, loop (all act as ground from above)
//TUNNEL should NOT act as ground - Sonic needs to pass through it!
                    if (tile == GRID_SOLID || tile == GRID_PLATFORM || tile == GRID_SLOPE ||
                        tile == GRID_LOOP) {
                        *dy = MulGridHeight(row) - y2 - 1;
                        break;
                    }
                }
            }
        }
    }
}

CollisionInfo GridLayer::CheckCollision(const Rect& r, int /*dx*/, int /*dy*/) const {
    CollisionInfo info;
    
//check tiles at current position
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    int startRow = DivGridHeight(r.y);
    int endRow = DivGridHeight(r.y + r.h - 1);
    
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            GridIndex tile = GetTileType(col, row);
            switch (tile) {
                case GRID_SLOPE: info.onSlope = true; break;
                case GRID_LOOP: info.inLoop = true; break;
                case GRID_TUNNEL: info.inTunnel = true; break;
                case GRID_DEATH: info.inDeath = true; break;
            }
        }
    }
    
//check ground below feet
    int feetRow = DivGridHeight(r.y + r.h);
    for (int col = startCol; col <= endCol; ++col) {
        if (IsGroundAt(col, feetRow)) {
            info.hitGround = true;
            info.groundY = MulGridHeight(feetRow);
            break;
        }
    }
    
    return info;
}

bool GridLayer::IsOnSolidGround(const Rect& r) const {
    int row = DivGridHeight(r.y + r.h);
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    
    for (int col = startCol; col <= endCol; ++col) {
        if (IsGroundAt(col, row)) {
            return true;
        }
    }
    return false;
}

bool GridLayer::IsOnSlope(const Rect& r) const {
    int row = DivGridHeight(r.y + r.h);
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    
    for (int col = startCol; col <= endCol; ++col) {
        if (GetTileType(col, row) == GRID_SLOPE) {
            return true;
        }
    }
    return false;
}

bool GridLayer::IsInLoop(const Rect& r) const {
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    int startRow = DivGridHeight(r.y);
    int endRow = DivGridHeight(r.y + r.h - 1);
    
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            if (GetTileType(col, row) == GRID_LOOP) {
                return true;
            }
        }
    }
    return false;
}

bool GridLayer::IsInTunnel(const Rect& r) const {
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    int startRow = DivGridHeight(r.y);
    int endRow = DivGridHeight(r.y + r.h - 1);
    
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            if (GetTileType(col, row) == GRID_TUNNEL) {
                return true;
            }
        }
    }
    return false;
}

bool GridLayer::IsInDeath(const Rect& r) const {
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    int startRow = DivGridHeight(r.y);
    int endRow = DivGridHeight(r.y + r.h - 1);
    
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startCol; col <= endCol; ++col) {
            if (GetTileType(col, row) == GRID_DEATH) {
                return true;
            }
        }
    }
    return false;
}

bool GridLayer::LoadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "GridLayer: Failed to open " << path << std::endl;
        return false;
    }
    
    std::string line;
    std::vector<GridIndex> tempGrid;
    Dim rows = 0, cols = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
//remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::stringstream ss(line);
        std::string cell;
        Dim currentCols = 0;
        
        while (std::getline(ss, cell, ',')) {
//trim whitespace
            size_t start = cell.find_first_not_of(" \t\r\n");
            size_t end = cell.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) {
//empty cell - treat as empty
                tempGrid.push_back(GRID_EMPTY);
                currentCols++;
                continue;
            }
            cell = cell.substr(start, end - start + 1);
            
            if (!cell.empty()) {
                int idx = std::stoi(cell);
//tiled format: -1 = empty, 1-based indices for tiles
//collision_tiles.png layout (7 tiles, 0-6 in tileset):
//tile 0 (CSV 1): Gray = Solid -> GRID_SOLID
//tile 1 (CSV 2): Green = Platform -> GRID_PLATFORM
//tile 2 (CSV 3): Yellow = Slope -> GRID_SLOPE
//tile 3 (CSV 4): Blue = Loop -> GRID_LOOP
//tile 4 (CSV 5): Cyan = Tunnel -> GRID_TUNNEL
//tile 5 (CSV 6): Dark Red = Death -> GRID_DEATH
                GridIndex gridVal = GRID_EMPTY;
                if (idx <= 0) {
                    gridVal = GRID_EMPTY;  //-1 or 0 = empty
                } else if (idx == 1) {
                    gridVal = GRID_SOLID;  //tile 0 (gray)
                } else if (idx == 2) {
                    gridVal = GRID_PLATFORM;  //tile 1 (green)
                } else if (idx == 3) {
                    gridVal = GRID_SLOPE;  //tile 2 (yellow)
                } else if (idx == 4) {
                    gridVal = GRID_LOOP;  //tile 3 (blue)
                } else if (idx == 5) {
                    gridVal = GRID_TUNNEL;  //tile 4 (cyan)
                } else if (idx == 6) {
                    gridVal = GRID_DEATH;  //tile 5 (dark red)
                }
                tempGrid.push_back(gridVal);
                currentCols++;
            }
        }
        
        if (currentCols > 0) {
            if (cols == 0) cols = currentCols;
            rows++;
        }
    }
    
    if (rows == 0 || cols == 0) {
        std::cerr << "GridLayer: Empty or invalid CSV" << std::endl;
        return false;
    }
    
    totalRows = rows;
    totalCols = cols;
    grid = std::move(tempGrid);
    
    std::cout << "GridLayer: Loaded " << totalCols << "x" << totalRows 
              << " grid (" << GetPixelWidth() << "x" << GetPixelHeight() << " pixels)" << std::endl;
    
    return true;
}

bool GridLayer::SaveBinary(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(reinterpret_cast<const char*>(&totalRows), sizeof(totalRows));
    file.write(reinterpret_cast<const char*>(&totalCols), sizeof(totalCols));
    file.write(reinterpret_cast<const char*>(grid.data()), grid.size());
    return true;
}

bool GridLayer::LoadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.read(reinterpret_cast<char*>(&totalRows), sizeof(totalRows));
    file.read(reinterpret_cast<char*>(&totalCols), sizeof(totalCols));
    grid.resize(totalRows * totalCols);
    file.read(reinterpret_cast<char*>(grid.data()), grid.size());
    return true;
}

}  //namespace engine
