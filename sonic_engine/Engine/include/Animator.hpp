#ifndef ENGINE_ANIMATOR_HPP
#define ENGINE_ANIMATOR_HPP

#include "Types.hpp"
#include "Animation.hpp"
#include <functional>
#include <set>

namespace engine {

class Animator {
public:
    using OnFinish = std::function<void(Animator*)>;
    using OnStart = std::function<void(Animator*)>;
    using OnAction = std::function<void(Animator*, const Animation&)>;
    
protected:
    timestamp_t lastTime = 0;
    AnimatorState state = AnimatorState::Finished;
    OnFinish onFinish;
    OnStart onStart;
    OnAction onAction;
    
    void NotifyStopped();
    void NotifyStarted();
    void NotifyAction(const Animation& anim);
    void Finish(bool isForced = false);
    
public:
    void Stop() { Finish(true); }
    bool HasFinished() const { return state != AnimatorState::Running; }
    bool IsRunning() const { return state == AnimatorState::Running; }
    AnimatorState GetState() const { return state; }
    
    virtual void TimeShift(timestamp_t offset) { lastTime += offset; }
    virtual void Progress(timestamp_t currTime) = 0;
    
    void SetOnFinish(const OnFinish& f) { onFinish = f; }
    void SetOnStart(const OnStart& f) { onStart = f; }
    void SetOnAction(const OnAction& f) { onAction = f; }
    
    Animator();
    Animator(const Animator&) = delete;
    virtual ~Animator();
};

// Moving animator
class MovingAnimator : public Animator {
protected:
    MovingAnimation* anim = nullptr;
    unsigned currRep = 0;
    
public:
    void Progress(timestamp_t currTime) override;
    
    const MovingAnimation& GetAnim() const { return *anim; }
    
    void Start(MovingAnimation* a, timestamp_t t);
    
    MovingAnimator() = default;
};

// Frame range animator
class FrameRangeAnimator : public Animator {
protected:
    FrameRangeAnimation* anim = nullptr;
    unsigned currFrame = 0;
    unsigned currRep = 0;
    
public:
    void Progress(timestamp_t currTime) override;
    
    unsigned GetCurrFrame() const { return currFrame; }
    unsigned GetCurrRep() const { return currRep; }
    const FrameRangeAnimation& GetAnim() const { return *anim; }
    
    void Start(FrameRangeAnimation* a, timestamp_t t);
    
    FrameRangeAnimator() = default;
};

// Frame list animator
class FrameListAnimator : public Animator {
protected:
    FrameListAnimation* anim = nullptr;
    unsigned currIndex = 0;
    unsigned currRep = 0;
    
public:
    void Progress(timestamp_t currTime) override;
    
    unsigned GetCurrIndex() const { return currIndex; }
    unsigned GetCurrFrame() const;
    const FrameListAnimation& GetAnim() const { return *anim; }
    
    void Start(FrameListAnimation* a, timestamp_t t);
    
    FrameListAnimator() = default;
};

// Flash animator for invincibility
class FlashAnimator : public Animator {
protected:
    FlashAnimation* anim = nullptr;
    unsigned currRep = 0;
    bool isVisible = true;
    
public:
    void Progress(timestamp_t currTime) override;
    
    bool IsVisible() const { return isVisible; }
    const FlashAnimation& GetAnim() const { return *anim; }
    
    void Start(FlashAnimation* a, timestamp_t t);
    
    FlashAnimator() = default;
};

// Tick animator for timers
class TickAnimator : public Animator {
protected:
    TickAnimation* anim = nullptr;
    unsigned currRep = 0;
    timestamp_t elapsedTime = 0;
    
public:
    void Progress(timestamp_t currTime) override;
    
    timestamp_t GetElapsedTime() const { return elapsedTime; }
    const TickAnimation& GetAnim() const { return *anim; }
    
    void Start(TickAnimation* a, timestamp_t t);
    
    TickAnimator() = default;
};

// Animator manager singleton
class AnimatorManager {
private:
    std::set<Animator*> running;
    std::set<Animator*> suspended;
    static AnimatorManager* instance;
    
    AnimatorManager() = default;
    
public:
    static AnimatorManager& Instance();
    
    void Register(Animator* a);
    void Cancel(Animator* a);
    void MarkAsRunning(Animator* a);
    void MarkAsSuspended(Animator* a);
    
    void Progress(timestamp_t currTime);
    void TimeShift(timestamp_t dt);
    
    size_t GetRunningCount() const { return running.size(); }
    size_t GetSuspendedCount() const { return suspended.size(); }
};

} // namespace engine

#endif // ENGINE_ANIMATOR_HPP
