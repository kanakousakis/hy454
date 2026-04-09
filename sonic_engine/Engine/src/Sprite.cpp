#include "Sprite.hpp"
#include "Bitmap.hpp"
#include "GridLayer.hpp"
#include <cassert>
#include <cmath>

namespace engine {

//============================================================================
//static instance pointers
//============================================================================
SpriteManager* SpriteManager::instance = nullptr;
CollisionChecker* CollisionChecker::instance = nullptr;

//============================================================================
//boundingBox Implementation
//============================================================================
bool BoundingBox::Intersects(const BoundingArea& area) const {
//try to cast to BoundingBox
    if (auto* other = dynamic_cast<const BoundingBox*>(&area)) {
        return BoxesOverlap(box, other->box);
    }
//try to cast to BoundingCircle
    if (auto* circle = dynamic_cast<const BoundingCircle*>(&area)) {
//box-circle intersection
        const Point& c = circle->GetCenter();
        int r = circle->GetRadius();
        
//find closest point on box to circle center
        int closestX = std::max(box.x, std::min(c.x, box.x + box.w));
        int closestY = std::max(box.y, std::min(c.y, box.y + box.h));
        
        int dx = c.x - closestX;
        int dy = c.y - closestY;
        
        return (dx * dx + dy * dy) <= (r * r);
    }
    return false;
}

//============================================================================
//boundingCircle Implementation
//============================================================================
bool BoundingCircle::Intersects(const BoundingArea& area) const {
//try to cast to BoundingCircle
    if (auto* other = dynamic_cast<const BoundingCircle*>(&area)) {
        int dx = center.x - other->center.x;
        int dy = center.y - other->center.y;
        int sumR = radius + other->radius;
        return (dx * dx + dy * dy) <= (sumR * sumR);
    }
//try to cast to BoundingBox (delegate)
    if (auto* box = dynamic_cast<const BoundingBox*>(&area)) {
        return box->Intersects(*this);
    }
    return false;
}

//============================================================================
//sprite Implementation
//============================================================================
Sprite::Sprite(int _x, int _y, AnimationFilm* film, const std::string& _typeId)
    : x(_x), y(_y), currFilm(film), typeId(_typeId) {
    if (currFilm && currFilm->GetTotalFrames() > 0) {
        frameNo = 0;
        frameBox = currFilm->GetFrameBox(0);
    }
}

Sprite::~Sprite() {
    delete boundingArea;
}

bool Sprite::CollisionCheck(const Sprite* s) const {
    if (!s || !isVisible || !s->isVisible)
        return false;
    
//quick bounding box test first
    if (NoBoxOverlap(GetBox(), s->GetBox()))
        return false;
    
//if we have bounding areas, use those for precise collision
    if (boundingArea && s->boundingArea)
        return boundingArea->Intersects(*s->boundingArea);
    
//bounding boxes overlap = collision
    return true;
}

void Sprite::Display(const Rect& dpyArea, const Clipper& clipper) const {
    if (!isVisible || !currFilm)
        return;
    
    Point dpyPos;
    Rect clippedBox;
    
    if (clipper.Clip(GetBox(), dpyArea, &dpyPos, &clippedBox)) {
        if (clippedBox.w > 0 && clippedBox.h > 0) {
//source rectangle in the film's bitmap
            const Rect& srcFrame = currFilm->GetFrameBox(frameNo);
            Rect srcRect = {
                srcFrame.x + clippedBox.x,
                srcFrame.y + clippedBox.y,
                clippedBox.w,
                clippedBox.h
            };
            
//draw the clipped portion
            BitmapPtr bmp = currFilm->GetBitmap();
            if (bmp) {
                GetGraphics().DrawTexture(bmp->GetTexture(), srcRect, dpyPos);
            }
        }
    }
}

//============================================================================
//spriteManager Implementation
//============================================================================
SpriteManager& SpriteManager::Instance() {
    if (!instance) {
        instance = new SpriteManager();
    }
    return *instance;
}

void SpriteManager::Add(Sprite* s) {
    if (!s) return;
    
//insert in z-order (ascending)
    auto it = std::find_if(dpyList.begin(), dpyList.end(),
        [s](Sprite* other) { return other->GetZorder() > s->GetZorder(); }
    );
    dpyList.insert(it, s);
    
//add to type list
    if (!s->GetTypeId().empty()) {
        types[s->GetTypeId()].push_back(s);
    }
}

void SpriteManager::Remove(Sprite* s) {
    if (!s) return;
    
    dpyList.remove(s);
    
    if (!s->GetTypeId().empty()) {
        auto it = types.find(s->GetTypeId());
        if (it != types.end()) {
            it->second.remove(s);
        }
    }
}

const SpriteManager::SpriteList& SpriteManager::GetTypeList(const std::string& typeId) const {
    static SpriteList empty;
    auto it = types.find(typeId);
    return (it != types.end()) ? it->second : empty;
}

void SpriteManager::Clear() {
    dpyList.clear();
    types.clear();
}

//============================================================================
//collisionChecker Implementation
//============================================================================
CollisionChecker& CollisionChecker::Instance() {
    if (!instance) {
        instance = new CollisionChecker();
    }
    return *instance;
}

std::list<CollisionChecker::Entry>::iterator 
CollisionChecker::Find(Sprite* s1, Sprite* s2) {
    return std::find_if(entries.begin(), entries.end(),
        [s1, s2](const Entry& e) {
            return (e.s1 == s1 && e.s2 == s2) ||
                   (e.s1 == s2 && e.s2 == s1);
        }
    );
}

void CollisionChecker::Register(Sprite* s1, Sprite* s2, const Action& f) {
    if (Find(s1, s2) == entries.end()) {
        entries.push_back({s1, s2, f});
    }
}

void CollisionChecker::Cancel(Sprite* s1, Sprite* s2) {
    auto it = Find(s1, s2);
    if (it != entries.end()) {
        entries.erase(it);
    }
}

void CollisionChecker::CancelAll(Sprite* s) {
    entries.remove_if([s](const Entry& e) {
        return e.s1 == s || e.s2 == s;
    });
}

void CollisionChecker::Check() {
//make a copy to allow modifications during iteration
    auto entriesCopy = entries;
    
    for (const auto& e : entriesCopy) {
        if (e.s1->CollisionCheck(e.s2)) {
            if (e.action) {
                e.action(e.s1, e.s2);
            }
        }
    }
}

//============================================================================
//helper Functions
//============================================================================
Sprite::Mover MakeGridLayerMover(GridLayer* grid) {
    return [grid](const Rect& r, int* dx, int* dy) {
        grid->FilterGridMotion(r, dx, dy);
    };
}

void PrepareSpriteGravityHandler(GridLayer* grid, Sprite* sprite) {
    sprite->GetGravityHandler().SetOnSolidGround(
        [grid](const Rect& r) {
            return grid->IsOnSolidGround(r);
        }
    );
    sprite->GetGravityHandler().SetGravityAddicted(true);
}

}  //namespace engine
