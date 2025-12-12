#ifndef TILEMAP_LOADER_HPP
#define TILEMAP_LOADER_HPP

#include "Engine.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>

namespace app {

using engine::Dim;
using engine::TileIndex;
using engine::GridIndex;
using engine::GRID_EMPTY;
using engine::GRID_SOLID;
using engine::GRID_TOP_SOLID;
using engine::GRID_BLOCK_ROWS;
using engine::GRID_BLOCK_COLUMNS;

// Loads tile maps and grid layers from CSV files exported from Tiled
class TileMapLoader {
public:
    // Load a tile layer from CSV file
    // Returns true on success
    static bool LoadTileLayerCSV(const std::string& filename, 
                                  engine::TileLayer* layer,
                                  engine::Dim& outRows, 
                                  engine::Dim& outCols) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open tile map: " << filename << std::endl;
            return false;
        }
        
        std::vector<std::vector<int>> data;
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<int> row;
            std::stringstream ss(line);
            std::string cell;
            
            while (std::getline(ss, cell, ',')) {
                // Trim whitespace
                size_t start = cell.find_first_not_of(" \t\r\n");
                size_t end = cell.find_last_not_of(" \t\r\n");
                if (start != std::string::npos) {
                    cell = cell.substr(start, end - start + 1);
                }
                
                if (!cell.empty()) {
                    try {
                        int value = std::stoi(cell);
                        row.push_back(value);
                    } catch (...) {
                        row.push_back(0);
                    }
                }
            }
            
