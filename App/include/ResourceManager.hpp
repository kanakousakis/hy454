#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include "Engine.hpp"
#include "SpriteSheetConfig.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <iostream>
#include <cmath>

namespace app {

// Singleton resource manager for loading and caching game assets
class ResourceManager {
private:
    // Sprite sheet bitmaps
    engine::BitmapPtr sonicSheet;
    engine::BitmapPtr enemiesSheet;
    engine::BitmapPtr miscSheet;
    engine::BitmapPtr animalsSheet;
    engine::BitmapPtr tilesSheet;
    engine::BitmapPtr backgroundSheet;
    
    // Animation films (created from sprite sheets)
    std::map<std::string, engine::AnimationFilm*> films;
    
    // Asset paths
    std::string assetPath = "assets/";
    
    bool loaded = false;
    
    ResourceManager() = default;
    
public:
    static ResourceManager& Instance() {
        static ResourceManager instance;
        return instance;
    }
    
    void SetAssetPath(const std::string& path) {
        assetPath = path;
        if (!assetPath.empty() && assetPath.back() != '/') {
            assetPath += '/';
        }
    }
    
    // Helper function to apply color key (make background transparent)
    void ApplyColorKey(engine::BitmapPtr& bmp, unsigned char r, unsigned char g, unsigned char b, int tolerance = 15) {
        if (!bmp) return;
        
        sf::Image& img = bmp->GetImage();
        auto size = img.getSize();
        
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = img.getPixel(x, y);
                // Check if pixel matches color key (with tolerance)
                int dr = std::abs(static_cast<int>(pixel.r) - static_cast<int>(r));
                int dg = std::abs(static_cast<int>(pixel.g) - static_cast<int>(g));
                int db = std::abs(static_cast<int>(pixel.b) - static_cast<int>(b));
                
                if (dr <= tolerance && dg <= tolerance && db <= tolerance) {
                    img.setPixel(x, y, sf::Color::Transparent);
                }
            }
        }
        
