#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include <memory>
#include "SystemClock.hpp"

namespace app {

//sound effect types that can be played
enum class SoundEffect {
    Jump,
    Ring,
    RingLoss,
    Spin,
    SpindashCharge,
    SpindashRelease,
    Spring,
    Checkpoint,
    Shield,
    Invincibility,
    SpeedShoes,
    ExtraLife,
    Hurt,
    Death,
    EnemyDestroy,
    MonitorBreak,
    BossHit,
    GameOver,
    ActClear,
    Splash,
    Spike,
    COUNT  //for array sizing
};

//music tracks that can be played
enum class MusicTrack {
    None,
    TitleScreen,
    GreenHillZone,
    MarbleZone,
    SpringYardZone,
    LabyrinthZone,
    StarLightZone,
    ScrapBrainZone,
    FinalZone,
    Invincibility,  //temporary track, resumes previous
    SpeedShoes,  //temporary track, resumes previous
    BossBattle,
    ActClear,
    GameOver,
    Drowning,
    Credits,
    COUNT
};

//music state for proper track management
enum class MusicState {
    Stopped,
    Playing,
    Paused,
    TemporaryOverride  //for invincibility/speed shoes
};

//sound Manager - handles all audio in the game
class SoundManager {
private:
//file paths for sounds (configured at startup)
    std::map<SoundEffect, std::string> soundPaths;
    std::map<MusicTrack, std::string> musicPaths;

//SFML audio objects
    sf::Music music;  //for background music
    std::map<SoundEffect, sf::SoundBuffer> soundBuffers;
    std::map<SoundEffect, std::vector<sf::Sound>> soundPools;  //multiple instances per effect
    static constexpr int SOUND_POOL_SIZE = 4;  //4 instances per effect for overlapping sounds

//current state
    MusicTrack currentTrack = MusicTrack::None;
    MusicTrack previousTrack = MusicTrack::None;  //for resuming after temp override
    MusicState musicState = MusicState::Stopped;
    float musicVolume = 0.3f;  //30% for background music (quiet)
    float soundVolume = 0.8f;  //80% for sound effects
    bool soundEnabled = true;
    bool musicEnabled = true;

    uint64_t tempTrackEndTime = 0;

