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

                // 2. Medium Green (0, 128, 0) - Standard green screen
                else if (pixel.r < 20 && pixel.g >= 120 && pixel.g <= 140 && pixel.b < 20) {
                    isBackground = true;
                }

                // 3. Pure Green (0, 255, 0)
                else if (pixel.r < 30 && pixel.g > 200 && pixel.b < 30) {
                    isBackground = true;
                }

                // 4. Light Green (Sonic sheet background ~147, 187, 148)
                else if (pixel.r >= 140 && pixel.r <= 155 &&
                         pixel.g >= 180 && pixel.g <= 195 &&
                         pixel.b >= 140 && pixel.b <= 155) {
                    isBackground = true;
                }

                // 5. INTELLIGENT GREEN DETECTION - Distinguishes green from yellow
                // Yellow rings: R≈G (both high), B low  →  KEEP
                // Green background: G >> R              →  REMOVE
                // Logic: If green is much higher than red, it's a green background
                else if (pixel.r < pixel.g - 30 && pixel.g > 80 && pixel.b < pixel.g - 20) {
                    // G is 30+ points higher than R, and B is also low
                    // This is GREEN background, not yellow sprite
                    isBackground = true;
                }

                // 6. BLACK and NEAR-BLACK edge artifacts (common sprite edge issue)
                else if (pixel.r < 15 && pixel.g < 15 && pixel.b < 15) {
                    isBackground = true;
                }

                // 7. DARK GREEN edge artifacts (greenish but very dark)
                else if (pixel.g > pixel.r + 8 && pixel.g > pixel.b + 8 &&
                         pixel.g < 40 && pixel.r < 30 && pixel.b < 30) {
                    // Green channel is dominant but overall very dark
                    isBackground = true;
                }

                // 8. ANTI-ALIASED GREEN EDGES - Wide range around base green (147, 187, 148)
                // Catches blended edge pixels from sprite anti-aliasing
                else if (pixel.r >= 100 && pixel.r <= 180 &&
                         pixel.g >= 140 && pixel.g <= 210 &&
                         pixel.b >= 100 && pixel.b <= 180 &&
                         pixel.g > pixel.r + 15 && pixel.g > pixel.b + 15) {
                    // Greenish color similar to background with green dominant
                    isBackground = true;
                }

                // 9. SUBTLE GREEN TINT - Any pixel with slight green dominance
                // Catches faint green halos on edges
                else if (pixel.g > pixel.r + 10 && pixel.g > pixel.b + 10 &&
                         pixel.g > 50 && pixel.g < 220 &&
                         !(pixel.r > 180 && pixel.g > 180 && pixel.b < 100)) {
                    // Green is clearly dominant (not yellow rings where R≈G)
                    // Exclude bright yellow (high R+G, low B)
                    isBackground = true;
                }

                // 10. MEDIUM GREEN SHADES - Variations of green screen
                else if (pixel.r < 60 && pixel.g >= 80 && pixel.g <= 180 && pixel.b < 60) {
                    isBackground = true;
                }

                // 11. ULTRA-AGGRESSIVE: Darker black edge artifacts
                else if (pixel.r < 20 && pixel.g < 20 && pixel.b < 20) {
                    isBackground = true;
                }

                // 12. ULTRA-AGGRESSIVE: Very subtle green tint (lower threshold)
                // Any pixel with even slight green dominance
                else if (pixel.g > pixel.r + 5 && pixel.g > pixel.b + 5 &&
                         pixel.g > 40 && pixel.g < 220 &&
                         !(pixel.r > 180 && pixel.g > 180 && pixel.b < 100)) {
                    // Even subtle green dominance = remove (except yellow rings)
                    isBackground = true;
                }

                // 13. ULTRA-AGGRESSIVE: Dark greenish pixels (any darkness with green tint)
                else if (pixel.g > pixel.r && pixel.g > pixel.b &&
                         (pixel.r + pixel.g + pixel.b) < 120 &&
                         pixel.g > 15) {
                    // Dark pixel with any green dominance
                    isBackground = true;
                }

                // 14. ULTRA-AGGRESSIVE: Grayish-green mid-tones
                else if (pixel.r >= 60 && pixel.r <= 100 &&
                         pixel.g >= 80 && pixel.g <= 140 &&
                         pixel.b >= 60 && pixel.b <= 100 &&
                         pixel.g > pixel.r + 5 && pixel.g > pixel.b + 5) {
                    // Mid-tone gray with green bias
                    isBackground = true;
                }

                if (isBackground) {
                    img.setPixel(x, y, sf::Color::Transparent);
                    transparentCount++;
                }
            }
        }

        std::cout << "    Made " << transparentCount << " pixels transparent" << std::endl;

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