            if (!row.empty()) {
                data.push_back(row);
            }
        }
        
        if (data.empty()) {
            std::cerr << "Empty tile map: " << filename << std::endl;
            return false;
        }
        
        outRows = static_cast<engine::Dim>(data.size());
        outCols = static_cast<engine::Dim>(data[0].size());
        
        // Set tiles in the layer
        for (engine::Dim row = 0; row < outRows; ++row) {
            for (engine::Dim col = 0; col < outCols && col < data[row].size(); ++col) {
                int index = data[row][col];
                // Tiled uses 0 for empty, 1+ for tiles (subtract 1 for our 0-based index)
                // But we'll keep as-is and let the caller decide
                layer->SetTile(col, row, static_cast<TileIndex>(index));
            }
        }
        
        std::cout << "Loaded tile map: " << filename << " (" << outCols << "x" << outRows << ")" << std::endl;
        return true;
    }
    
    // Load a grid layer from CSV file
    // gridType: 0=empty, 1=solid, 2=top-solid (platform)
    static bool LoadGridLayerCSV(const std::string& filename,
                                  engine::GridLayer* grid,
                                  engine::Dim& outRows,
                                  engine::Dim& outCols) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open grid map: " << filename << std::endl;
            return false;
        }
        
        std::vector<std::vector<int>> data;
        std::string line;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<int> row;
            std::stringstream ss(line);
            std::string cell;
            
            while (std::getline(ss, cell, ',')) {
                size_t start = cell.find_first_not_of(" \t\r\n");
                size_t end = cell.find_last_not_of(" \t\r\n");
                if (start != std::string::npos) {
                    cell = cell.substr(start, end - start + 1);
                }
                
                if (!cell.empty()) {
                    try {
                        int value = std::stoi(cell);
                        row.push_back(value);
                    } catch (...) {
                        row.push_back(0);
                    }
                }
            }
            
            if (!row.empty()) {
                data.push_back(row);
            }
        }
        
        if (data.empty()) {
            std::cerr << "Empty grid map: " << filename << std::endl;
            return false;
        }
        
        outRows = static_cast<engine::Dim>(data.size());
        outCols = static_cast<engine::Dim>(data[0].size());
        
        // Set grid tiles
        for (engine::Dim row = 0; row < outRows; ++row) {
            for (engine::Dim col = 0; col < outCols && col < data[row].size(); ++col) {
                int value = data[row][col];
                engine::GridIndex gridType;
                
                // Map Tiled indices to grid types
                // Convention: 0 or -1 = empty, 1 = solid, 2 = top-solid (platform), 3 = empty explicitly
                switch (value) {
                    case 0:
                    case -1:
                    case 3:
                        gridType = GRID_EMPTY;
                        break;
                    case 1:
                        gridType = GRID_SOLID;
                        break;
                    case 2:
                        gridType = GRID_TOP_SOLID;
                        break;
                    default:
                        // Tiled uses 1-based indices, so subtract 1
                        // 1 = first tile = solid, 2 = second tile = empty, 3 = third tile = top-solid
                        if (value == 1) gridType = GRID_SOLID;
                        else if (value == 3) gridType = GRID_TOP_SOLID;
                        else gridType = GRID_EMPTY;
                        break;
                }
                
                grid->SetTile(col, row, gridType);
            }
        }
        
        std::cout << "Loaded grid map: " << filename << " (" << outCols << "x" << outRows << ")" << std::endl;
        return true;
    }
    
    // Compute grid from tile layer using empty tile indices
    // emptyTileIndices: set of tile indices that should be treated as empty (passable)
    static void ComputeGridFromTiles(engine::TileLayer* tileLayer,
                                      engine::GridLayer* gridLayer,
                                      const std::set<int>& emptyTileIndices,
                                      const std::set<int>& platformTileIndices = {}) {
        
        engine::Dim tileRows = tileLayer->GetRows();
        engine::Dim tileCols = tileLayer->GetCols();
        
        for (engine::Dim tileRow = 0; tileRow < tileRows; ++tileRow) {
            for (engine::Dim tileCol = 0; tileCol < tileCols; ++tileCol) {
                TileIndex tileIndex = tileLayer->GetTile(tileCol, tileRow);
                
                engine::GridIndex gridType;
                if (emptyTileIndices.count(tileIndex) > 0) {
                    gridType = GRID_EMPTY;
                } else if (platformTileIndices.count(tileIndex) > 0) {
                    gridType = GRID_TOP_SOLID;
                } else {
                    gridType = GRID_SOLID;
                }
                
                // Fill entire tile's worth of grid cells
                int gridStartRow = tileRow * GRID_BLOCK_ROWS;
                int gridStartCol = tileCol * GRID_BLOCK_COLUMNS;
                
                for (int gr = 0; gr < GRID_BLOCK_ROWS; ++gr) {
                    for (int gc = 0; gc < GRID_BLOCK_COLUMNS; ++gc) {
                        engine::Dim row = static_cast<engine::Dim>(gridStartRow + gr);
                        engine::Dim col = static_cast<engine::Dim>(gridStartCol + gc);
                        
                        if (row < gridLayer->GetRows() && col < gridLayer->GetCols()) {
                            gridLayer->SetTile(col, row, gridType);
                        }
                    }
                }
            }
        }
    }
    
    // Parse width and height from first line of custom format
    // Format: "width height\ndata..."
    static bool LoadGridWithHeader(const std::string& filename,
                                    engine::GridLayer*& gridOut) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open: " << filename << std::endl;
            return false;
        }
        
        int width, height;
        file >> width >> height;
        
        if (width <= 0 || height <= 0) {
            std::cerr << "Invalid dimensions in: " << filename << std::endl;
            return false;
        }
        
        gridOut = new engine::GridLayer(
            static_cast<engine::Dim>(height), 
            static_cast<engine::Dim>(width)
        );
        
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                int value;
                file >> value;
                
                engine::GridIndex gridType;
                switch (value) {
                    case 0: gridType = GRID_SOLID; break;
                    case 1: gridType = GRID_EMPTY; break;
                    case 2: gridType = GRID_TOP_SOLID; break;
                    default: gridType = GRID_EMPTY; break;
                }
                
                gridOut->SetTile(
                    static_cast<engine::Dim>(col), 
                    static_cast<engine::Dim>(row), 
                    gridType
                );
            }
        }
        
        std::cout << "Loaded grid: " << filename << " (" << width << "x" << height << ")" << std::endl;
        return true;
    }
};

} // namespace app

#endif // TILEMAP_LOADER_HPP