    SoundManager() = default;
    
public:
    static SoundManager& Instance() {
        static SoundManager instance;
        return instance;
    }
    
//=== INITIALIZATION ===
    
//set the base path for all sound files
    void SetAssetPath(const std::string& path) {
        basePath = path;
        ConfigureDefaultPaths();
    }
    
//load all sounds (call after SetAssetPath)
    bool LoadAll() {
        bool success = true;
        
        for (int i = 0; i < static_cast<int>(SoundEffect::COUNT); ++i) {
            SoundEffect effect = static_cast<SoundEffect>(i);
            if (soundPaths.count(effect)) {
                if (!LoadSound(effect, soundPaths[effect])) {
                    std::cout << "[Sound] Failed to load: " << soundPaths[effect] << std::endl;
//don't fail completely - game can run without sounds
                }
            }
        }
        
        for (int i = 0; i < static_cast<int>(MusicTrack::COUNT); ++i) {
            MusicTrack track = static_cast<MusicTrack>(i);
            if (musicPaths.count(track)) {
                if (!LoadMusic(track, musicPaths[track])) {
                    std::cout << "[Sound] Failed to load music: " << musicPaths[track] << std::endl;
                }
            }
        }
        
        std::cout << "[Sound] Sound system initialized" << std::endl;
        return success;
    }
    
//=== SOUND EFFECTS ===
    
//play a sound effect (fire and forget)
    void PlaySound(SoundEffect effect) {
        if (!soundEnabled) return;

        if (soundPools.count(effect) == 0) {
            return;  //sound not loaded
        }

//find available sound instance from pool
        auto& pool = soundPools[effect];
        for (auto& sound : pool) {
            if (sound.getStatus() != sf::Sound::Playing) {
                sound.setVolume(soundVolume * 100.0f);
                sound.play();
                return;
            }
        }
//all busy, use first one (oldest)
        pool[0].setVolume(soundVolume * 100.0f);
        pool[0].play();
    }

//play sound at specific volume (0.0 - 1.0)
    void PlaySound(SoundEffect effect, float volume) {
        if (!soundEnabled) return;

        if (soundPools.count(effect) == 0) {
            return;  //sound not loaded
        }

//find available sound instance from pool
        auto& pool = soundPools[effect];
        for (auto& sound : pool) {
            if (sound.getStatus() != sf::Sound::Playing) {
                sound.setVolume(volume * 100.0f);
                sound.play();
                return;
            }
        }
//all busy, use first one (oldest)
        pool[0].setVolume(volume * 100.0f);
        pool[0].play();
    }
    
//=== MUSIC ===
    
//play a music track (loops by default)
    void PlayMusic(MusicTrack track, bool loop = true) {
        if (!musicEnabled) return;
        if (track == currentTrack && musicState == MusicState::Playing) return;

        if (musicPaths.count(track) == 0) {
            return;  //track not configured
        }

        music.stop();
        if (!music.openFromFile(musicPaths[track])) {
            std::cout << "[Music] Failed to open: " << musicPaths[track] << std::endl;
            return;
        }

        music.setLoop(loop);
        music.setVolume(musicVolume * 100.0f);
        music.play();

        previousTrack = currentTrack;
        currentTrack = track;
        musicState = MusicState::Playing;
    }
    
//play temporary override track (invincibility, speed shoes)
//will resume previous track when duration expires
    void PlayTemporaryMusic(MusicTrack track, uint64_t durationMs) {
        if (!musicEnabled) return;
        
        previousTrack = currentTrack;
        currentTrack = track;
        musicState = MusicState::TemporaryOverride;
        tempTrackEndTime = GetSystemTimeForSound() + durationMs;
        
        std::cout << "[Music] Temporary: " << GetMusicName(track) << " for " << durationMs << "ms" << std::endl;
    }
    
//stop current music
    void StopMusic() {
        music.stop();
        musicState = MusicState::Stopped;
        currentTrack = MusicTrack::None;
    }
    
//pause/resume music
    void PauseMusic() {
        if (musicState == MusicState::Playing) {
            music.pause();
            musicState = MusicState::Paused;
        }
    }

    void ResumeMusic() {
        if (musicState == MusicState::Paused) {
            music.play();
            musicState = MusicState::Playing;
        }
    }
    
    
    void Update() {
//check if temporary track should end
        if (musicState == MusicState::TemporaryOverride) {
            if (GetSystemTimeForSound() >= tempTrackEndTime) {
//resume previous track
                MusicTrack trackToResume = previousTrack;
                currentTrack = trackToResume;
                musicState = MusicState::Playing;

                if (trackToResume != MusicTrack::None && musicPaths.count(trackToResume)) {
                    music.stop();
                    if (music.openFromFile(musicPaths[trackToResume])) {
                        music.setLoop(true);
                        music.setVolume(musicVolume * 100.0f);
                        music.play();
                    }
                }
            }
        }
    }
    
//=== VOLUME CONTROLS ===
    
    void SetMusicVolume(float vol) { 
        musicVolume = std::max(0.0f, std::min(1.0f, vol)); 
    }
    void SetSoundVolume(float vol) { 
        soundVolume = std::max(0.0f, std::min(1.0f, vol)); 
    }
    float GetMusicVolume() const { return musicVolume; }
    float GetSoundVolume() const { return soundVolume; }
    
    void EnableSound(bool enable) { soundEnabled = enable; }
    void EnableMusic(bool enable) { musicEnabled = enable; }
    bool IsSoundEnabled() const { return soundEnabled; }
    bool IsMusicEnabled() const { return musicEnabled; }
    
//=== STATE QUERIES ===
    
