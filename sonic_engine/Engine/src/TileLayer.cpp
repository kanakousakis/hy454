#include "TileLayer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace engine {

TileLayer::TileLayer(Dim rows, Dim cols, BitmapPtr tileset) {
    Create(rows, cols, tileset);
}

bool TileLayer::Create(Dim rows, Dim cols, BitmapPtr tileset) {
    totalRows = rows;
    totalCols = cols;
    tileSet = tileset;
    
    map.resize(rows * cols, 0);
    
    // Create display buffer with extra tiles for scrolling
    unsigned bufferW = GetGraphics().GetWidth() + 2 * TILE_WIDTH;
    unsigned bufferH = GetGraphics().GetHeight() + 2 * TILE_HEIGHT;
    displayBuffer.create(bufferW, bufferH);
    
    viewWin = {0, 0, 
               static_cast<int>(GetGraphics().GetWidth()), 
               static_cast<int>(GetGraphics().GetHeight())};
    
    displayChanged = true;
    return true;
}

void TileLayer::SetTile(Dim col, Dim row, TileIndex idx) {
    if (col < totalCols && row < totalRows) {
        map[row * totalCols + col] = idx;
        displayChanged = true;
    }
}

TileIndex TileLayer::GetTile(Dim col, Dim row) const {
    if (col < totalCols && row < totalRows) {
        return map[row * totalCols + col];
    }
    return 0;
}

void TileLayer::SetViewWindow(const Rect& r) {
    if (viewWin.x != r.x || viewWin.y != r.y || 
        viewWin.w != r.w || viewWin.h != r.h) {
        viewWin = r;
        displayChanged = true;
    }
}

void TileLayer::Scroll(int dx, int dy) {
    FilterScroll(&dx, &dy);
    if (dx != 0 || dy != 0) {
        viewWin.x += dx;
        viewWin.y += dy;
        displayChanged = true;
    }
}

bool TileLayer::CanScrollHoriz(int dx) const {
    int newX = viewWin.x + dx;
    return newX >= 0 && (newX + viewWin.w) <= GetPixelWidth();
}

bool TileLayer::CanScrollVert(int dy) const {
    int newY = viewWin.y + dy;
    return newY >= 0 && (newY + viewWin.h) <= GetPixelHeight();
}

void TileLayer::FilterScroll(int* dx, int* dy) const {
    // Horizontal bounds
    int newX = viewWin.x + *dx;
    if (newX < 0) {
        *dx = -viewWin.x;
    } else if (newX + viewWin.w > GetPixelWidth()) {
        *dx = GetPixelWidth() - viewWin.w - viewWin.x;
    }
    
    // Vertical bounds
    int newY = viewWin.y + *dy;
    if (newY < 0) {
        *dy = -viewWin.y;
    } else if (newY + viewWin.h > GetPixelHeight()) {
        *dy = GetPixelHeight() - viewWin.h - viewWin.y;
    }
}

Point TileLayer::PickTile(int pixelX, int pixelY) const {
    return Point(
        DivTileWidth(pixelX + viewWin.x),
        DivTileHeight(pixelY + viewWin.y)
    );
}

void TileLayer::Display(const Rect& displayArea) {
    if (!tileSet) return;
    
    auto& gfx = GetGraphics();
    const sf::Texture& tileTexture = tileSet->GetTexture();
    
    // Calculate visible tile range (in game tile coordinates)
    int startCol = DivTileWidth(viewWin.x);
    int startRow = DivTileHeight(viewWin.y);
    int endCol = DivTileWidth(viewWin.x + viewWin.w - 1) + 1;
    int endRow = DivTileHeight(viewWin.y + viewWin.h - 1) + 1;
    
    // Clamp to map bounds
    startCol = std::max(0, startCol);
    startRow = std::max(0, startRow);
    endCol = std::min(static_cast<int>(totalCols), endCol);
    endRow = std::min(static_cast<int>(totalRows), endRow);
    
    // Offset within first tile (for smooth scrolling)
    int offsetX = ModTileWidth(viewWin.x);
    int offsetY = ModTileHeight(viewWin.y);
    
    // Draw visible tiles
    for (int row = startRow; row < endRow; ++row) {
        for (int col = startCol; col < endCol; ++col) {
            TileIndex tile = GetTile(col, row);
            
            // Skip empty tiles (0 means empty/sky)
            if (tile == 0) continue;
            
            // For 0-based manual tile indices, use directly
            // For Tiled CSV (1-based), subtract 1
            TileIndex tileIdx = tile;
            if (tile >= 100) {
                // Tiled CSV format uses large indices, subtract 1
                tileIdx = tile - 1;
            }
            
            // Calculate which tile in the tileset grid
            int tileCol = tileIdx % TILESET_WIDTH;
            int tileRow = tileIdx / TILESET_WIDTH;
            
            // Calculate source rect using ACTUAL tileset pixel dimensions
            int srcX = tileCol * TILESET_TILE_WIDTH;
            int srcY = tileRow * TILESET_TILE_HEIGHT;
            
            // Bounds check
            if (srcX + TILESET_TILE_WIDTH > static_cast<int>(tileSet->GetWidth()) ||
                srcY + TILESET_TILE_HEIGHT > static_cast<int>(tileSet->GetHeight())) {
                continue;  // Skip invalid tiles
            }
            
            Rect srcRect(srcX, srcY, TILESET_TILE_WIDTH, TILESET_TILE_HEIGHT);
            
            // Calculate destination position (using game tile size)
            int destX = displayArea.x + (col - startCol) * TILE_WIDTH - offsetX;
            int destY = displayArea.y + (row - startRow) * TILE_HEIGHT - offsetY;
            
            // Draw with scaling (source 182x182 -> destination 128x128)
            gfx.DrawTextureScaled(tileTexture, srcRect, 
                                  Rect(destX, destY, TILE_WIDTH, TILE_HEIGHT));
        }
    }
}

bool TileLayer::LoadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::string line;
    std::vector<TileIndex> tempMap;
    Dim rows = 0, cols = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string cell;
        Dim currentCols = 0;
        
        while (std::getline(ss, cell, ',')) {
            // Handle whitespace
            size_t start = cell.find_first_not_of(" \t\r\n");
            size_t end = cell.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            cell = cell.substr(start, end - start + 1);
            
            if (!cell.empty()) {
                int idx = std::stoi(cell);
                // Tiled uses 0 as empty, 1-based indices for tiles
                // Convert to 0-based if needed
                if (idx > 0) idx--;
                tempMap.push_back(static_cast<TileIndex>(idx));
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
    map = std::move(tempMap);
    displayChanged = true;
    
    // Recreate display buffer
    unsigned bufferW = GetGraphics().GetWidth() + 2 * TILE_WIDTH;
    unsigned bufferH = GetGraphics().GetHeight() + 2 * TILE_HEIGHT;
    displayBuffer.create(bufferW, bufferH);
    
    return true;
}

bool TileLayer::Save(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    for (Dim row = 0; row < totalRows; ++row) {
        for (Dim col = 0; col < totalCols; ++col) {
            if (col > 0) file << ",";
            file << map[row * totalCols + col];
        }
        file << "\n";
    }
    
    return true;
}

} // namespace engine
