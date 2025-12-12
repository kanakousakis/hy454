#pragma once

#include "Types.hpp"
#include "TileConstants.hpp"
#include <vector>
#include <string>

namespace engine {

class GridLayer {
private:
    std::vector<GridIndex> grid;
    Dim totalRows = 0;
    Dim totalCols = 0;
    
    void FilterGridMotionLeft(const Rect& r, int* dx) const;
    void FilterGridMotionRight(const Rect& r, int* dx) const;
    void FilterGridMotionUp(const Rect& r, int* dy) const;
    void FilterGridMotionDown(const Rect& r, int* dy) const;

public:
    GridLayer() = default;
    GridLayer(Dim rows, Dim cols);
    
    bool Create(Dim rows, Dim cols);
    
    // Grid access
    void SetTile(Dim col, Dim row, GridIndex idx);
    GridIndex GetTile(Dim col, Dim row) const;
    
    // Dimensions
    Dim GetRows() const { return totalRows; }
    Dim GetCols() const { return totalCols; }
    int GetPixelWidth() const { return totalCols * GRID_ELEMENT_WIDTH; }
    int GetPixelHeight() const { return totalRows * GRID_ELEMENT_HEIGHT; }
    
    // Collision filtering - the core functionality
    void FilterGridMotion(const Rect& r, int* dx, int* dy) const;
    
    // Ground detection
    bool IsOnSolidGround(const Rect& r) const;
    bool IsOnPlatform(const Rect& r) const;
    
    // Serialization
    bool LoadCSV(const std::string& path);
    bool SaveBinary(const std::string& path) const;
    bool LoadBinary(const std::string& path);
};

} // namespace engine
