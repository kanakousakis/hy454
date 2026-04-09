#pragma once

#include "Types.hpp"
#include "TileConstants.hpp"
#include <vector>
#include <string>

namespace engine {

//collision result info
struct CollisionInfo {
    bool hitWall = false;
    bool hitGround = false;
    bool hitCeiling = false;
    bool onSlope = false;
    bool inLoop = false;
    bool inTunnel = false;
    bool inDeath = false;
    int groundY = 0;  //y position of ground below
    int stepHeight = 0;  //height of step encountered
};

class GridLayer {
private:
    std::vector<GridIndex> grid;
    Dim totalRows = 0;
    Dim totalCols = 0;
    
//core collision checks
    bool IsSolidAt(int col, int row) const;
    bool IsGroundAt(int col, int row) const;  //solid or platform or slope from above
    GridIndex GetTileType(int col, int row) const;
    
//step-up/down helpers
    int FindGroundY(int pixelX, int startY, int maxDist) const;
    bool CanStepUp(const Rect& r, int dx, int stepHeight) const;
    bool CanStepDown(const Rect& r, int dx, int stepHeight) const;

public:
    GridLayer() = default;
    GridLayer(Dim rows, Dim cols);
    
    bool Create(Dim rows, Dim cols);
    
//grid access
    void SetTile(Dim col, Dim row, GridIndex idx);
    GridIndex GetTile(Dim col, Dim row) const;
    
//dimensions
    Dim GetRows() const { return totalRows; }
    Dim GetCols() const { return totalCols; }
    int GetPixelWidth() const { return totalCols * GRID_ELEMENT_WIDTH; }
    int GetPixelHeight() const { return totalRows * GRID_ELEMENT_HEIGHT; }
    
//main collision filtering with step-up/down
    void FilterGridMotion(const Rect& r, int* dx, int* dy) const;
    
//advanced collision with full info
    CollisionInfo CheckCollision(const Rect& r, int dx, int dy) const;
    
//ground detection
    bool IsOnSolidGround(const Rect& r) const;
    bool IsOnSlope(const Rect& r) const;
    bool IsInLoop(const Rect& r) const;
    bool IsInTunnel(const Rect& r) const;
    bool IsInDeath(const Rect& r) const;
    
//find ground below a point
    int FindGroundBelow(int pixelX, int pixelY, int maxDist = 256) const;
    
//serialization - loads Tiled CSV format
    bool LoadCSV(const std::string& path);
    bool SaveBinary(const std::string& path) const;
    bool LoadBinary(const std::string& path);
};

}  //namespace engine
