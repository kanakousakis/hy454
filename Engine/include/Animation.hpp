#ifndef ENGINE_ANIMATION_HPP
#define ENGINE_ANIMATION_HPP

#include "Types.hpp"
#include <string>
#include <vector>

namespace engine {

// Base animation class
class Animation {
protected:
    std::string id;
    
public:
    const std::string& GetId() const { return id; }
    void SetId(const std::string& _id) { id = _id; }
    
    virtual Animation* Clone() const = 0;
    
    Animation(const std::string& _id = "") : id(_id) {}
    virtual ~Animation() {}
};

// Moving animation - basic movement with delay
class MovingAnimation : public Animation {
protected:
    unsigned reps = 1;    // 0 = forever
    int dx = 0, dy = 0;
    unsigned delay = 0;
    
public:
    using Me = MovingAnimation;
    
    int GetDx() const { return dx; }
    Me& SetDx(int v) { dx = v; return *this; }
    int GetDy() const { return dy; }
    Me& SetDy(int v) { dy = v; return *this; }
    unsigned GetDelay() const { return delay; }
    Me& SetDelay(unsigned v) { delay = v; return *this; }
    unsigned GetReps() const { return reps; }
    Me& SetReps(unsigned n) { reps = n; return *this; }
    bool IsForever() const { return reps == 0; }
    Me& SetForever() { reps = 0; return *this; }
    
    Animation* Clone() const override {
        return new MovingAnimation(id, reps, dx, dy, delay);
    }
    
    MovingAnimation(const std::string& _id = "", unsigned _reps = 1,
                    int _dx = 0, int _dy = 0, unsigned _delay = 0)
        : Animation(_id), reps(_reps), dx(_dx), dy(_dy), delay(_delay) {}
};

// Frame range animation - cycles through frame range
class FrameRangeAnimation : public MovingAnimation {
protected:
    unsigned start = 0;
    unsigned end = 0;
    
public:
    using Me = FrameRangeAnimation;
    
    unsigned GetStartFrame() const { return start; }
    Me& SetStartFrame(unsigned v) { start = v; return *this; }
    unsigned GetEndFrame() const { return end; }
    Me& SetEndFrame(unsigned v) { end = v; return *this; }
    
    Animation* Clone() const override {
        return new FrameRangeAnimation(id, start, end, GetReps(), GetDx(), GetDy(), GetDelay());
    }
    
    FrameRangeAnimation(const std::string& _id = "", unsigned s = 0, unsigned e = 0,
                        unsigned r = 1, int dx = 0, int dy = 0, unsigned d = 0)
        : MovingAnimation(_id, r, dx, dy, d), start(s), end(e) {}
};

// Frame list animation - explicit frame sequence
class FrameListAnimation : public MovingAnimation {
public:
    using Frames = std::vector<unsigned>;
    
protected:
    Frames frames;
    
public:
    const Frames& GetFrames() const { return frames; }
    void SetFrames(const Frames& f) { frames = f; }
    
    Animation* Clone() const override {
        return new FrameListAnimation(id, frames, GetReps(), GetDx(), GetDy(), GetDelay());
    }
    
    FrameListAnimation(const std::string& _id = "", const Frames& _frames = {},
                       unsigned r = 1, int dx = 0, int dy = 0, unsigned d = 0)
        : MovingAnimation(_id, r, dx, dy, d), frames(_frames) {}
};

// Path entry for moving path animation
struct PathEntry {
    int dx = 0, dy = 0;
    unsigned frame = 0;
    unsigned delay = 0;
    
    PathEntry() = default;
    PathEntry(int _dx, int _dy, unsigned _frame, unsigned _delay)
        : dx(_dx), dy(_dy), frame(_frame), delay(_delay) {}
};

// Moving path animation - follows a path
class MovingPathAnimation : public Animation {
public:
    using Path = std::vector<PathEntry>;
    
private:
    Path path;
    
public:
    const Path& GetPath() const { return path; }
    void SetPath(const Path& p) { path = p; }
    
    Animation* Clone() const override {
        return new MovingPathAnimation(id, path);
    }
    
    MovingPathAnimation(const std::string& _id = "", const Path& _path = {})
        : Animation(_id), path(_path) {}
};

// Flash animation - for invincibility effect
class FlashAnimation : public Animation {
private:
    unsigned repetitions = 0;
    unsigned hideDelay = 0;
    unsigned showDelay = 0;
    
public:
    using Me = FlashAnimation;
    
    Me& SetRepetitions(unsigned n) { repetitions = n; return *this; }
    unsigned GetRepetitions() const { return repetitions; }
    Me& SetHideDelay(unsigned d) { hideDelay = d; return *this; }
    unsigned GetHideDelay() const { return hideDelay; }
    Me& SetShowDelay(unsigned d) { showDelay = d; return *this; }
    unsigned GetShowDelay() const { return showDelay; }
    
    Animation* Clone() const override {
        return new FlashAnimation(id, repetitions, showDelay, hideDelay);
    }
    
    FlashAnimation(const std::string& _id = "", unsigned n = 0,
                   unsigned show = 0, unsigned hide = 0)
        : Animation(_id), repetitions(n), hideDelay(hide), showDelay(show) {}
};

// Tick animation - for timers
class TickAnimation : public Animation {
protected:
    unsigned delay = 0;
    unsigned reps = 1;
    bool isDiscrete = true;
    
public:
    using Me = TickAnimation;
    
    unsigned GetDelay() const { return delay; }
    Me& SetDelay(unsigned v) { delay = v; return *this; }
    unsigned GetReps() const { return reps; }
    Me& SetReps(unsigned n) { reps = n; return *this; }
    bool IsForever() const { return reps == 0; }
    Me& SetForever() { reps = 0; return *this; }
    bool IsDiscrete() const { return isDiscrete; }
    Me& SetDiscrete(bool v) { isDiscrete = v; return *this; }
    
    Animation* Clone() const override {
        return new TickAnimation(id, delay, reps, isDiscrete);
    }
    
    TickAnimation(const std::string& _id = "", unsigned d = 0, 
                  unsigned r = 1, bool discrete = true)
        : Animation(_id), delay(d), reps(r), isDiscrete(discrete) {}
};

} // namespace engine

#endif // ENGINE_ANIMATION_HPP
