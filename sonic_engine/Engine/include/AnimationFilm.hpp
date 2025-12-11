#ifndef ENGINE_ANIMATION_FILM_HPP
#define ENGINE_ANIMATION_FILM_HPP

#include "Types.hpp"
#include "Bitmap.hpp"
#include <vector>
#include <string>
#include <map>

namespace engine {

class AnimationFilm {
private:
    std::vector<Rect> boxes;  // Frame bounding boxes
    BitmapPtr bitmap;
    std::string id;
    
public:
    byte GetTotalFrames() const { return static_cast<byte>(boxes.size()); }
    BitmapPtr GetBitmap() const { return bitmap; }
    const std::string& GetId() const { return id; }
    
    const Rect& GetFrameBox(byte frameNo) const {
        return boxes[frameNo % boxes.size()];
    }
    
    void DisplayFrame(const Point& at, byte frameNo) const;
    void DisplayFrameFlipped(const Point& at, byte frameNo) const;
    
    void SetBitmap(BitmapPtr b) { bitmap = b; }
    void Append(const Rect& r) { boxes.push_back(r); }
    void Clear() { boxes.clear(); }
    
    AnimationFilm(const std::string& _id = "") : id(_id) {}
    AnimationFilm(BitmapPtr bmp, const std::vector<Rect>& rects, const std::string& _id);
};

// Singleton holder for all animation films
class AnimationFilmHolder {
private:
    std::map<std::string, AnimationFilm*> films;
    static AnimationFilmHolder* instance;
    
    AnimationFilmHolder() = default;
    
public:
    static AnimationFilmHolder& Get();
    static AnimationFilmHolder& Instance() { return Get(); }  // Alias
    
    void Load(const std::string& jsonPath);  // Load from config
    
    void AddFilm(const std::string& id, AnimationFilm* film) {
        films[id] = film;
    }
    
    // Convenience: create and add film in one call
    AnimationFilm* AddFilm(const std::string& id, BitmapPtr bitmap, const std::vector<Rect>& frames) {
        auto* film = new AnimationFilm(bitmap, frames, id);
        films[id] = film;
        return film;
    }
    
    AnimationFilm* GetFilm(const std::string& id) const {
        auto it = films.find(id);
        return (it != films.end()) ? it->second : nullptr;
    }
    
    bool HasFilm(const std::string& id) const {
        return films.find(id) != films.end();
    }
    
    void CleanUp();
    
    ~AnimationFilmHolder() { CleanUp(); }
};

} // namespace engine

#endif // ENGINE_ANIMATION_FILM_HPP
