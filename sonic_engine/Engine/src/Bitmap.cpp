#include "Bitmap.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

namespace engine {

//static instance pointers
Graphics* Graphics::instance = nullptr;
BitmapLoader* BitmapLoader::instance = nullptr;

//============ Bitmap Implementation ============

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
    
//pixel-perfect rendering settings
    texture.setSmooth(false);  //no interpolation
    texture.setRepeated(false);  //clamp to edge
    
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
    
    texture.setSmooth(false);
    
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
        texture.setSmooth(false);  //maintain pixel-perfect rendering
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

//============ Graphics Implementation ============

Graphics& Graphics::Instance() {
    if (!instance) {
        instance = new Graphics();
    }
    return *instance;
}

bool Graphics::Init(Dim width, Dim height, const std::string& title) {
    if (initialized)
        return true;
    
//store windowed dimensions and title
    windowedWidth = width;
    windowedHeight = height;
    windowTitle = title;
    isFullscreen = false;
    
    window.create(sf::VideoMode(width, height), title);
    
//frame limiting for consistent speed (60 FPS)
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(60);
    
//create backbuffer at window size initially
//(will be changed by SetVirtualResolution if called)
    if (!backBuffer.create(width, height))
        return false;
    
//CRITICAL: Disable smoothing for pixel-perfect rendering
    backBuffer.setSmooth(false);
    
//default virtual resolution = window size
    virtualWidth = width;
    virtualHeight = height;
    
    initialized = true;
    
    std::cout << "Graphics initialized: " << width << "x" << height << std::endl;
    
    return true;
}

void Graphics::ToggleFullscreen() {
    if (!initialized) return;
    
    window.close();
    isFullscreen = !isFullscreen;
    
    if (isFullscreen) {
//switch to fullscreen at desktop resolution
        auto desktop = sf::VideoMode::getDesktopMode();
        window.create(desktop, windowTitle, sf::Style::Fullscreen);
        std::cout << "Fullscreen: " << desktop.width << "x" << desktop.height << std::endl;
    } else {
//switch back to windowed mode
        window.create(sf::VideoMode(windowedWidth, windowedHeight), windowTitle);
        std::cout << "Windowed: " << windowedWidth << "x" << windowedHeight << std::endl;
    }
    
//CRITICAL: Reset view to default for new window
    window.setView(window.getDefaultView());

//use different settings for fullscreen vs windowed
    if (isFullscreen) {
//fullscreen: use VSync for smoother performance at high resolutions
        window.setVerticalSyncEnabled(true);
        window.setFramerateLimit(0);  //disable frame limit when using VSync
    } else {
//windowed: use frame limit for consistent 60 FPS
        window.setVerticalSyncEnabled(false);
        window.setFramerateLimit(60);
    }
}

//set virtual resolution for scaling (call after Init)
void Graphics::SetVirtualResolution(Dim vWidth, Dim vHeight) {
    virtualWidth = vWidth;
    virtualHeight = vHeight;
    
//recreate backbuffer at virtual resolution
    backBuffer.create(vWidth, vHeight);
    
//CRITICAL: Disable smoothing for pixel-perfect upscaling
    backBuffer.setSmooth(false);
    
    std::cout << "Virtual resolution: " << vWidth << "x" << vHeight << std::endl;
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
    window.setActive(true);
    backBuffer.display();
    
//get sizes
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);
    float bufW = static_cast<float>(backBuffer.getSize().x);
    float bufH = static_cast<float>(backBuffer.getSize().y);
    
//CRITICAL: Set view to match window pixels 1:1 (fixes fullscreen positioning)
    sf::View pixelView(sf::FloatRect(0, 0, winW, winH));
    window.setView(pixelView);
    
//calculate integer scale (pixel-perfect)
    int scaleX = static_cast<int>(winW / bufW);
    int scaleY = static_cast<int>(winH / bufH);
    int scale = std::max(1, std::min(scaleX, scaleY));
    
//store scale for native rendering
    nativeScale = scale;
    nativeOffsetX = (winW - bufW * scale) / 2.0f;
    nativeOffsetY = (winH - bufH * scale) / 2.0f;
    
//scaled dimensions
    float scaledW = bufW * static_cast<float>(scale);
    float scaledH = bufH * static_cast<float>(scale);
    
//center position
    float posX = (winW - scaledW) / 2.0f;
    float posY = (winH - scaledH) / 2.0f;
    
//create and position sprite
    sf::Sprite bufferSprite(backBuffer.getTexture());
    bufferSprite.setScale(static_cast<float>(scale), static_cast<float>(scale));
    bufferSprite.setPosition(posX, posY);
    
//clear and draw game
    window.clear(sf::Color::Black);
    window.draw(bufferSprite);
//don't display yet - allow native rendering
}

void Graphics::DrawNativeText(const std::string& text, int x, int y, Color c) {
//draw text directly to window at native resolution
//x, y are in WINDOW coordinates (640x448), not virtual
//add offset to account for letterboxing
    float realX = nativeOffsetX + static_cast<float>(x);
    float realY = nativeOffsetY + static_cast<float>(y);
    
    if (!fontLoaded) {
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
            "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"
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
        sfText.setCharacterSize(14);  //native size
        sfText.setFillColor(sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c)));
        sfText.setPosition(realX, realY);
        window.draw(sfText);
    }
}

void Graphics::DrawNativeRect(const Rect& r, Color c, bool filled) {
//draw rect directly to window at native resolution
//coordinates are in WINDOW space (640x448)
    float realX = nativeOffsetX + static_cast<float>(r.x);
    float realY = nativeOffsetY + static_cast<float>(r.y);
    float realW = static_cast<float>(r.w);
    float realH = static_cast<float>(r.h);

    sf::RectangleShape rect(sf::Vector2f(realW, realH));
    rect.setPosition(realX, realY);
    if (filled) {
        rect.setFillColor(sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c)));
        rect.setOutlineThickness(0);
    } else {
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c)));
        rect.setOutlineThickness(1);
    }
    window.draw(rect);
}

void Graphics::DrawNativeSprite(const sf::Sprite& sprite, int x, int y) {
//draw sprite directly to window at native resolution
//x, y are in WINDOW coordinates (640x448), not virtual
//add offset to account for letterboxing
    float realX = nativeOffsetX + static_cast<float>(x);
    float realY = nativeOffsetY + static_cast<float>(y);

    sf::Sprite nativeSprite = sprite;
    nativeSprite.setPosition(realX, realY);
    window.draw(nativeSprite);
}

void Graphics::FinalDisplay() {
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
    
//scale to fit destination rect
    float scaleX = static_cast<float>(dest.w) / static_cast<float>(src.w);
    float scaleY = static_cast<float>(dest.h) / static_cast<float>(src.h);
    sprite.setScale(scaleX, scaleY);
    
    backBuffer.draw(sprite);
}

void Graphics::DrawText(const std::string& text, int x, int y, Color c) {
//try to load font if not loaded
    if (!fontLoaded) {
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
        sfText.setCharacterSize(8);  //for 160x112 resolution
        sfText.setFillColor(sf::Color(GetRed(c), GetGreen(c), GetBlue(c), GetAlpha(c)));
        sfText.setPosition(static_cast<float>(x), static_cast<float>(y));
        backBuffer.draw(sfText);
    } else {
//fallback: draw rectangles for each character
        int charWidth = 4;
        int charHeight = 6;
        
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] != ' ') {
                DrawRect({x + static_cast<int>(i) * charWidth, y, charWidth - 1, charHeight}, c, true);
            }
        }
    }
}

//============ BitmapLoader Implementation ============

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

}  //namespace engine
