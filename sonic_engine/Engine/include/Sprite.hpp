#ifndef ENGINE_SPRITE_HPP
#define ENGINE_SPRITE_HPP

#include "Types.hpp"
#include "AnimationFilm.hpp"
#include <functional>
#include <list>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>

namespace engine {

//forward declarations
class Sprite;
class GridLayer;

//============================================================================
//utility: clip_rect function (from lectures)
//============================================================================
template <typename T>
bool clip_rect(T x, T y, T w, T h,
               T wx, T wy, T ww, T wh,
               T* cx, T* cy, T* cw, T* ch) {
    *cw = T(std::min(wx + ww, x + w)) - (*cx = T(std::max(wx, x)));
    *ch = T(std::min(wy + wh, y + h)) - (*cy = T(std::max(wy, y)));
    return *cw > 0 && *ch > 0;
}

inline bool clip_rect(const Rect& r, const Rect& area, Rect* result) {
    return clip_rect(
        r.x, r.y, r.w, r.h,
        area.x, area.y, area.w, area.h,
        &result->x, &result->y, &result->w, &result->h
    );
}

//fast box overlap test (returns true if boxes DO NOT overlap)
inline bool NoBoxOverlap(const Rect& b1, const Rect& b2) {
    return b2.x + b2.w < b1.x ||  //b2 left of b1
           b1.x + b1.w < b2.x ||  //b1 left of b2
           b2.y + b2.h < b1.y ||  //b2 above b1
           b1.y + b1.h < b2.y;  //b1 above b2
}

inline bool BoxesOverlap(const Rect& b1, const Rect& b2) {
    return !NoBoxOverlap(b1, b2);
}

//============================================================================
//motionQuantizer (from lectures Section 10.2)
//breaks large movements into grid-element-sized steps
//============================================================================
class MotionQuantizer {
public:
    using Mover = std::function<void(const Rect& r, int* dx, int* dy)>;
    
private:
    int   horizMax = 0;
    int   vertMax = 0;
    Mover mover;
    bool  used = false;
    
public:
    MotionQuantizer& SetUsed(bool val) { used = val; return *this; }
    
    MotionQuantizer& SetRange(int h, int v) {
        horizMax = h;
        vertMax = v;
        used = true;
        return *this;
    }
    
    MotionQuantizer& SetMover(const Mover& f) { mover = f; return *this; }
    
    void Move(const Rect& r, int* dx, int* dy) {
        if (!used) {
            if (mover) mover(r, dx, dy);
        } else {
//break motion into smaller steps
            Rect currRect = r;
            int totalDx = 0, totalDy = 0;
            
            while (*dx != 0 || *dy != 0) {
                int sign_x = Sign(*dx);
                int sign_y = Sign(*dy);
                int dxStep = sign_x * std::min(horizMax, std::abs(*dx));
                int dyStep = sign_y * std::min(vertMax, std::abs(*dy));
                
                if (mover) mover(currRect, &dxStep, &dyStep);
                
                if (dxStep == 0) *dx = 0;
                else {
                    *dx -= dxStep;
                    totalDx += dxStep;
                    currRect.x += dxStep;
                }
                
                if (dyStep == 0) *dy = 0;
                else {
                    *dy -= dyStep;
                    totalDy += dyStep;
                    currRect.y += dyStep;
                }
            }
            
            *dx = totalDx;
            *dy = totalDy;
        }
    }
    
    MotionQuantizer() = default;
};

//============================================================================
//gravityHandler (from lectures Section 12.1)
//manages falling state based on ground detection
//============================================================================
class GravityHandler {
public:
    using OnSolidGroundPred = std::function<bool(const Rect&)>;
    using OnStartFalling    = std::function<void(void)>;
    using OnStopFalling     = std::function<void(void)>;
    
private:
    bool              gravityAddicted = false;
    bool              isFalling = false;
    OnSolidGroundPred onSolidGround;
    OnStartFalling    onStartFalling;
    OnStopFalling     onStopFalling;
    
public:
    void SetOnStartFalling(const OnStartFalling& f) { onStartFalling = f; }
    void SetOnStopFalling(const OnStopFalling& f) { onStopFalling = f; }
    void SetOnSolidGround(const OnSolidGroundPred& f) { onSolidGround = f; }
    
    void SetGravityAddicted(bool v) { gravityAddicted = v; }
    bool IsGravityAddicted() const { return gravityAddicted; }
    bool IsFalling() const { return isFalling; }
    