    MusicTrack GetCurrentTrack() const { return currentTrack; }
    MusicState GetMusicState() const { return musicState; }
    
//=== GAME EVENT HOOKS ===
//call these from game logic to trigger appropriate sounds
    
    void OnJump() { PlaySound(SoundEffect::Jump); }
    void OnRingCollect() { PlaySound(SoundEffect::Ring); }
    void OnRingLoss() { PlaySound(SoundEffect::RingLoss); }
    void OnSpindashCharge() { PlaySound(SoundEffect::SpindashCharge); }
    void OnSpindashRelease() { PlaySound(SoundEffect::SpindashRelease); }
    void OnSpring() { PlaySound(SoundEffect::Spring); }
    void OnCheckpoint() { PlaySound(SoundEffect::Checkpoint); }
    void OnShieldCollect() { PlaySound(SoundEffect::Shield); }
    void OnInvincibilityCollect(uint64_t duration) { 
        PlaySound(SoundEffect::Invincibility); 
        PlayTemporaryMusic(MusicTrack::Invincibility, duration);
    }
    void OnSpeedShoesCollect(uint64_t duration) { 
        PlaySound(SoundEffect::SpeedShoes); 
        PlayTemporaryMusic(MusicTrack::SpeedShoes, duration);
    }
    void OnExtraLife() { PlaySound(SoundEffect::ExtraLife); }
    void OnHurt() { PlaySound(SoundEffect::Hurt); }
    void OnDeath() { 
        PlaySound(SoundEffect::Death); 
        StopMusic();  //music stops on death
    }
    void OnEnemyDestroy() { PlaySound(SoundEffect::EnemyDestroy); }
    void OnMonitorBreak() { PlaySound(SoundEffect::MonitorBreak); }
    void OnGameOver() { 
        PlaySound(SoundEffect::GameOver); 
        PlayMusic(MusicTrack::GameOver, false);  //don't loop
    }
    void OnActClear() { 
        PlaySound(SoundEffect::ActClear);
        PlayMusic(MusicTrack::ActClear, false);
    }
    
//start level with appropriate music
    void OnLevelStart(int zone) {
        switch (zone) {
            case 1: PlayMusic(MusicTrack::GreenHillZone); break;
            case 2: PlayMusic(MusicTrack::MarbleZone); break;
            case 3: PlayMusic(MusicTrack::SpringYardZone); break;
            case 4: PlayMusic(MusicTrack::LabyrinthZone); break;
            case 5: PlayMusic(MusicTrack::StarLightZone); break;
            case 6: PlayMusic(MusicTrack::ScrapBrainZone); break;
            case 7: PlayMusic(MusicTrack::FinalZone); break;
            default: PlayMusic(MusicTrack::GreenHillZone); break;
        }
    }
    
