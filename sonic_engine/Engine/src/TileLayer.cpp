#include "TileLayer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace engine {

TileLayer::TileLayer(Dim rows, Dim cols, BitmapPtr tileset) {
    Create(rows, cols, tileset);
}

bool TileLayer::Create(Dim rows, Dim cols, BitmapPtr tileset) {
    totalRows = rows;
    totalCols = cols;
    tileSet = tileset;
    
    map.resize(rows * cols, 0);
    
//create display buffer with extra tiles for scrolling
//use VIRTUAL resolution, not window size
    unsigned virtualW = GetGraphics().GetVirtualWidth();
    unsigned virtualH = GetGraphics().GetVirtualHeight();
    
    std::cout << "TileLayer::Create - Virtual resolution: " << virtualW << "×" << virtualH << std::endl;
    
    unsigned bufferW = virtualW + 2 * TILE_WIDTH;
    unsigned bufferH = virtualH + 2 * TILE_HEIGHT;
    displayBuffer.create(bufferW, bufferH);
    
    viewWin = {0, 0, 
               static_cast<int>(virtualW), 
               static_cast<int>(virtualH)};
    
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
//horizontal bounds
    int newX = viewWin.x + *dx;
    if (newX < 0) {
        *dx = -viewWin.x;
    } else if (newX + viewWin.w > GetPixelWidth()) {
        *dx = GetPixelWidth() - viewWin.w - viewWin.x;
    }
    
//vertical bounds
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
    
//calculate visible tile range (in game tile coordinates)
    int startCol = DivTileWidth(viewWin.x);
    int startRow = DivTileHeight(viewWin.y);
    int endCol = DivTileWidth(viewWin.x + viewWin.w - 1) + 1;
    int endRow = DivTileHeight(viewWin.y + viewWin.h - 1) + 1;
    
//clamp to map bounds
    startCol = std::max(0, startCol);
    startRow = std::max(0, startRow);
    endCol = std::min(static_cast<int>(totalCols), endCol);
    endRow = std::min(static_cast<int>(totalRows), endRow);
    
//offset within first tile (for smooth scrolling)
    int offsetX = ModTileWidth(viewWin.x);
    int offsetY = ModTileHeight(viewWin.y);
    
//draw visible tiles
    for (int row = startRow; row < endRow; ++row) {
        for (int col = startCol; col < endCol; ++col) {
            TileIndex tile = GetTile(col, row);
            
//skip empty tiles (0 means empty/sky in our internal format)
            if (tile == 0) continue;
            
//our LoadCSV already converts to 0-based, so tile index is ready
//calculate which tile in the tileset grid
            int tileCol = tile % TILESET_WIDTH;
            int tileRow = tile / TILESET_WIDTH;
            
//calculate source rect WITH margin and spacing
//formula: margin + col * (tileWidth + spacing)
            int srcX = GetTileSourceX(tileCol);
            int srcY = GetTileSourceY(tileRow);
            
//bounds check
            if (srcX + TILESET_TILE_WIDTH > static_cast<int>(tileSet->GetWidth()) ||
                srcY + TILESET_TILE_HEIGHT > static_cast<int>(tileSet->GetHeight())) {
                continue;  //skip invalid tiles
            }
            
            Rect srcRect(srcX, srcY, TILESET_TILE_WIDTH, TILESET_TILE_HEIGHT);
            
//calculate destination position (using game tile size)
            int destX = displayArea.x + (col - startCol) * TILE_WIDTH - offsetX;
            int destY = displayArea.y + (row - startRow) * TILE_HEIGHT - offsetY;
            
//draw 1:1 (source and dest are both 256×256)
            gfx.DrawTextureScaled(tileTexture, srcRect, 
                                  Rect(destX, destY, TILE_WIDTH, TILE_HEIGHT));
        }
    }
}

bool TileLayer::LoadCSV(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "TileLayer: Failed to open " << path << std::endl;
        return false;
    }
    
    std::string line;
    std::vector<TileIndex> tempMap;
    Dim rows = 0, cols = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
//remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::stringstream ss(line);
        std::string cell;
        Dim currentCols = 0;
        
        while (std::getline(ss, cell, ',')) {
//handle whitespace
            size_t start = cell.find_first_not_of(" \t\r\n");
            size_t end = cell.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) {
//empty cell - treat as empty tile
                tempMap.push_back(0);
                currentCols++;
                continue;
            }
            cell = cell.substr(start, end - start + 1);
            
            if (!cell.empty()) {
                int idx = std::stoi(cell);
//tiled format:
//-1 = empty (no tile)
//1+ = 1-based tile index (subtract 1 to get 0-based)
//we store 0 as empty, 1+ as actual tile indices
                if (idx <= 0) {
                    tempMap.push_back(0);  //empty
                } else {
                    tempMap.push_back(static_cast<TileIndex>(idx - 1));  //convert to 0-based
                }
                currentCols++;
            }
        }
        
        if (currentCols > 0) {
            if (cols == 0) cols = currentCols;
            rows++;
        }
    }
    
    if (rows == 0 || cols == 0) {
        std::cerr << "TileLayer: Empty or invalid CSV" << std::endl;
        return false;
    }
    
    totalRows = rows;
    totalCols = cols;
    map = std::move(tempMap);
    displayChanged = true;
    
    std::cout << "TileLayer: Loaded " << totalCols << "x" << totalRows 
              << " map (" << GetPixelWidth() << "x" << GetPixelHeight() << " pixels)" << std::endl;
    
//recreate display buffer using VIRTUAL resolution
    unsigned bufferW = GetGraphics().GetVirtualWidth() + 2 * TILE_WIDTH;
    unsigned bufferH = GetGraphics().GetVirtualHeight() + 2 * TILE_HEIGHT;
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

}  //namespace engine