    void Reset() { isFalling = false; }
    
    void Check(const Rect& r) {
        if (gravityAddicted && onSolidGround) {
            if (onSolidGround(r)) {
                if (isFalling) {
                    isFalling = false;
                    if (onStopFalling) onStopFalling();
                }
            } else {
                if (!isFalling) {
                    isFalling = true;
                    if (onStartFalling) onStartFalling();
                }
            }
        }
    }
    
    GravityHandler() = default;
};

//============================================================================
//clipper (from lectures Section 10.3)
//clips sprites to view window for rendering
//============================================================================
class Clipper {
public:
    using ViewGetter = std::function<const Rect&(void)>;
    
private:
    ViewGetter viewGetter;
    
public:
    Clipper& SetViewGetter(const ViewGetter& f) {
        viewGetter = f;
        return *this;
    }
    
    bool Clip(const Rect& r, const Rect& dpyArea,
              Point* dpyPos, Rect* clippedBox) const {
        
        if (!viewGetter) {
//no view set, render at absolute position
            *dpyPos = {r.x, r.y};
            *clippedBox = {0, 0, r.w, r.h};
            return true;
        }
        
        const Rect& view = viewGetter();
        Rect visibleArea;
        
        if (!clip_rect(r, view, &visibleArea)) {
            clippedBox->w = clippedBox->h = 0;
            return false;
        }
        
//offset within sprite frame
        clippedBox->x = visibleArea.x - r.x;
        clippedBox->y = visibleArea.y - r.y;
        clippedBox->w = visibleArea.w;
        clippedBox->h = visibleArea.h;
        
//screen position
        dpyPos->x = dpyArea.x + (visibleArea.x - view.x);
        dpyPos->y = dpyArea.y + (visibleArea.y - view.y);
        
        return true;
    }
    
    Clipper() = default;
};

//============================================================================
//boundingArea (from lectures Section 11.2)
//abstract base for collision shapes
//============================================================================
class BoundingArea {
public:
    virtual bool Intersects(const BoundingArea& area) const = 0;
    virtual BoundingArea* Clone() const = 0;
    virtual ~BoundingArea() {}
};

class BoundingBox : public BoundingArea {
    Rect box;
public:
    const Rect& GetBox() const { return box; }
    void SetBox(const Rect& r) { box = r; }
    
    bool Intersects(const BoundingArea& area) const override;
    BoundingArea* Clone() const override { return new BoundingBox(box); }
    
    BoundingBox() = default;
    BoundingBox(const Rect& r) : box(r) {}
};

class BoundingCircle : public BoundingArea {
    Point center;
    int   radius = 0;
public:
    const Point& GetCenter() const { return center; }
    int GetRadius() const { return radius; }
    void SetCenter(const Point& c) { center = c; }
    void SetRadius(int r) { radius = r; }
    
    bool Intersects(const BoundingArea& area) const override;
    BoundingArea* Clone() const override { 
        return new BoundingCircle(center, radius); 
    }
    
    BoundingCircle() = default;
    BoundingCircle(const Point& c, int r) : center(c), radius(r) {}
};

//============================================================================
//sprite (from lectures Section 10.1)
//main game object with animation, movement, and collision
//============================================================================
class Sprite {
public:
    using Mover = std::function<void(const Rect&, int* dx, int* dy)>;
    
protected:
    byte            frameNo = 0;
    Rect            frameBox = {0, 0, 0, 0};
    int             x = 0, y = 0;
    bool            isVisible = true;
    AnimationFilm*  currFilm = nullptr;
    BoundingArea*   boundingArea = nullptr;
    unsigned        zorder = 0;
    std::string     typeId;
    std::string     stateId;
    Mover           mover;
    MotionQuantizer quantizer;
    bool            directMotion = false;
    GravityHandler  gravity;
    
public:
//position
    int GetX() const { return x; }
    int GetY() const { return y; }
    void SetPos(int _x, int _y) { x = _x; y = _y; }
    
    Rect GetBox() const { 
        return { x, y, frameBox.w, frameBox.h }; 
    }
    
//movement with grid collision
    void SetMover(const Mover& f) { 
        mover = f;
        quantizer.SetMover(f); 
    }
    
