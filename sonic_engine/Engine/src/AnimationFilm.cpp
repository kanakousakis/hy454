#include "AnimationFilm.hpp"

namespace engine {

AnimationFilmHolder* AnimationFilmHolder::instance = nullptr;

AnimationFilm::AnimationFilm(BitmapPtr bmp, const std::vector<Rect>& rects, const std::string& _id)
    : boxes(rects), bitmap(bmp), id(_id) {}

void AnimationFilm::DisplayFrame(const Point& at, byte frameNo) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);
        
        // Get the texture and create a sprite
        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));
        sprite.setPosition(static_cast<float>(at.x), static_cast<float>(at.y));
        
        // Draw to back buffer
        GetGraphics().DrawSprite(sprite);
    }
}

void AnimationFilm::DisplayFrameFlipped(const Point& at, byte frameNo) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);
        
        // Get the texture and create a sprite
        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));
        
        // Flip horizontally by scaling negative and offsetting
        sprite.setScale(-1.f, 1.f);
        sprite.setPosition(static_cast<float>(at.x + box.w), static_cast<float>(at.y));
        
        // Draw to back buffer
        GetGraphics().DrawSprite(sprite);
    }
}

AnimationFilmHolder& AnimationFilmHolder::Get() {
    if (!instance) {
        instance = new AnimationFilmHolder();
    }
    return *instance;
}

void AnimationFilmHolder::Load(const std::string& jsonPath) {
    // TODO: Implement JSON loading
    // For now, films should be added manually
    (void)jsonPath;
}

void AnimationFilmHolder::CleanUp() {
    for (auto& pair : films) {
        delete pair.second;
    }
    films.clear();
}

} // namespace engine
