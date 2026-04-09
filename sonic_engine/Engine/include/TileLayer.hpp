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
    
//display cache
    sf::RenderTexture displayBuffer;
    bool displayChanged = true;
    int cachedViewX = -1;
    int cachedViewY = -1;

public:
    TileLayer() = default;
    TileLayer(Dim rows, Dim cols, BitmapPtr tileset);
    
    bool Create(Dim rows, Dim cols, BitmapPtr tileset);
    
//tile access
    void SetTile(Dim col, Dim row, TileIndex idx);
    TileIndex GetTile(Dim col, Dim row) const;
    
//dimensions
    Dim GetRows() const { return totalRows; }
    Dim GetCols() const { return totalCols; }
    int GetPixelWidth() const { return MulTileWidth(totalCols); }
    int GetPixelHeight() const { return MulTileHeight(totalRows); }
    
//view window
    const Rect& GetViewWindow() const { return viewWin; }
    void SetViewWindow(const Rect& r);
    
//scrolling
    void Scroll(int dx, int dy);
    bool CanScrollHoriz(int dx) const;
    bool CanScrollVert(int dy) const;
    void FilterScroll(int* dx, int* dy) const;
    
//pick tile from pixel coordinates
    Point PickTile(int pixelX, int pixelY) const;
    
//rendering
    void Display(const Rect& displayArea);
    
//serialization
    bool LoadCSV(const std::string& path);
    bool Save(const std::string& path) const;
    
//tileset
    void SetTileSet(BitmapPtr ts) { tileSet = ts; displayChanged = true; }
    BitmapPtr GetTileSet() const { return tileSet; }
};

}  //namespace engine
