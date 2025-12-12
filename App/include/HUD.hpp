#ifndef HUD_HPP
#define HUD_HPP

#include "Engine.hpp"
#include <string>
#include <cstdio>
#include <vector>

using namespace engine;

namespace app {

// ============================================================
// Score Popup - Floating score display when enemies killed/items collected
// ============================================================
struct ScorePopup {
    float x, y;
    float velY = -2.0f;
    int value;
    uint64_t spawnTime;
    bool active = true;
    
    static constexpr uint64_t DURATION = 1200;
    
    ScorePopup(float px, float py, int val) 
        : x(px), y(py), value(val), spawnTime(GetSystemTime()) {}
    
    void Update() {
        if (!active) return;
        
        y += velY;
        velY *= 0.98f;  // Slow down
        
        if (GetSystemTime() - spawnTime > DURATION) {
            active = false;
        }
    }
    
    void Draw(Graphics& gfx, int viewX, int viewY) {
        if (!active) return;
        
        int screenX = static_cast<int>(x) - viewX;
        int screenY = static_cast<int>(y) - viewY;
        
        // Skip if off screen
        if (screenX < -50 || screenX > 690 || screenY < -20 || screenY > 500) return;
        
        // Calculate fade based on time
        uint64_t elapsed = GetSystemTime() - spawnTime;
        int alpha = 255;
        if (elapsed > 800) {
            alpha = 255 - static_cast<int>((elapsed - 800) * 255 / 400);
            if (alpha < 0) alpha = 0;
        }
        
        // Background
        int width = (value >= 1000) ? 40 : (value >= 100) ? 32 : 24;
        gfx.DrawRect({screenX - 2, screenY - 2, width + 4, 16}, 
                     MakeColor(0, 0, 0, alpha / 2), true);
        
        // Score value color based on amount
        Color c;
        if (value >= 1000)
            c = MakeColor(255, 50, 50, alpha);   // Red for 1000+
        else if (value >= 500)
            c = MakeColor(255, 150, 0, alpha);   // Orange for 500
        else if (value >= 200)
            c = MakeColor(255, 255, 0, alpha);   // Yellow for 200
        else
            c = MakeColor(255, 255, 255, alpha); // White for 100
        
        // Draw digits
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%d", value);
        int charWidth = 8;
        for (int i = 0; buffer[i] != '\0'; ++i) {
            gfx.DrawRect({screenX + i * charWidth, screenY, 6, 12}, c, true);
        }
    }
};

class ScorePopupManager {
private:
    std::vector<ScorePopup> popups;
    
public:
    void Spawn(float x, float y, int value) {
        popups.emplace_back(x, y - 20, value);  // Start slightly above position
    }
    
    void Update() {
        for (auto& popup : popups) {
            popup.Update();
        }
        
        // Remove inactive
        popups.erase(
            std::remove_if(popups.begin(), popups.end(),
                [](const ScorePopup& p) { return !p.active; }),
            popups.end()
        );
    }
    
    void Draw(Graphics& gfx, int viewX, int viewY) {
        for (auto& popup : popups) {
            popup.Draw(gfx, viewX, viewY);
        }
    }
    
    void Clear() { popups.clear(); }
};

// ============================================================
// HUD - Heads Up Display
// Shows: Score, Time, Rings, Lives
// ============================================================
class HUD {
public:
    // Game stats
    int score = 0;
    int rings = 0;
    int lives = 3;
    int timeSeconds = 0;
    uint64_t levelStartTime = 0;
    bool timePaused = false;
    
    // Display position
    int hudX = 16;
    int hudY = 8;
    
    // Ring flash animation (when rings are 0)
    bool ringFlash = false;
    uint64_t lastFlashTime = 0;
    static constexpr uint64_t FLASH_INTERVAL = 500;
    
    // Score popups manager
    ScorePopupManager popupManager;
    
    HUD() = default;
    
    void StartLevel(uint64_t currentTime) {
        levelStartTime = currentTime;
        timeSeconds = 0;
        timePaused = false;
        popupManager.Clear();
    }
    