        // Update texture from modified image
        bmp->UpdateTexture();
    }
    
    // Apply color key to make backgrounds transparent
    // Handles: Magenta (tiles), All Green shades (Sonic/misc/enemies)
    // AGGRESSIVE green removal to prevent green halos on rings and other sprites
    void ApplyMultipleColorKeys(engine::BitmapPtr& bmp) {
        if (!bmp) return;

        sf::Image& img = bmp->GetImage();
        auto size = img.getSize();

        int transparentCount = 0;
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = img.getPixel(x, y);

                // Skip already transparent pixels
                if (pixel.a == 0) continue;

                bool isBackground = false;

                // 1. MAGENTA (Pink) - Common in Tilesets (255, 0, 255)
                if (pixel.r > 240 && pixel.g < 15 && pixel.b > 240) {
                    isBackground = true;
                }

                // 2. WHITE/GREY backgrounds - Remove light backgrounds
                // Any light grey/white/beige pixel is a background (NOT sprite content)
                // Check if R, G, B are all high and roughly equal (grey/white spectrum)
                else if (pixel.r >= 200 && pixel.g >= 200 && pixel.b >= 200 &&
                         std::abs(static_cast<int>(pixel.r) - static_cast<int>(pixel.g)) < 30 &&
                         std::abs(static_cast<int>(pixel.g) - static_cast<int>(pixel.b)) < 30) {
                    isBackground = true; // Remove white/grey/beige backgrounds
                }

                // 3. AGGRESSIVE GREEN DETECTION - Any pixel that's predominantly green
                // This catches ALL green shades including halos around rings
                // VERY AGGRESSIVE: Even slight green tint = background
                else if (pixel.g > pixel.r + 5 && pixel.g > pixel.b + 5 && pixel.g > 15) {
                    // Green channel is higher than red and blue (even slightly)
                    // This is a green-ish background pixel - remove it
                    isBackground = true;
                }

                // 3b. ANY greenish pixel (backup catch-all for stubborn greens)
                else if (pixel.g > 50 && pixel.g > pixel.r && pixel.g > pixel.b) {
                    isBackground = true;
                }

                // 4. Medium Green (0, 128, 0) - Standard green screen (WIDENED RANGE)
                else if (pixel.r < 30 && pixel.g >= 100 && pixel.g <= 160 && pixel.b < 30) {
                    isBackground = true;
                }

                // 5. Pure Green (0, 255, 0) and bright greens (WIDENED RANGE)
                else if (pixel.r < 40 && pixel.g > 180 && pixel.b < 40) {
                    isBackground = true;
                }

                // 6. Lighter Green key (147, 187, 148 variant) - Sonic sheet (WIDENED RANGE)
                else if (pixel.r >= 130 && pixel.r <= 165 &&
                    pixel.g >= 170 && pixel.g <= 205 &&
                    pixel.b >= 130 && pixel.b <= 165) {
                    isBackground = true;
                }

                // 7. Dark greens (any shade) - Catch darker green variants
                else if (pixel.r < 70 && pixel.g > 30 && pixel.g < 120 && pixel.b < 70 &&
                    pixel.g > pixel.r && pixel.g > pixel.b) {
                    isBackground = true;
                }

                if (isBackground) {
                    img.setPixel(x, y, sf::Color::Transparent);
                    transparentCount++;
                }
            }
        }

        // SECOND PASS: Clean up edge artifacts (semi-transparent green pixels)
        // This catches anti-aliased green edges that blend with sprites
        int edgeCleanupCount = 0;
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = img.getPixel(x, y);

                // Skip already transparent
                if (pixel.a == 0) continue;

                // If pixel has any green tint and low opacity, it's likely an edge artifact
                if (pixel.a < 255 && pixel.g > pixel.r + 10 && pixel.g > pixel.b + 10) {
                    img.setPixel(x, y, sf::Color::Transparent);
                    edgeCleanupCount++;
                }
                // Also remove semi-opaque greenish pixels
                else if (pixel.a < 200 && pixel.g > 100 &&
                         pixel.g > pixel.r && pixel.g > pixel.b) {
                    img.setPixel(x, y, sf::Color::Transparent);
                    edgeCleanupCount++;
                }
            }
        }

        std::cout << "    Made " << transparentCount << " pixels transparent";
        if (edgeCleanupCount > 0) {
            std::cout << " (+" << edgeCleanupCount << " edge cleanup)";
        }
        std::cout << std::endl;

        bmp->UpdateTexture();
    }
    
    bool LoadAll() {
        if (loaded) return true;
        
        std::cout << "Loading game assets from: " << assetPath << std::endl;
        
        auto& loader = engine::BitmapLoader::Instance();
        
        // Try to load with full path feedback
        std::string fullPath = assetPath + "sonic_sheet_fixed.png";
        std::cout << "  Trying to load: " << fullPath << std::endl;
        
        // Load sprite sheets
        sonicSheet = loader.Load(fullPath);
        if (!sonicSheet) {
            std::cerr << "  ERROR: Failed to load " << fullPath << std::endl;
            std::cerr << "  Make sure you're running from the sonic_engine directory" << std::endl;
            std::cerr << "  or that assets/ folder is in your current directory" << std::endl;
            return false;
        }
        std::cout << "  Loaded Sonic sprite sheet (" << sonicSheet->GetWidth() << "x" << sonicSheet->GetHeight() << ")" << std::endl;
        
        // Apply color key to make green background transparent
        // Sonic sheet has green background RGB(147, 187, 148) - use multi-key for safety
        ApplyMultipleColorKeys(sonicSheet);
        std::cout << "    Applied transparency to Sonic sheet" << std::endl;
        
        enemiesSheet = loader.Load(assetPath + "enemies_sheet_fixed.png");
        if (!enemiesSheet) {
            std::cerr << "Failed to load enemies_sheet_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded enemies sprite sheet (" << enemiesSheet->GetWidth() << "x" << enemiesSheet->GetHeight() << ")" << std::endl;
        // Enemies sheet has dark green background
        ApplyMultipleColorKeys(enemiesSheet);
        
        miscSheet = loader.Load(assetPath + "misc_fixed.png");
        if (!miscSheet) {
            std::cerr << "Failed to load misc_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded misc sprite sheet (" << miscSheet->GetWidth() << "x" << miscSheet->GetHeight() << ")" << std::endl;
        // Misc sheet has green background
        ApplyMultipleColorKeys(miscSheet);
        
        animalsSheet = loader.Load(assetPath + "animals_fixed.png");
        if (!animalsSheet) {
            std::cerr << "Failed to load animals_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded animals sprite sheet" << std::endl;
        
        tilesSheet = loader.Load(assetPath + "tiles_first_map_fixed.png");
        if (!tilesSheet) {
            std::cerr << "Failed to load tiles_first_map_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded tileset (" << tilesSheet->GetWidth() << "x" << tilesSheet->GetHeight() << ")" << std::endl;
        // Note: Tileset already has proper alpha transparency - no color keying needed
        
        // Optional: background/foreground layer
        backgroundSheet = loader.Load(assetPath + "background_foreground64.png");
        if (backgroundSheet) {
            std::cout << "  Loaded background sheet" << std::endl;
        }
        
        // Create animation films from sprite sheets
        CreateAnimationFilms();
        
        loaded = true;
        std::cout << "All assets loaded successfully!" << std::endl;
        return true;
    }
    
    void CreateAnimationFilms() {
        auto& filmHolder = engine::AnimationFilmHolder::Instance();
        
        // Helper lambda to add animation
        auto addAnim = [&](const AnimationDef& anim, engine::BitmapPtr sheet) {
            std::vector<engine::Rect> frames;
            for (const auto& fr : anim.frames) {
                frames.push_back({fr.x, fr.y, fr.w, fr.h});
            }
            filmHolder.AddFilm(anim.id, sheet, frames);
            std::cout << "    Film: " << anim.id << " (" << frames.size() << " frames)" << std::endl;
        };
        
        std::cout << "  Creating Sonic animations..." << std::endl;
        for (const auto& anim : SonicSpriteConfig::GetAnimations()) {
            addAnim(anim, sonicSheet);
        }
        
        std::cout << "  Creating Enemy animations..." << std::endl;
        // Motobug
        for (const auto& anim : EnemySpriteConfig::GetMotobugAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Crabmeat
        for (const auto& anim : EnemySpriteConfig::GetCrabmeatAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // BuzzBomber
        for (const auto& anim : EnemySpriteConfig::GetBuzzBomberAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Masher
        for (const auto& anim : EnemySpriteConfig::GetMasherAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Newtron Blue
        for (const auto& anim : EnemySpriteConfig::GetNewtronBlueAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Newtron Green
        for (const auto& anim : EnemySpriteConfig::GetNewtronGreenAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Bomb
        for (const auto& anim : EnemySpriteConfig::GetBombAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Caterkiller
        for (const auto& anim : EnemySpriteConfig::GetCaterkillerAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Spikes enemy (Yadorin)
        for (const auto& anim : EnemySpriteConfig::GetSpikesEnemyAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Batbrain
        for (const auto& anim : EnemySpriteConfig::GetBatbrainAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Burrobot
        for (const auto& anim : EnemySpriteConfig::GetBurrobotAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Roller
        for (const auto& anim : EnemySpriteConfig::GetRollerAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Jaws
        for (const auto& anim : EnemySpriteConfig::GetJawsAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Orbinaut
        for (const auto& anim : EnemySpriteConfig::GetOrbinautAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // BallHog
        for (const auto& anim : EnemySpriteConfig::GetBallHogAnimations()) {
            addAnim(anim, enemiesSheet);
        }
        // Explosion
        addAnim(EnemySpriteConfig::GetExplosionAnimation(), enemiesSheet);
        
        std::cout << "  Creating Misc animations..." << std::endl;
        // Ring
        addAnim(MiscSpriteConfig::GetRingAnimation(), miscSheet);
        addAnim(MiscSpriteConfig::GetRingCollectAnimation(), miscSheet);
        // Big Ring
        addAnim(MiscSpriteConfig::GetBigRingAnimation(), miscSheet);
        // Invincibility
        addAnim(MiscSpriteConfig::GetInvincibilityAnimation(), miscSheet);
        // Shield
        addAnim(MiscSpriteConfig::GetShieldAnimation(), miscSheet);
        // Checkpoint
        addAnim(MiscSpriteConfig::GetCheckpointAnimation(), miscSheet);
        // Springs
        addAnim(MiscSpriteConfig::GetYellowSpringAnimation(), miscSheet);
        addAnim(MiscSpriteConfig::GetRedSpringAnimation(), miscSheet);
        // Ring loss
        addAnim(MiscSpriteConfig::GetRingLossAnimation(), miscSheet);
        
        std::cout << "  Creating Animal animations..." << std::endl;
        // All animals
        addAnim(AnimalSpriteConfig::GetFlickyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetPockyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetCuckyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetRickyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetPeckyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetRockyAnimation(), animalsSheet);
        
        std::cout << "  Animation films created!" << std::endl;
    }
    
    // Getters
    engine::BitmapPtr GetSonicSheet() const { return sonicSheet; }
    engine::BitmapPtr GetEnemiesSheet() const { return enemiesSheet; }
    engine::BitmapPtr GetMiscSheet() const { return miscSheet; }
    engine::BitmapPtr GetAnimalsSheet() const { return animalsSheet; }
    engine::BitmapPtr GetTilesSheet() const { return tilesSheet; }
    engine::BitmapPtr GetBackgroundSheet() const { return backgroundSheet; }
    
    engine::AnimationFilm* GetFilm(const std::string& id) {
        return engine::AnimationFilmHolder::Instance().GetFilm(id);
    }
    
    bool IsLoaded() const { return loaded; }
    
    void Cleanup() {
        // BitmapPtr are shared_ptr so they clean up automatically
        sonicSheet = nullptr;
        enemiesSheet = nullptr;
        miscSheet = nullptr;
        animalsSheet = nullptr;
        tilesSheet = nullptr;
        backgroundSheet = nullptr;
        films.clear();
        loaded = false;
    }
};

} // namespace app

#endif // RESOURCE_MANAGER_HPP
