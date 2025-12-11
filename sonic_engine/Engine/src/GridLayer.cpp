#include "GridLayer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>

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
    // Out of bounds handling:
    // - Below map (row >= totalRows): return EMPTY so Sonic can fall into void
    // - Left/right of map: return SOLID to block
    if (row >= totalRows) {
        return GRID_EMPTY;  // Allow falling below map
    }
    return GRID_SOLID;  // Block at sides
}

void GridLayer::FilterGridMotion(const Rect& r, int* dx, int* dy) const {
    if (*dx < 0)
        FilterGridMotionLeft(r, dx);
    else if (*dx > 0)
        FilterGridMotionRight(r, dx);
    
    if (*dy < 0)
        FilterGridMotionUp(r, dy);
    else if (*dy > 0)
        FilterGridMotionDown(r, dy);
}

void GridLayer::FilterGridMotionLeft(const Rect& r, int* dx) const {
    int x1_next = r.x + *dx;
    if (x1_next < 0) {
        *dx = -r.x;
        return;
    }
    
    int newCol = DivGridWidth(x1_next);
    int currCol = DivGridWidth(r.x);
    
    if (newCol < currCol) {
        int startRow = DivGridHeight(r.y);
        int endRow = DivGridHeight(r.y + r.h - 1);
        
        for (int row = startRow; row <= endRow; ++row) {
            if (GetTile(newCol, row) & GRID_RIGHT_SOLID) {
                *dx = MulGridWidth(currCol) - r.x;
                return;
            }
        }
    }
}

void GridLayer::FilterGridMotionRight(const Rect& r, int* dx) const {
    int x2 = r.x + r.w - 1;
    int x2_next = x2 + *dx;
    
    if (x2_next >= GetPixelWidth()) {
        *dx = GetPixelWidth() - 1 - x2;
        return;
    }
    
    int newCol = DivGridWidth(x2_next);
    int currCol = DivGridWidth(x2);
    
    if (newCol > currCol) {
        int startRow = DivGridHeight(r.y);
        int endRow = DivGridHeight(r.y + r.h - 1);
        
        for (int row = startRow; row <= endRow; ++row) {
            if (GetTile(newCol, row) & GRID_LEFT_SOLID) {
                *dx = MulGridWidth(newCol) - 1 - x2;
                return;
            }
        }
    }
}

void GridLayer::FilterGridMotionUp(const Rect& r, int* dy) const {
    int y1_next = r.y + *dy;
    if (y1_next < 0) {
        *dy = -r.y;
        return;
    }
    
    int newRow = DivGridHeight(y1_next);
    int currRow = DivGridHeight(r.y);
    
    if (newRow < currRow) {
        int startCol = DivGridWidth(r.x);
        int endCol = DivGridWidth(r.x + r.w - 1);
        
        for (int col = startCol; col <= endCol; ++col) {
            if (GetTile(col, newRow) & GRID_BOTTOM_SOLID) {
                *dy = MulGridHeight(currRow) - r.y;
                return;
            }
        }
    }
}

void GridLayer::FilterGridMotionDown(const Rect& r, int* dy) const {
    int y2 = r.y + r.h - 1;
    int y2_next = y2 + *dy;
    
    if (y2_next >= GetPixelHeight()) {
        *dy = GetPixelHeight() - 1 - y2;
        return;
    }
    
    int newRow = DivGridHeight(y2_next);
    int currRow = DivGridHeight(y2);
    
    if (newRow > currRow) {
        int startCol = DivGridWidth(r.x);
        int endCol = DivGridWidth(r.x + r.w - 1);
        
        for (int col = startCol; col <= endCol; ++col) {
            if (GetTile(col, newRow) & GRID_TOP_SOLID) {
                *dy = MulGridHeight(newRow) - 1 - y2;
                return;
            }
        }
    }
}

bool GridLayer::IsOnSolidGround(const Rect& r) const {
    int dy = 1;
    FilterGridMotionDown(r, &dy);
    return dy == 0;
}

bool GridLayer::IsOnPlatform(const Rect& r) const {
    int y2 = r.y + r.h;
    int row = DivGridHeight(y2);
    int startCol = DivGridWidth(r.x);
    int endCol = DivGridWidth(r.x + r.w - 1);
    
    for (int col = startCol; col <= endCol; ++col) {
        if (GetTile(col, row) & GRID_TOP_SOLID)
            return true;
    }
    return false;
}

bool GridLayer::LoadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::string line;
    std::vector<GridIndex> tempGrid;
    Dim rows = 0, cols = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string cell;
        Dim currentCols = 0;
        
        while (std::getline(ss, cell, ',')) {
            size_t start = cell.find_first_not_of(" \t\r\n");
            size_t end = cell.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            cell = cell.substr(start, end - start + 1);
            
            if (!cell.empty()) {
                int idx = std::stoi(cell);
                // Dami-Karv format: 0=solid, 1=platform, other=empty
                GridIndex gridVal = GRID_EMPTY;
                if (idx == 0) gridVal = GRID_SOLID;
                else if (idx == 1) gridVal = GRID_TOP_SOLID;  // Platform
                // else: empty (default)
                tempGrid.push_back(gridVal);
                currentCols++;
            }
        }
        
        if (currentCols > 0) {
            if (cols == 0) cols = currentCols;
            rows++;
        }
    }
    
    if (rows == 0 || cols == 0) return false;
    
    totalRows = rows;
    totalCols = cols;
    grid = std::move(tempGrid);
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

} // namespace engine
