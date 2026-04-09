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

//singleton resource manager for loading and caching game assets
class ResourceManager {
private:
//sprite sheet bitmaps
    engine::BitmapPtr sonicSheet;
    engine::BitmapPtr enemiesSheet;
    engine::BitmapPtr miscSheet;
    engine::BitmapPtr animalsSheet;
    engine::BitmapPtr tilesSheet;
    engine::BitmapPtr backgroundSheet;
    engine::BitmapPtr flowersSheet;
    
//animation films (created from sprite sheets)
    std::map<std::string, engine::AnimationFilm*> films;
    
//asset paths
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
    
//helper function to apply color key (make background transparent)
    void ApplyColorKey(engine::BitmapPtr& bmp, unsigned char r, unsigned char g, unsigned char b, int tolerance = 15) {
        if (!bmp) return;
        
        sf::Image& img = bmp->GetImage();
        auto size = img.getSize();
        
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = img.getPixel(x, y);
//check if pixel matches color key (with tolerance)
                int dr = std::abs(static_cast<int>(pixel.r) - static_cast<int>(r));
                int dg = std::abs(static_cast<int>(pixel.g) - static_cast<int>(g));
                int db = std::abs(static_cast<int>(pixel.b) - static_cast<int>(b));
                
                if (dr <= tolerance && dg <= tolerance && db <= tolerance) {
                    img.setPixel(x, y, sf::Color::Transparent);
                }
            }
        }
        
        bmp->UpdateTexture();
    }
    