    void OnTitleScreen() { PlayMusic(MusicTrack::TitleScreen); }
    void OnBossStart() { PlayMusic(MusicTrack::BossBattle); }
    void OnDrowningWarning() { PlayMusic(MusicTrack::Drowning); }
    
private:
    std::string basePath = "assets/";
    
//configure default file paths for sounds
    void ConfigureDefaultPaths() {
//sound effects - using available music files
        soundPaths[SoundEffect::Jump] = basePath + "music/jump.mp3";
        soundPaths[SoundEffect::Ring] = basePath + "music/sonic-rings.mp3";
        soundPaths[SoundEffect::RingLoss] = basePath + "music/sonic-rings-drop.mp3";
        soundPaths[SoundEffect::Spring] = basePath + "music/sonic-spring.mp3";
        soundPaths[SoundEffect::Checkpoint] = basePath + "music/sonic-checkpoint.mp3";
        soundPaths[SoundEffect::Death] = basePath + "music/sonic-dead.mp3";
        soundPaths[SoundEffect::MonitorBreak] = basePath + "music/get-a-load-of-this.mp3";

//music tracks - using available music files
        musicPaths[MusicTrack::TitleScreen] = basePath + "music/sonic-theme-song.mp3";
        musicPaths[MusicTrack::GreenHillZone] = basePath + "music/sonic-theme-song.mp3";
        musicPaths[MusicTrack::ActClear] = basePath + "music/this-too-easy.mp3";
        musicPaths[MusicTrack::GameOver] = basePath + "music/game-over.mp3";
    }
    
//helper to load a sound effect
    bool LoadSound(SoundEffect effect, const std::string& path) {
        sf::SoundBuffer& buffer = soundBuffers[effect];
        if (!buffer.loadFromFile(path)) {
            return false;
        }
//create pool of sound instances for overlapping playback
        soundPools[effect].resize(SOUND_POOL_SIZE);
        for (int i = 0; i < SOUND_POOL_SIZE; ++i) {
            soundPools[effect][i].setBuffer(buffer);
        }
        return true;
    }
    
//helper to load a music track
    bool LoadMusic(MusicTrack track, const std::string& path) {
        (void)track;  //unused - music is streamed when played
        (void)path;   //unused - music is streamed when played
//music files are typically streamed, not pre-loaded
        return true;
    }
    
//get current time (milliseconds)
    uint64_t GetSystemTimeForSound() const {
        return engine::GetSystemTime();
    }
    
//debug helpers
    const char* GetSoundName(SoundEffect effect) const {
        switch (effect) {
            case SoundEffect::Jump: return "Jump";
            case SoundEffect::Ring: return "Ring";
            case SoundEffect::RingLoss: return "RingLoss";
            case SoundEffect::Spin: return "Spin";
            case SoundEffect::SpindashCharge: return "SpindashCharge";
            case SoundEffect::SpindashRelease: return "SpindashRelease";
            case SoundEffect::Spring: return "Spring";
            case SoundEffect::Checkpoint: return "Checkpoint";
            case SoundEffect::Shield: return "Shield";
            case SoundEffect::Invincibility: return "Invincibility";
            case SoundEffect::SpeedShoes: return "SpeedShoes";
            case SoundEffect::ExtraLife: return "ExtraLife";
            case SoundEffect::Hurt: return "Hurt";
            case SoundEffect::Death: return "Death";
            case SoundEffect::EnemyDestroy: return "EnemyDestroy";
            case SoundEffect::MonitorBreak: return "MonitorBreak";
            case SoundEffect::BossHit: return "BossHit";
            case SoundEffect::GameOver: return "GameOver";
            case SoundEffect::ActClear: return "ActClear";
            case SoundEffect::Splash: return "Splash";
            case SoundEffect::Spike: return "Spike";
            default: return "Unknown";
        }
    }
    
    const char* GetMusicName(MusicTrack track) const {
        switch (track) {
            case MusicTrack::None: return "None";
            case MusicTrack::TitleScreen: return "TitleScreen";
            case MusicTrack::GreenHillZone: return "GreenHillZone";
            case MusicTrack::MarbleZone: return "MarbleZone";
            case MusicTrack::SpringYardZone: return "SpringYardZone";
            case MusicTrack::LabyrinthZone: return "LabyrinthZone";
            case MusicTrack::StarLightZone: return "StarLightZone";
            case MusicTrack::ScrapBrainZone: return "ScrapBrainZone";
            case MusicTrack::FinalZone: return "FinalZone";
            case MusicTrack::Invincibility: return "Invincibility";
            case MusicTrack::SpeedShoes: return "SpeedShoes";
            case MusicTrack::BossBattle: return "BossBattle";
            case MusicTrack::ActClear: return "ActClear";
            case MusicTrack::GameOver: return "GameOver";
            case MusicTrack::Drowning: return "Drowning";
            case MusicTrack::Credits: return "Credits";
            default: return "Unknown";
        }
    }
};

}  //namespace app

#endif  //SOUND_MANAGER_HPP
