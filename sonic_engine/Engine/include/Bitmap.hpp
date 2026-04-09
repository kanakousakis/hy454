#pragma once

#include "Types.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <memory>

namespace engine {

//forward declarations
class Bitmap;
using BitmapPtr = std::shared_ptr<Bitmap>;

//bitmap class wrapping SFML texture and sprite
class Bitmap {
private:
    sf::Texture texture;
    sf::Image image;
    bool hasImage = false;
    Dim width = 0, height = 0;

public:
    Bitmap() = default;
    Bitmap(Dim w, Dim h);
    Bitmap(const std::string& path);
    
    bool LoadFromFile(const std::string& path);
    bool Create(Dim w, Dim h);
    
    Dim GetWidth() const { return width; }
    Dim GetHeight() const { return height; }
    
    const sf::Texture& GetTexture() const { return texture; }
    sf::Image& GetImage();
    
    void SetPixel(int x, int y, Color c);
    Color GetPixel(int x, int y) const;
    
    void Clear(Color c);
    void UpdateTexture();  //call after modifying pixels
    
//blit operations
    void Blit(const Rect& src, Bitmap& dest, const Point& destPos) const;
    void BlitMasked(const Rect& src, Bitmap& dest, const Point& destPos, Color colorKey) const;
};

//graphics system - singleton managing the render window
class Graphics {
private:
    sf::RenderWindow window;
    sf::RenderTexture backBuffer;
    BitmapPtr screenBitmap;
    Color backgroundColor = MakeColor(0, 0, 0);
    Color colorKey = MakeColor(255, 0, 255);  //magenta default
    bool initialized = false;
    sf::Font defaultFont;
    bool fontLoaded = false;
    
//virtual resolution for scaling
    Dim virtualWidth = 0;
    Dim virtualHeight = 0;
    
//fullscreen tracking
    bool isFullscreen = false;
    Dim windowedWidth = 0;
    Dim windowedHeight = 0;
    std::string windowTitle = "Sonic Engine";
    
//native rendering mode (for HUD)
    bool nativeRenderMode = false;
    int nativeScale = 1;
    float nativeOffsetX = 0;
    float nativeOffsetY = 0;
    
    static Graphics* instance;
    Graphics() = default;

public:
    static Graphics& Instance();
    
    bool Init(Dim width, Dim height, const std::string& title = "Sonic Engine");
    void SetVirtualResolution(Dim vWidth, Dim vHeight);  //set virtual res for scaling
    void Shutdown();
    
    sf::RenderWindow& GetWindow() { return window; }
    sf::RenderTexture& GetBackBuffer() { return backBuffer; }
    
    Dim GetWidth() const;
    Dim GetHeight() const;
    Dim GetVirtualWidth() const { return virtualWidth > 0 ? virtualWidth : GetWidth(); }
    Dim GetVirtualHeight() const { return virtualHeight > 0 ? virtualHeight : GetHeight(); }
    
    void SetBackgroundColor(Color c) { backgroundColor = c; }
    Color GetBackgroundColor() const { return backgroundColor; }
    
    void SetColorKey(Color c) { colorKey = c; }
    Color GetColorKey() const { return colorKey; }
    
    void Clear();
    void Clear(Color c);
    void Present();  //scale backbuffer to window (doesn't display yet)
    void FinalDisplay();  //actually show the frame
    
//native resolution rendering (for HUD) - draws directly to window at native res
    void DrawNativeText(const std::string& text, int x, int y, Color c);
    void DrawNativeRect(const Rect& r, Color c, bool filled = false);
    void DrawNativeSprite(const sf::Sprite& sprite, int x, int y);
    int GetNativeScale() const { return nativeScale; }
    Dim GetNativeWidth() const { return virtualWidth * nativeScale; }
    Dim GetNativeHeight() const { return virtualHeight * nativeScale; }
    
    bool IsOpen() const { return window.isOpen(); }
    bool IsFullscreen() const { return isFullscreen; }
    void ToggleFullscreen();  //toggle fullscreen mode
    
//drawing helpers
    void DrawRect(const Rect& r, Color c, bool filled = false);
    void DrawSprite(const sf::Sprite& sprite);
    void DrawTexture(const sf::Texture& tex, const Rect& src, const Point& dest);
    void DrawTextureScaled(const sf::Texture& tex, const Rect& src, const Rect& dest);
    void DrawText(const std::string& text, int x, int y, Color c);
};

//bitmapLoader - caches loaded bitmaps
class BitmapLoader {
private:
    std::map<std::string, BitmapPtr> cache;
    static BitmapLoader* instance;
    BitmapLoader() = default;

public:
    static BitmapLoader& Instance();
    
    BitmapPtr Load(const std::string& path);
    BitmapPtr Create(Dim w, Dim h);
    void Clear();
    
    bool HasBitmap(const std::string& path) const;
    BitmapPtr GetBitmap(const std::string& path) const;
};

//convenience functions
inline Graphics& GetGraphics() { return Graphics::Instance(); }
inline BitmapLoader& GetBitmapLoader() { return BitmapLoader::Instance(); }

inline sf::Color ToSFMLColor(Color c) {
    return sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c));
}

inline Color FromSFMLColor(const sf::Color& c) {
    return MakeColor(c.r, c.g, c.b, c.a);
}

}  //namespace engine
