#include "Bitmap.hpp"
#include <cassert>

namespace engine {

// Static instance pointers
Graphics* Graphics::instance = nullptr;
BitmapLoader* BitmapLoader::instance = nullptr;

// ============ Bitmap Implementation ============

Bitmap::Bitmap(Dim w, Dim h) {
    Create(w, h);
}

Bitmap::Bitmap(const std::string& path) {
    LoadFromFile(path);
}

bool Bitmap::LoadFromFile(const std::string& path) {
    if (!image.loadFromFile(path))
        return false;
    
    if (!texture.loadFromImage(image))
        return false;
    
    auto size = image.getSize();
    width = static_cast<Dim>(size.x);
    height = static_cast<Dim>(size.y);
    hasImage = true;
    return true;
}

bool Bitmap::Create(Dim w, Dim h) {
    image.create(w, h, sf::Color::Transparent);
    if (!texture.loadFromImage(image))
        return false;
    
    width = w;
    height = h;
    hasImage = true;
    return true;
}

sf::Image& Bitmap::GetImage() {
    if (!hasImage) {
        image = texture.copyToImage();
        hasImage = true;
    }
    return image;
}

void Bitmap::SetPixel(int x, int y, Color c) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        GetImage().setPixel(static_cast<unsigned>(x), static_cast<unsigned>(y), 
                           ToSFMLColor(c));
    }
}

Color Bitmap::GetPixel(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height && hasImage) {
        auto c = image.getPixel(static_cast<unsigned>(x), static_cast<unsigned>(y));
        return FromSFMLColor(c);
    }
    return 0;
}

void Bitmap::Clear(Color c) {
    sf::Color col = ToSFMLColor(c);
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            GetImage().setPixel(x, y, col);
        }
    }
}

void Bitmap::UpdateTexture() {
    if (hasImage) {
        texture.loadFromImage(image);
    }
}

void Bitmap::Blit(const Rect& src, Bitmap& dest, const Point& destPos) const {
    auto& destImg = dest.GetImage();
    
    for (int y = 0; y < src.h; ++y) {
        for (int x = 0; x < src.w; ++x) {
            int srcX = src.x + x;
            int srcY = src.y + y;
            int dstX = destPos.x + x;
            int dstY = destPos.y + y;
            
            if (srcX >= 0 && srcX < width && srcY >= 0 && srcY < height &&
                dstX >= 0 && dstX < dest.width && dstY >= 0 && dstY < dest.height) {
                auto c = image.getPixel(static_cast<unsigned>(srcX), 
                                        static_cast<unsigned>(srcY));
                destImg.setPixel(static_cast<unsigned>(dstX), 
                                 static_cast<unsigned>(dstY), c);
            }
        }
    }
}

void Bitmap::BlitMasked(const Rect& src, Bitmap& dest, const Point& destPos, 
                        Color colorKey) const {
    auto& destImg = dest.GetImage();
    sf::Color keyColor = ToSFMLColor(colorKey);
    
    for (int y = 0; y < src.h; ++y) {
        for (int x = 0; x < src.w; ++x) {
            int srcX = src.x + x;
            int srcY = src.y + y;
            int dstX = destPos.x + x;
            int dstY = destPos.y + y;
            
            if (srcX >= 0 && srcX < width && srcY >= 0 && srcY < height &&
                dstX >= 0 && dstX < dest.width && dstY >= 0 && dstY < dest.height) {
                auto c = image.getPixel(static_cast<unsigned>(srcX), 
                                        static_cast<unsigned>(srcY));
                if (c.r != keyColor.r || c.g != keyColor.g || c.b != keyColor.b) {
                    destImg.setPixel(static_cast<unsigned>(dstX), 
                                     static_cast<unsigned>(dstY), c);
                }
            }
        }
    }
}

// ============ Graphics Implementation ============

Graphics& Graphics::Instance() {
    if (!instance) {
        instance = new Graphics();
    }
    return *instance;
}

bool Graphics::Init(Dim width, Dim height, const std::string& title) {
    if (initialized)
        return true;
    
    window.create(sf::VideoMode(width, height), title);
    
    // Use VSync for smooth rendering (better than hard frame limit)
    window.setVerticalSyncEnabled(true);
    // Keep frame limit as fallback
    window.setFramerateLimit(60);
    
    if (!backBuffer.create(width, height))
        return false;
    
    initialized = true;
    return true;
}

