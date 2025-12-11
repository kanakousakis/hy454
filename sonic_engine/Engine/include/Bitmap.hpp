#pragma once

#include "Types.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <memory>

namespace engine {

// Forward declarations
class Bitmap;
using BitmapPtr = std::shared_ptr<Bitmap>;

// Bitmap class wrapping SFML texture and sprite
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
    void UpdateTexture();  // Call after modifying pixels
    
    // Blit operations
    void Blit(const Rect& src, Bitmap& dest, const Point& destPos) const;
    void BlitMasked(const Rect& src, Bitmap& dest, const Point& destPos, Color colorKey) const;
};

// Graphics system - singleton managing the render window
class Graphics {
private:
    sf::RenderWindow window;
    sf::RenderTexture backBuffer;
    BitmapPtr screenBitmap;
    Color backgroundColor = MakeColor(0, 0, 0);
    Color colorKey = MakeColor(255, 0, 255);  // Magenta default
    bool initialized = false;
    sf::Font defaultFont;
    bool fontLoaded = false;
    
    static Graphics* instance;
    Graphics() = default;

public:
    static Graphics& Instance();
    
    bool Init(Dim width, Dim height, const std::string& title = "Sonic Engine");
    void Shutdown();
    
    sf::RenderWindow& GetWindow() { return window; }
    sf::RenderTexture& GetBackBuffer() { return backBuffer; }
    
    Dim GetWidth() const;
    Dim GetHeight() const;
    
    void SetBackgroundColor(Color c) { backgroundColor = c; }
    Color GetBackgroundColor() const { return backgroundColor; }
    
    void SetColorKey(Color c) { colorKey = c; }
    Color GetColorKey() const { return colorKey; }
    
    void Clear();
    void Clear(Color c);
    void Present();  // Swap buffers
    
    bool IsOpen() const { return window.isOpen(); }
    
    // Drawing helpers
    void DrawRect(const Rect& r, Color c, bool filled = false);
    void DrawSprite(const sf::Sprite& sprite);
    void DrawTexture(const sf::Texture& tex, const Rect& src, const Point& dest);
    void DrawTextureScaled(const sf::Texture& tex, const Rect& src, const Rect& dest);
    void DrawText(const std::string& text, int x, int y, Color c);
};

// BitmapLoader - caches loaded bitmaps
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

// Convenience functions
inline Graphics& GetGraphics() { return Graphics::Instance(); }
inline BitmapLoader& GetBitmapLoader() { return BitmapLoader::Instance(); }

// SFML color conversion helpers
inline sf::Color ToSFMLColor(Color c) {
    return sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c));
}

inline Color FromSFMLColor(const sf::Color& c) {
    return MakeColor(c.r, c.g, c.b, c.a);
}

} // namespace engine
