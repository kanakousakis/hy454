#ifndef PARALLAX_HPP
#define PARALLAX_HPP

#include "Engine.hpp"
#include <vector>

using namespace engine;

namespace app {

//============================================================
//PARALLAX LAYER - Background layer that scrolls at different speed
//============================================================
class ParallaxLayer {
public:
    BitmapPtr bitmap;
    float scrollFactorX = 0.5f;  //0 = static, 1 = same as camera
    float scrollFactorY = 0.5f;
    int offsetX = 0;      int offsetY = 0;
    bool repeatX = true;  //tile horizontally
    bool repeatY = false;  //tile vertically
    int zOrder = 0;  //drawing order (lower = further back)
    
    ParallaxLayer() = default;
    
    ParallaxLayer(BitmapPtr bmp, float factorX, float factorY = 0.5f, int z = 0)
        : bitmap(bmp), scrollFactorX(factorX), scrollFactorY(factorY), zOrder(z) {}
    
    void Draw(Graphics& gfx, int cameraX, int cameraY, int screenWidth, int screenHeight) {
        if (!bitmap) return;
        
        int bmpWidth = bitmap->GetWidth();
        int bmpHeight = bitmap->GetHeight();
        
        if (bmpWidth <= 0 || bmpHeight <= 0) return;
        
//calculate parallax offset
        int parallaxX = static_cast<int>(cameraX * scrollFactorX) + offsetX;
        int parallaxY = static_cast<int>(cameraY * scrollFactorY) + offsetY;
        
//handle horizontal repeat
        if (repeatX) {
//calculate starting X position (wrap around)
            int startX = -(parallaxX % bmpWidth);
            if (startX > 0) startX -= bmpWidth;
            
//calculate Y position
            int drawY = repeatY ? -(parallaxY % bmpHeight) : -parallaxY;
            if (repeatY && drawY > 0) drawY -= bmpHeight;
            
//draw tiles to cover screen
            for (int x = startX; x < screenWidth; x += bmpWidth) {
                if (repeatY) {
                    for (int y = drawY; y < screenHeight; y += bmpHeight) {
                        DrawBitmap(gfx, x, y);
                    }
                } else {
                    DrawBitmap(gfx, x, drawY);
                }
            }
        } else {
//single image
            int drawX = -parallaxX;
            int drawY = repeatY ? -(parallaxY % bmpHeight) : -parallaxY;
            
            if (repeatY) {
                for (int y = drawY; y < screenHeight; y += bmpHeight) {
                    DrawBitmap(gfx, drawX, y);
                }
            } else {
                DrawBitmap(gfx, drawX, drawY);
            }
        }
    }
    
private:
    void DrawBitmap(Graphics& gfx, int x, int y) {
        if (!bitmap) return;
        
//use the full bitmap
        Rect src = {0, 0, static_cast<int>(bitmap->GetWidth()), static_cast<int>(bitmap->GetHeight())};
        gfx.DrawTexture(bitmap->GetTexture(), src, {x, y});
    }
};

//============================================================
//PARALLAX MANAGER - Handles multiple background layers
//============================================================
class ParallaxManager {
    std::vector<ParallaxLayer> layers;
    Color backgroundColor = MakeColor(0, 100, 200);  //default sky blue
    
public:
    void SetBackgroundColor(Color c) {
        backgroundColor = c;
    }
    
    void AddLayer(const ParallaxLayer& layer) {
        layers.push_back(layer);
//sort by z-order (lower z = draw first = further back)
        std::sort(layers.begin(), layers.end(), 
            [](const ParallaxLayer& a, const ParallaxLayer& b) {
                return a.zOrder < b.zOrder;
            });
    }
    
    void AddLayer(BitmapPtr bitmap, float scrollFactorX, float scrollFactorY = 0.5f, int zOrder = 0) {
        ParallaxLayer layer(bitmap, scrollFactorX, scrollFactorY, zOrder);
        AddLayer(layer);
    }
    
    void Clear() {
        layers.clear();
    }
    
    void Draw(Graphics& gfx, int cameraX, int cameraY, int screenWidth, int screenHeight) {
//first fill with background color
        gfx.DrawRect({0, 0, screenWidth, screenHeight}, backgroundColor, true);
        
//then draw layers in order
        for (auto& layer : layers) {
            layer.Draw(gfx, cameraX, cameraY, screenWidth, screenHeight);
        }
    }
    
    size_t GetLayerCount() const { return layers.size(); }
};

//============================================================
//helper: Create Green Hill Zone style background
//using the provided background_foreground64.png sprite sheet
//the sheet has layers stacked vertically that we extract
//============================================================
inline void SetupGreenHillBackground(ParallaxManager& manager, BitmapPtr backgroundSheet) {
//set sky blue as base color
    manager.SetBackgroundColor(MakeColor(0, 156, 252));  //classic Sonic sky blue
    
    if (!backgroundSheet) {
        return;  //just use solid color
    }
    
//the background_foreground64.png (704x640) contains:
//- Sky/clouds at top (y=0-112)
//- Mountains (y=112-160)
//- Hills with trees (y=160-224)
//- Near hills (y=224-288)
//- Ground decoration (y=288-352)
//- Water layers (y=352+)
    
//for now, add the whole sheet as a slow-scrolling background
//it will tile horizontally which creates the parallax effect
    ParallaxLayer bgLayer;
    bgLayer.bitmap = backgroundSheet;
    bgLayer.scrollFactorX = 0.3f;  //slow horizontal scroll
    bgLayer.scrollFactorY = 0.0f;  //no vertical scroll
    bgLayer.repeatX = true;
    bgLayer.repeatY = true;  //tile vertically to fill screen
    bgLayer.zOrder = 0;
    bgLayer.offsetY = 0;  //start at top of screen
    
    manager.AddLayer(bgLayer);
}

}  //namespace app

#endif  //PARALLAX_HPP