void Graphics::Shutdown() {
    if (initialized) {
        window.close();
        initialized = false;
    }
}

Dim Graphics::GetWidth() const {
    return initialized ? static_cast<Dim>(window.getSize().x) : 0;
}

Dim Graphics::GetHeight() const {
    return initialized ? static_cast<Dim>(window.getSize().y) : 0;
}

void Graphics::Clear() {
    Clear(backgroundColor);
}

void Graphics::Clear(Color c) {
    backBuffer.clear(ToSFMLColor(c));
}

void Graphics::Present() {
    backBuffer.display();
    
    sf::Sprite bufferSprite(backBuffer.getTexture());
    window.clear();
    window.draw(bufferSprite);
    window.display();
}

void Graphics::DrawRect(const Rect& r, Color c, bool filled) {
    sf::RectangleShape rect(sf::Vector2f(static_cast<float>(r.w), 
                                          static_cast<float>(r.h)));
    rect.setPosition(static_cast<float>(r.x), static_cast<float>(r.y));
    
    if (filled) {
        rect.setFillColor(ToSFMLColor(c));
        rect.setOutlineThickness(0);
    } else {
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(ToSFMLColor(c));
        rect.setOutlineThickness(1);
    }
    
    backBuffer.draw(rect);
}

void Graphics::DrawSprite(const sf::Sprite& sprite) {
    backBuffer.draw(sprite);
}

void Graphics::DrawTexture(const sf::Texture& tex, const Rect& src, const Point& dest) {
    sf::Sprite sprite(tex);
    sprite.setTextureRect(sf::IntRect(src.x, src.y, src.w, src.h));
    sprite.setPosition(static_cast<float>(dest.x), static_cast<float>(dest.y));
    backBuffer.draw(sprite);
}

void Graphics::DrawTextureScaled(const sf::Texture& tex, const Rect& src, const Rect& dest) {
    sf::Sprite sprite(tex);
    sprite.setTextureRect(sf::IntRect(src.x, src.y, src.w, src.h));
    sprite.setPosition(static_cast<float>(dest.x), static_cast<float>(dest.y));
    
    // Scale to fit destination rect
    float scaleX = static_cast<float>(dest.w) / static_cast<float>(src.w);
    float scaleY = static_cast<float>(dest.h) / static_cast<float>(src.h);
    sprite.setScale(scaleX, scaleY);
    
    backBuffer.draw(sprite);
}

void Graphics::DrawText(const std::string& text, int x, int y, Color c) {
    // Try to load font if not loaded
    if (!fontLoaded) {
        // Try common font locations
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
            "assets/font.ttf",
            "DejaVuSans-Bold.ttf"
        };
        
        for (const char* path : fontPaths) {
            if (defaultFont.loadFromFile(path)) {
                fontLoaded = true;
                break;
            }
        }
    }
    
    if (fontLoaded) {
        sf::Text sfText;
        sfText.setFont(defaultFont);
        sfText.setString(text);
        sfText.setCharacterSize(14);
        sfText.setFillColor(sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c)));
        sfText.setPosition(static_cast<float>(x), static_cast<float>(y));
        backBuffer.draw(sfText);
    } else {
        // Fallback: draw text as rectangles (one per character)
        int charWidth = 8;
        int charHeight = 12;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] != ' ') {
                DrawRect({x + static_cast<int>(i) * charWidth, y, charWidth - 2, charHeight}, c, true);
            }
        }
    }
}

// ============ BitmapLoader Implementation ============

BitmapLoader& BitmapLoader::Instance() {
    if (!instance) {
        instance = new BitmapLoader();
    }
    return *instance;
}

BitmapPtr BitmapLoader::Load(const std::string& path) {
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }
    
    auto bitmap = std::make_shared<Bitmap>();
    if (bitmap->LoadFromFile(path)) {
        cache[path] = bitmap;
        return bitmap;
    }
    return nullptr;
}

BitmapPtr BitmapLoader::Create(Dim w, Dim h) {
    return std::make_shared<Bitmap>(w, h);
}

void BitmapLoader::Clear() {
    cache.clear();
}

bool BitmapLoader::HasBitmap(const std::string& path) const {
    return cache.find(path) != cache.end();
}

BitmapPtr BitmapLoader::GetBitmap(const std::string& path) const {
    auto it = cache.find(path);
    return (it != cache.end()) ? it->second : nullptr;
}

} // namespace engine