    void Update(uint64_t currentTime) {
        // Update time
        if (!timePaused && levelStartTime > 0) {
            timeSeconds = static_cast<int>((currentTime - levelStartTime) / 1000);
        }
        
        // Ring flash when at 0
        if (rings == 0) {
            if (currentTime - lastFlashTime > FLASH_INTERVAL) {
                ringFlash = !ringFlash;
                lastFlashTime = currentTime;
            }
        } else {
            ringFlash = false;
        }
        
        // Update popups
        popupManager.Update();
    }
    
    void AddScore(int points, float worldX = 0, float worldY = 0) {
        score += points;
        
        // Show popup if position given
        if (worldX != 0 || worldY != 0) {
            popupManager.Spawn(worldX, worldY, points);
        }
    }
    
    void AddRings(int count) {
        rings += count;
        // Check for extra life
        if (rings >= 100) {
            rings -= 100;
            lives++;
            // Could play 1-up sound here
        }
    }
    
    void LoseRings() {
        rings = 0;
    }
    
    void LoseLife() {
        if (lives > 0) {
            lives--;
        }
    }
    
    void GainLife() {
        lives++;
    }
    
    bool IsGameOver() const {
        return lives <= 0;
    }
    
    void PauseTime() { timePaused = true; }
    void ResumeTime() { timePaused = false; }
    
    // Draw the HUD
    void Draw(Graphics& gfx, int viewX = 0, int viewY = 0) {
        // Background panels
        Color panelColor = MakeColor(0, 0, 0, 180);
        
        // === LEFT PANEL: Score, Time, Rings ===
        gfx.DrawRect({hudX - 4, hudY - 4, 180, 72}, panelColor, true);
        
        // SCORE
        DrawLabel(gfx, hudX, hudY, "SCORE", MakeColor(255, 255, 0));
        DrawNumber(gfx, hudX + 70, hudY, score, 6, MakeColor(255, 255, 255));
        
        // TIME
        DrawLabel(gfx, hudX, hudY + 20, "TIME", MakeColor(255, 255, 0));
        int minutes = timeSeconds / 60;
        int seconds = timeSeconds % 60;
        DrawTime(gfx, hudX + 70, hudY + 20, minutes, seconds, MakeColor(255, 255, 255));
        
        // RINGS
        Color ringLabelColor = ringFlash ? MakeColor(255, 0, 0) : MakeColor(255, 255, 0);
        DrawLabel(gfx, hudX, hudY + 40, "RINGS", ringLabelColor);
        DrawNumber(gfx, hudX + 70, hudY + 40, rings, 3, MakeColor(255, 255, 255));
        
        // === LIVES (bottom left) ===
        int livesY = 480 - 40;  // Near bottom
        gfx.DrawRect({hudX - 4, livesY - 4, 60, 28}, panelColor, true);
        
        // Sonic icon placeholder (small blue circle)
        gfx.DrawRect({hudX, livesY, 16, 16}, MakeColor(0, 100, 255), true);
        
        // "x" and number
        DrawLabel(gfx, hudX + 20, livesY, "x", MakeColor(255, 255, 255));
        DrawNumber(gfx, hudX + 32, livesY, lives, 1, MakeColor(255, 255, 255));
        
        // === SCORE POPUPS ===
        popupManager.Draw(gfx, viewX, viewY);
    }
    
private:
    // Simple label drawing using rectangles (placeholder for text)
    void DrawLabel(Graphics& gfx, int x, int y, const std::string& label, Color c) {
        gfx.DrawText(label, x, y, c);
    }
    
    // Draw a number with leading zeros
    void DrawNumber(Graphics& gfx, int x, int y, int number, int digits, Color c) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%0*d", digits, number);
        gfx.DrawText(buffer, x, y, c);
    }
    
    // Draw time in M:SS format
    void DrawTime(Graphics& gfx, int x, int y, int minutes, int seconds, Color c) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
        gfx.DrawText(buffer, x, y, c);
    }
};

} // namespace app

#endif // HUD_HPP