//apply color key to make backgrounds transparent
//handles: Magenta (tiles), Dark Green (Sonic/misc), Pure Green, Medium Green, Enemy Sheet Green
    void ApplyMultipleColorKeys(engine::BitmapPtr& bmp) {
        if (!bmp) return;
        
        sf::Image& img = bmp->GetImage();
        auto size = img.getSize();
        
        int transparentCount = 0;
        for (unsigned y = 0; y < size.y; ++y) {
            for (unsigned x = 0; x < size.x; ++x) {
                sf::Color pixel = img.getPixel(x, y);
                
                bool isBackground = false;
                
//1. MAGENTA (Pink) - Common in Tilesets (255, 0, 255)
                if (pixel.r > 240 && pixel.g < 15 && pixel.b > 240) {
                    isBackground = true;
                }
                
//2. Medium Green (0, 128, 0) - Used in misc/enemies sheets
//EXPANDED tolerance to catch more variants
                else if (pixel.r < 30 && pixel.g >= 110 && pixel.g <= 145 && pixel.b < 30) {
                    isBackground = true;
                }
                
//3. Dark Green used in some Sonic sheets (darker variant)
                else if (pixel.r < 60 && pixel.g > 25 && pixel.g < 90 && pixel.b < 60 &&
                    pixel.g > pixel.r && pixel.g > pixel.b) {
                    isBackground = true;
                }
                
//4. Pure Green (0, 255, 0) - BRIGHT GREEN (most common in sprite sheets)
//be aggressive here - catch all bright greens
                else if (pixel.r < 50 && pixel.g > 180 && pixel.b < 50) {
                    isBackground = true;
                }
                
//5. Enemy Sheet Dark Green (RGB ~35-50, 95-110, 20-40)
//this is the specific green used in enemies_sheet_fixed.png
//sample colors: RGB(38, 103, 26), RGB(39, 102, 26), RGB(36, 103, 26)
                else if (pixel.r >= 25 && pixel.r <= 55 && 
                         pixel.g >= 90 && pixel.g <= 115 && 
                         pixel.b >= 15 && pixel.b <= 45 &&
                         pixel.g > pixel.r + 40 && pixel.g > pixel.b + 40) {
                    isBackground = true;
                }
                
//5. Lighter Green key (147, 187, 148 variant) - Sonic sheet
                else if (pixel.r >= 140 && pixel.r <= 155 &&
                    pixel.g >= 180 && pixel.g <= 195 &&
                    pixel.b >= 140 && pixel.b <= 155) {
                    isBackground = true;
                }
                
//6. NEW: Catch any pixel where green is dominant (generic green detection)
//this helps catch any green variants that slip through
                else if (pixel.r < 80 && pixel.b < 80 && pixel.g > 90 &&
                         pixel.g > pixel.r * 1.8 && pixel.g > pixel.b * 1.8) {
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
        
//try to load with full path feedback
        std::string fullPath = assetPath + "sonic_sheet_fixed.png";
        std::cout << "  Trying to load: " << fullPath << std::endl;
        
//load sprite sheets
        sonicSheet = loader.Load(fullPath);
        if (!sonicSheet) {
            std::cerr << "  ERROR: Failed to load " << fullPath << std::endl;
            std::cerr << "  Make sure you're running from the sonic_engine directory" << std::endl;
            std::cerr << "  or that assets/ folder is in your current directory" << std::endl;
            return false;
        }
        std::cout << "  Loaded Sonic sprite sheet (" << sonicSheet->GetWidth() << "x" << sonicSheet->GetHeight() << ")" << std::endl;
        
//apply color key to make green background transparent
//sonic sheet has green background RGB(147, 187, 148) - use multi-key for safety
        ApplyMultipleColorKeys(sonicSheet);
        std::cout << "    Applied transparency to Sonic sheet" << std::endl;
        
        enemiesSheet = loader.Load(assetPath + "enemies_sheet_fixed.png");
        if (!enemiesSheet) {
            std::cerr << "Failed to load enemies_sheet_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded enemies sprite sheet (" << enemiesSheet->GetWidth() << "x" << enemiesSheet->GetHeight() << ")" << std::endl;
//enemies sheet has dark green background
        ApplyMultipleColorKeys(enemiesSheet);
        
        miscSheet = loader.Load(assetPath + "misc_fixed.png");
        if (!miscSheet) {
            std::cerr << "Failed to load misc_fixed.png" << std::endl;
            return false;
        }
        std::cout << "  Loaded misc sprite sheet (" << miscSheet->GetWidth() << "x" << miscSheet->GetHeight() << ")" << std::endl;
//misc sheet has green background
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
//note: Tileset already has proper alpha transparency - no color keying needed
        
//optional: background/foreground layer
        backgroundSheet = loader.Load(assetPath + "background_foreground64.png");
        if (backgroundSheet) {
            std::cout << "  Loaded background sheet" << std::endl;
        }
        
//load flowers sprite sheet
        flowersSheet = loader.Load(assetPath + "flowers.png");
        if (flowersSheet) {
            std::cout << "  Loaded flowers sprite sheet" << std::endl;
//flowers.png has pink background - apply comprehensive color keying
            ApplyMultipleColorKeys(flowersSheet);  //use multi-key to catch all backgrounds
            ApplyColorKey(flowersSheet, 255, 192, 203, 50);  //light pink - higher tolerance
            ApplyColorKey(flowersSheet, 255, 182, 193, 50);  //another pink variant
        }
        
//create animation films from sprite sheets
        CreateAnimationFilms();
        
        loaded = true;
        std::cout << "All assets loaded successfully!" << std::endl;
        return true;
    }
    
    void CreateAnimationFilms() {
        auto& filmHolder = engine::AnimationFilmHolder::Instance();
        
//helper lambda to add animation
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
//motobug
        for (const auto& anim : EnemySpriteConfig::GetMotobugAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//crabmeat
        for (const auto& anim : EnemySpriteConfig::GetCrabmeatAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//buzzBomber
        for (const auto& anim : EnemySpriteConfig::GetBuzzBomberAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//masher
        for (const auto& anim : EnemySpriteConfig::GetMasherAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//newtron Blue
        for (const auto& anim : EnemySpriteConfig::GetNewtronBlueAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//newtron Green
        for (const auto& anim : EnemySpriteConfig::GetNewtronGreenAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//bomb
        for (const auto& anim : EnemySpriteConfig::GetBombAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//caterkiller
        for (const auto& anim : EnemySpriteConfig::GetCaterkillerAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//spikes enemy (Yadorin)
        for (const auto& anim : EnemySpriteConfig::GetSpikesEnemyAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//batbrain
        for (const auto& anim : EnemySpriteConfig::GetBatbrainAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//burrobot
        for (const auto& anim : EnemySpriteConfig::GetBurrobotAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//roller
        for (const auto& anim : EnemySpriteConfig::GetRollerAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//jaws
        for (const auto& anim : EnemySpriteConfig::GetJawsAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//orbinaut
        for (const auto& anim : EnemySpriteConfig::GetOrbinautAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//ballHog
        for (const auto& anim : EnemySpriteConfig::GetBallHogAnimations()) {
            addAnim(anim, enemiesSheet);
        }
//explosion
        addAnim(EnemySpriteConfig::GetExplosionAnimation(), enemiesSheet);
        
        std::cout << "  Creating Misc animations..." << std::endl;
//ring
        addAnim(MiscSpriteConfig::GetRingAnimation(), miscSheet);
        addAnim(MiscSpriteConfig::GetRingCollectAnimation(), miscSheet);
//big Ring
        addAnim(MiscSpriteConfig::GetBigRingAnimation(), miscSheet);
//invincibility
        addAnim(MiscSpriteConfig::GetInvincibilityAnimation(), miscSheet);
//shield
        addAnim(MiscSpriteConfig::GetShieldAnimation(), miscSheet);
//checkpoint
        addAnim(MiscSpriteConfig::GetCheckpointAnimation(), miscSheet);
//springs
        addAnim(MiscSpriteConfig::GetYellowSpringAnimation(), miscSheet);
        addAnim(MiscSpriteConfig::GetRedSpringAnimation(), miscSheet);
        addAnim(MiscSpriteConfig::GetSideSpringAnimation(), miscSheet);
//ring loss
        addAnim(MiscSpriteConfig::GetRingLossAnimation(), miscSheet);
        
//flowers (decorative - from flowers.png sheet)
        std::cout << "  Creating Flower animations (from flowers.png)..." << std::endl;
        if (flowersSheet) {
            addAnim(TileConfig::GetFlowerTallAnimation(), flowersSheet);
            addAnim(TileConfig::GetFlowerShortAnimation(), flowersSheet);
        } else {
            std::cerr << "  Warning: flowers.png not loaded, using tiles sheet fallback" << std::endl;
            addAnim(TileConfig::GetFlowerTallAnimation(), tilesSheet);
            addAnim(TileConfig::GetFlowerShortAnimation(), tilesSheet);
        }
        
        std::cout << "  Creating Animal animations..." << std::endl;
//all animals - loaded from animals sheet (animals_fixed.png)
        addAnim(AnimalSpriteConfig::GetFlickyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetPockyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetCuckyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetRickyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetPeckyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetRockyAnimation(), animalsSheet);
        addAnim(AnimalSpriteConfig::GetPickyAnimation(), animalsSheet);  //new: Picky pig
        
        std::cout << "  Animation films created!" << std::endl;
    }
    
//getters
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
        sonicSheet = nullptr;
        enemiesSheet = nullptr;
        miscSheet = nullptr;
        animalsSheet = nullptr;
        tilesSheet = nullptr;
        backgroundSheet = nullptr;
        flowersSheet = nullptr;
        films.clear();
        loaded = false;
    }
};

}  //namespace app

#endif  //RESOURCE_MANAGER_HPP
