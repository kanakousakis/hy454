#pragma once

#include "Types.hpp"
#include "TileConstants.hpp"
#include "Bitmap.hpp"
#include <vector>
#include <string>

namespace engine {

class TileLayer {
private:
    std::vector<TileIndex> map;
    Dim totalRows = 0;
    Dim totalCols = 0;
    BitmapPtr tileSet;
    Rect viewWin;
    
    // Display cache
    sf::RenderTexture displayBuffer;
    bool displayChanged = true;
    int cachedViewX = -1;
    int cachedViewY = -1;

public:
    TileLayer() = default;
    TileLayer(Dim rows, Dim cols, BitmapPtr tileset);
    
    bool Create(Dim rows, Dim cols, BitmapPtr tileset);
    
    // Tile access
    void SetTile(Dim col, Dim row, TileIndex idx);
    TileIndex GetTile(Dim col, Dim row) const;
    
    // Dimensions
    Dim GetRows() const { return totalRows; }
    Dim GetCols() const { return totalCols; }
    int GetPixelWidth() const { return MulTileWidth(totalCols); }
    int GetPixelHeight() const { return MulTileHeight(totalRows); }
    
    // View window
    const Rect& GetViewWindow() const { return viewWin; }
    void SetViewWindow(const Rect& r);
    
    // Scrolling
    void Scroll(int dx, int dy);
    bool CanScrollHoriz(int dx) const;
    bool CanScrollVert(int dy) const;
    void FilterScroll(int* dx, int* dy) const;
    
    // Pick tile from pixel coordinates
    Point PickTile(int pixelX, int pixelY) const;
    
    // Rendering
    void Display(const Rect& displayArea);
    
    // Serialization
    bool LoadCSV(const std::string& path);
    bool Save(const std::string& path) const;
    
    // Tileset
    void SetTileSet(BitmapPtr ts) { tileSet = ts; displayChanged = true; }
    BitmapPtr GetTileSet() const { return tileSet; }
};

} // namespace engine
