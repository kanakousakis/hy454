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
using engine::GRID_PLATFORM;
using engine::GRID_SLOPE;
using engine::GRID_LOOP;
using engine::GRID_TUNNEL;
using engine::GRID_DEATH;
using engine::GRID_CELLS_PER_TILE_X;
using engine::GRID_CELLS_PER_TILE_Y;

//loads tile maps and grid layers from CSV files exported from Tiled
class TileMapLoader {
public:
//load a tile layer from CSV file
//returns true on success
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
//trim whitespace
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
        
//set tiles in the layer
        for (engine::Dim row = 0; row < outRows; ++row) {
            for (engine::Dim col = 0; col < outCols && col < data[row].size(); ++col) {
                int index = data[row][col];
//tiled uses 0 for empty, 1+ for tiles (subtract 1 for our 0-based index)
//but we'll keep as-is and let the caller decide
                layer->SetTile(col, row, static_cast<TileIndex>(index));
            }
        }
        
        std::cout << "Loaded tile map: " << filename << " (" << outCols << "x" << outRows << ")" << std::endl;
        return true;
    }
    
//load a grid layer from CSV file with new collision types
//now uses: 0=empty, 1=solid, 2=platform, 3=slope, 4=loop, 5=tunnel, 6=death
    static bool LoadGridLayerCSV(const std::string& filename,
                                  engine::GridLayer* grid,
                                  engine::Dim& outRows,
                                  engine::Dim& outCols) {
//use GridLayer's built-in CSV loading
        if (!grid->LoadCSV(filename)) {
            return false;
        }
        outRows = grid->GetRows();
        outCols = grid->GetCols();
        return true;
    }
    
//compute grid from tile layer using empty tile indices
//emptyTileIndices: set of tile indices that should be treated as empty (passable)
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
                    gridType = GRID_PLATFORM;
                } else {
                    gridType = GRID_SOLID;
                }
                
//fill entire tile's worth of grid cells
                int gridStartRow = tileRow * GRID_CELLS_PER_TILE_Y;
                int gridStartCol = tileCol * GRID_CELLS_PER_TILE_X;
                
                for (int gr = 0; gr < static_cast<int>(GRID_CELLS_PER_TILE_Y); ++gr) {
                    for (int gc = 0; gc < static_cast<int>(GRID_CELLS_PER_TILE_X); ++gc) {
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
    
//parse width and height from first line of custom format
//format: "width height\ndata..."
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
                    case 2: gridType = GRID_PLATFORM; break;
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

}  //namespace app

#endif  //TILEMAP_LOADER_HPP