    Sprite& Move(int dx, int dy) {
        if (directMotion) {
            x += dx;
            y += dy;
        } else {
            quantizer.Move(GetBox(), &dx, &dy);
            if (dx || dy) {
                x += dx;
                y += dy;
            }
            gravity.Check(GetBox());
        }
        return *this;
    }
    
//z-order for rendering
    void SetZorder(unsigned z) { zorder = z; }
    unsigned GetZorder() const { return zorder; }
    
//animation frame
    void SetFrame(byte i) {
        if (currFilm && i < currFilm->GetTotalFrames()) {
            if (i != frameNo) {
                frameNo = i;
                frameBox = currFilm->GetFrameBox(frameNo);
            }
        }
    }
    byte GetFrame() const { return frameNo; }
    
//film
    void SetFilm(AnimationFilm* film) {
        currFilm = film;
        if (film) {
            frameNo = 0;
            frameBox = film->GetFrameBox(0);
        }
    }
    AnimationFilm* GetFilm() const { return currFilm; }
    
//visibility
    void SetVisibility(bool v) { isVisible = v; }
    bool IsVisible() const { return isVisible; }
    
//type and state IDs
    const std::string& GetTypeId() const { return typeId; }
    void SetTypeId(const std::string& t) { typeId = t; }
    const std::string& GetStateId() const { return stateId; }
    void SetStateId(const std::string& s) { stateId = s; }
    
//direct motion (bypasses grid collision)
    Sprite& SetHasDirectMotion(bool v) { directMotion = v; return *this; }
    bool GetHasDirectMotion() const { return directMotion; }
    
//gravity
    GravityHandler& GetGravityHandler() { return gravity; }
    const GravityHandler& GetGravityHandler() const { return gravity; }
    
//motion quantizer access
    MotionQuantizer& GetQuantizer() { return quantizer; }
    
//bounding area for collision
    void SetBoundingArea(BoundingArea* area) { 
        delete boundingArea;
        boundingArea = area; 
    }
    BoundingArea* GetBoundingArea() { return boundingArea; }
    const BoundingArea* GetBoundingArea() const { return boundingArea; }
    
//collision check
    bool CollisionCheck(const Sprite* s) const;
    
//display (clips to view and renders)
    void Display(const Rect& dpyArea, const Clipper& clipper) const;
    
//constructors
    Sprite() = default;
    Sprite(int _x, int _y, AnimationFilm* film, const std::string& _typeId = "");
    ~Sprite();
};

//============================================================================
//spriteManager (from lectures Section 10.5)
//singleton managing all sprites, sorted by z-order
//============================================================================
class SpriteManager {
public:
    using SpriteList = std::list<Sprite*>;
    using TypeLists  = std::map<std::string, SpriteList>;
    
private:
    SpriteList dpyList;  //sorted by ascending z-order
    TypeLists  types;  //sprites grouped by typeId
    static SpriteManager* instance;
    
    SpriteManager() = default;
    
public:
    static SpriteManager& Instance();
    
    void Add(Sprite* s);
    void Remove(Sprite* s);
    
    const SpriteList& GetDisplayList() const { return dpyList; }
    SpriteList& GetDisplayList() { return dpyList; }
    
    const SpriteList& GetTypeList(const std::string& typeId) const;
    
    void Clear();
    
    ~SpriteManager() { Clear(); }
};

//============================================================================
//collisionChecker (from lectures Section 11.3)
//singleton managing sprite-to-sprite collision pairs
//============================================================================
class CollisionChecker {
public:
    using Action = std::function<void(Sprite* s1, Sprite* s2)>;
    
private:
    struct Entry {
        Sprite* s1;
        Sprite* s2;
        Action  action;
    };
    
    std::list<Entry> entries;
    static CollisionChecker* instance;
    
    CollisionChecker() = default;
    
    std::list<Entry>::iterator Find(Sprite* s1, Sprite* s2);
    
public:
    static CollisionChecker& Instance();
    
    void Register(Sprite* s1, Sprite* s2, const Action& f);
    void Cancel(Sprite* s1, Sprite* s2);
    void CancelAll(Sprite* s);  //remove all entries involving this sprite
    
    void Check();
    
    void Clear() { entries.clear(); }
};

//============================================================================
//helper: Link sprite to grid layer for collision
//============================================================================
Sprite::Mover MakeGridLayerMover(GridLayer* grid);

void PrepareSpriteGravityHandler(GridLayer* grid, Sprite* sprite);

}  //namespace engine

#endif  //ENGINE_SPRITE_HPP
