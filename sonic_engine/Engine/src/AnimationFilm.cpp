#include "AnimationFilm.hpp"

namespace engine {

AnimationFilmHolder* AnimationFilmHolder::instance = nullptr;

AnimationFilm::AnimationFilm(BitmapPtr bmp, const std::vector<Rect>& rects, const std::string& _id)
    : boxes(rects), bitmap(bmp), id(_id) {}

void AnimationFilm::DisplayFrame(const Point& at, byte frameNo) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);

//get the texture and create a sprite
        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));
        sprite.setPosition(static_cast<float>(at.x), static_cast<float>(at.y));

//draw to back buffer
        GetGraphics().DrawSprite(sprite);
    }
}

void AnimationFilm::DisplayFrameFlipped(const Point& at, byte frameNo) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);

//get the texture and create a sprite
        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));

//flip horizontally by scaling negative and offsetting
        sprite.setScale(-1.f, 1.f);
        sprite.setPosition(static_cast<float>(at.x + box.w), static_cast<float>(at.y));

//draw to back buffer
        GetGraphics().DrawSprite(sprite);
    }
}

void AnimationFilm::DisplayFrameFlippedVertical(const Point& at, byte frameNo) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);

//get the texture and create a sprite
        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));

//flip vertically by scaling negative Y and offsetting
        sprite.setScale(1.f, -1.f);
        sprite.setPosition(static_cast<float>(at.x), static_cast<float>(at.y + box.h));

//draw to back buffer
        GetGraphics().DrawSprite(sprite);
    }
}

void AnimationFilm::DisplayFrameScaled(const Point& at, byte frameNo, float scale) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);

        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));
        sprite.setScale(scale, scale);
        sprite.setPosition(static_cast<float>(at.x), static_cast<float>(at.y));

        GetGraphics().DrawSprite(sprite);
    }
}

void AnimationFilm::DisplayFrameScaledFlipped(const Point& at, byte frameNo, float scale) const {
    if (bitmap && frameNo < boxes.size()) {
        const Rect& box = GetFrameBox(frameNo);

        sf::Sprite sprite(bitmap->GetTexture());
        sprite.setTextureRect(sf::IntRect(box.x, box.y, box.w, box.h));

//flip horizontally (negative X scale) and apply overall scale
        sprite.setScale(-scale, scale);
        sprite.setPosition(static_cast<float>(at.x + static_cast<int>(box.w * scale)), static_cast<float>(at.y));

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
//for now, films should be added manually
    (void)jsonPath;
}

void AnimationFilmHolder::CleanUp() {
    for (auto& pair : films) {
        delete pair.second;
    }
    films.clear();
}

}  //namespace engine
