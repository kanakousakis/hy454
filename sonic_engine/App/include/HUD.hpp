#ifndef HUD_HPP
#define HUD_HPP

#include "Engine.hpp"
#include <string>
#include <cstdio>
#include <vector>

using namespace engine;

namespace app {

//============================================================
//score Popup - Floating score display when enemies killed/items collected
//============================================================
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
        velY *= 0.98f;  //slow down
        
        if (GetSystemTime() - spawnTime > DURATION) {
            active = false;
        }
    }
    
    void Draw(Graphics& gfx, int viewX, int viewY) {
        if (!active) return;
        
        int screenX = static_cast<int>(x) - viewX;
        int screenY = static_cast<int>(y) - viewY;
        
//skip if off screen - use virtual resolution (320×224)
        const int virtWidth = gfx.GetVirtualWidth();
        const int virtHeight = gfx.GetVirtualHeight();
        if (screenX < -50 || screenX > virtWidth || screenY < -20 || screenY > virtHeight) return;
        
//calculate fade based on time
        uint64_t elapsed = GetSystemTime() - spawnTime;
        int alpha = 255;
        if (elapsed > 800) {
            alpha = 255 - static_cast<int>((elapsed - 800) * 255 / 400);
            if (alpha < 0) alpha = 0;
        }
        
//background
        int width = (value >= 1000) ? 40 : (value >= 100) ? 32 : 24;
        gfx.DrawRect({screenX - 2, screenY - 2, width + 4, 16}, 
                     MakeColor(0, 0, 0, alpha / 2), true);
        
//score value color based on amount
        Color c;
        if (value >= 1000)
            c = MakeColor(255, 50, 50, alpha);  //red for 1000+
        else if (value >= 500)
            c = MakeColor(255, 150, 0, alpha);  //orange for 500
        else if (value >= 200)
            c = MakeColor(255, 255, 0, alpha);  //yellow for 200
        else
            c = MakeColor(255, 255, 255, alpha);  //white for 100
        
//draw digits
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
        popups.emplace_back(x, y - 20, value);  //start slightly above position
    }
    
    void Update() {
        for (auto& popup : popups) {
            popup.Update();
        }
        
//remove inactive
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

//============================================================
//HUD - Heads Up Display
//shows: Score, Time, Rings, Lives
//============================================================
class HUD {
public:
//game stats
    int score = 0;
    int rings = 0;
//NOTE: lives removed - HUD gets this from sonic->GetLives() to avoid dual tracking
    int timeSeconds = 0;
    uint64_t levelStartTime = 0;
    bool timePaused = false;
    
//display position - NATIVE resolution (640×448)
    int hudX = 10;
    int hudY = 10;
    
//ring flash animation (when rings are 0)
    bool ringFlash = false;
    uint64_t lastFlashTime = 0;
    static constexpr uint64_t FLASH_INTERVAL = 500;
    
//score popups manager
    ScorePopupManager popupManager;
    
    HUD() = default;
    
    void StartLevel(uint64_t currentTime) {
        levelStartTime = currentTime;
        timeSeconds = 0;
        timePaused = false;
        popupManager.Clear();
    }
    
    void Update(uint64_t currentTime) {
        if (!timePaused && levelStartTime > 0) {
            timeSeconds = static_cast<int>((currentTime - levelStartTime) / 1000);
        }
        
//ring flash when at 0
        if (rings == 0) {
            if (currentTime - lastFlashTime > FLASH_INTERVAL) {
                ringFlash = !ringFlash;
                lastFlashTime = currentTime;
            }
        } else {
            ringFlash = false;
        }
        
        popupManager.Update();
    }
    
    void AddScore(int points, float worldX = 0, float worldY = 0) {
        score += points;
        
//show popup if position given
        if (worldX != 0 || worldY != 0) {
            popupManager.Spawn(worldX, worldY, points);
        }
    }
    
    void AddRings(int count) {
        rings += count;
//NOTE: Extra life logic removed - should be handled in SonicPlayer
//when rings are collected in main.cpp, check sonic->GetRings() there
    }
    
    void LoseRings() {
        rings = 0;
    }
    
//NOTE: Lives functions removed - lives managed by SonicPlayer only
    void LoseLife() {
//disabled - use sonic->LoseLife() instead
    }
    
    void GainLife() {
//disabled - use sonic->AddLife() instead
    }
    
    bool IsGameOver() const {
//disabled - game over checked in death handler using sonic->GetLives()
        return false;
    }
    
    void PauseTime() { timePaused = true; }
    void ResumeTime() { timePaused = false; }
    
//draw the HUD at native 640×448 resolution
    void Draw(Graphics& gfx, SonicPlayer* sonic, int viewX = 0, int viewY = 0) {
        if (!sonic) return;
        int lives = sonic->GetLives();
        
        Color bg = MakeColor(0, 0, 0, 200);
        
//main HUD panel - BIGGER to fit 6-digit score
        gfx.DrawRect({10, 10, 180, 70}, bg, true);
        gfx.DrawRect({10, 10, 180, 70}, MakeColor(255, 255, 255, 50), false);
        
//SCORE
        gfx.DrawText("SCORE", 20, 20, MakeColor(255, 255, 0));
        DrawNumber(gfx, 100, 20, score, 6, MakeColor(255, 255, 255));
        
//TIME
        gfx.DrawText("TIME", 20, 40, MakeColor(255, 255, 0));
        int minutes = timeSeconds / 60;
        int seconds = timeSeconds % 60;
        DrawTime(gfx, 100, 40, minutes, seconds, MakeColor(255, 255, 255));
        
//RINGS
        Color ringColor = ringFlash ? MakeColor(255, 0, 0) : MakeColor(255, 255, 0);
        gfx.DrawText("RINGS", 20, 60, ringColor);
        DrawNumber(gfx, 100, 60, rings, 3, MakeColor(255, 255, 255));
        
//lives (bottom-left)
        gfx.DrawRect({10, 410, 80, 30}, bg, true);
        gfx.DrawRect({10, 410, 80, 30}, MakeColor(255, 255, 255, 50), false);
        
//sonic icon
        gfx.DrawRect({15, 415, 20, 20}, MakeColor(0, 100, 255), true);
        
//"x" and lives count
        gfx.DrawText("x", 40, 418, MakeColor(255, 255, 255));
        DrawNumber(gfx, 55, 418, lives, 1, MakeColor(255, 255, 255));
        
//score popups still use world coordinates
        popupManager.Draw(gfx, viewX, viewY);
    }
    
private:
//simple label drawing using rectangles (placeholder for text)
    void DrawLabel(Graphics& gfx, int x, int y, const std::string& label, Color c) {
        gfx.DrawText(label, x, y, c);
    }
    
//draw a number with leading zeros
    void DrawNumber(Graphics& gfx, int x, int y, int number, int digits, Color c) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%0*d", digits, number);
        gfx.DrawText(buffer, x, y, c);
    }
    
//draw time in M:SS format
    void DrawTime(Graphics& gfx, int x, int y, int minutes, int seconds, Color c) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
        gfx.DrawText(buffer, x, y, c);
    }
};

}  //namespace app

#endif  //HUD_HPP
