#include "Animator.hpp"
#include <algorithm>

namespace engine {

AnimatorManager* AnimatorManager::instance = nullptr;

// ============ Animator Base ============

Animator::Animator() {
    AnimatorManager::Instance().Register(this);
}

Animator::~Animator() {
    AnimatorManager::Instance().Cancel(this);
}

void Animator::Finish(bool forced) {
    if (!HasFinished()) {
        state = forced ? AnimatorState::Stopped : AnimatorState::Finished;
        NotifyStopped();
    }
}

void Animator::NotifyStarted() {
    AnimatorManager::Instance().MarkAsRunning(this);
    if (onStart) onStart(this);
}

void Animator::NotifyStopped() {
    AnimatorManager::Instance().MarkAsSuspended(this);
    if (onFinish) onFinish(this);
}

void Animator::NotifyAction(const Animation& anim) {
    if (onAction) onAction(this, anim);
}

// ============ MovingAnimator ============

void MovingAnimator::Start(MovingAnimation* a, timestamp_t t) {
    anim = a;
    lastTime = t;
    currRep = 0;
    state = AnimatorState::Running;
    NotifyStarted();
}

void MovingAnimator::Progress(timestamp_t currTime) {
    while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
        lastTime += anim->GetDelay();
        NotifyAction(*anim);
        
        if (!anim->IsForever() && ++currRep == anim->GetReps()) {
            state = AnimatorState::Finished;
            NotifyStopped();
            return;
        }
    }
}

// ============ FrameRangeAnimator ============

void FrameRangeAnimator::Start(FrameRangeAnimation* a, timestamp_t t) {
    anim = a;
    lastTime = t;
    currFrame = a->GetStartFrame();
    currRep = 0;
    state = AnimatorState::Running;
    NotifyStarted();
    NotifyAction(*anim);  // Apply first frame immediately
}

void FrameRangeAnimator::Progress(timestamp_t currTime) {
    while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
        // Advance frame
        if (currFrame == anim->GetEndFrame()) {
            currFrame = anim->GetStartFrame();
        } else {
            ++currFrame;
        }
        
        lastTime += anim->GetDelay();
        NotifyAction(*anim);
        
        // Check if completed a full cycle
        if (currFrame == anim->GetEndFrame()) {
            if (!anim->IsForever() && ++currRep == anim->GetReps()) {
                state = AnimatorState::Finished;
                NotifyStopped();
                return;
            }
        }
    }
}

// ============ FrameListAnimator ============

void FrameListAnimator::Start(FrameListAnimation* a, timestamp_t t) {
    anim = a;
    lastTime = t;
    currIndex = 0;
    currRep = 0;
    state = AnimatorState::Running;
    NotifyStarted();
    NotifyAction(*anim);
}

void FrameListAnimator::Progress(timestamp_t currTime) {
    const auto& frames = anim->GetFrames();
    if (frames.empty()) return;
    
    while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
        currIndex++;
        if (currIndex >= frames.size()) {
            currIndex = 0;
            if (!anim->IsForever() && ++currRep == anim->GetReps()) {
                state = AnimatorState::Finished;
                NotifyStopped();
                return;
            }
        }
        
        lastTime += anim->GetDelay();
        NotifyAction(*anim);
    }
}

unsigned FrameListAnimator::GetCurrFrame() const {
    if (!anim || anim->GetFrames().empty()) return 0;
    return anim->GetFrames()[currIndex];
}

// ============ FlashAnimator ============

void FlashAnimator::Start(FlashAnimation* a, timestamp_t t) {
    anim = a;
    lastTime = t;
    currRep = 0;
    isVisible = true;
    state = AnimatorState::Running;
    NotifyStarted();
}

void FlashAnimator::Progress(timestamp_t currTime) {
    unsigned delay = isVisible ? anim->GetShowDelay() : anim->GetHideDelay();
    
    while (currTime > lastTime && (currTime - lastTime) >= delay) {
        lastTime += delay;
        isVisible = !isVisible;
        NotifyAction(*anim);
        
        if (!isVisible && ++currRep == anim->GetRepetitions()) {
            state = AnimatorState::Finished;
            NotifyStopped();
            return;
        }
        
        delay = isVisible ? anim->GetShowDelay() : anim->GetHideDelay();
    }
}

// ============ TickAnimator ============

void TickAnimator::Start(TickAnimation* a, timestamp_t t) {
    anim = a;
    lastTime = t;
    currRep = 0;
    elapsedTime = 0;
    state = AnimatorState::Running;
    NotifyStarted();
}

void TickAnimator::Progress(timestamp_t currTime) {
    if (anim->IsDiscrete()) {
        while (currTime > lastTime && (currTime - lastTime) >= anim->GetDelay()) {
            lastTime += anim->GetDelay();
            elapsedTime += anim->GetDelay();
            NotifyAction(*anim);
            
            if (!anim->IsForever() && ++currRep == anim->GetReps()) {
                state = AnimatorState::Finished;
                NotifyStopped();
                return;
            }
        }
    } else {
        // Continuous - called every frame
        elapsedTime = currTime - lastTime;
        NotifyAction(*anim);
    }
}

// ============ AnimatorManager ============

AnimatorManager& AnimatorManager::Instance() {
    if (!instance) {
        instance = new AnimatorManager();
    }
    return *instance;
}

void AnimatorManager::Register(Animator* a) {
    suspended.insert(a);
}

void AnimatorManager::Cancel(Animator* a) {
    suspended.erase(a);
    running.erase(a);
}

void AnimatorManager::MarkAsRunning(Animator* a) {
    suspended.erase(a);
    running.insert(a);
}

void AnimatorManager::MarkAsSuspended(Animator* a) {
    running.erase(a);
    suspended.insert(a);
}

void AnimatorManager::Progress(timestamp_t currTime) {
    auto copy = running;  // Copy to avoid iterator invalidation
    for (auto* a : copy) {
        if (a->IsRunning()) {
            a->Progress(currTime);
        }
    }
}

void AnimatorManager::TimeShift(timestamp_t dt) {
    for (auto* a : running) {
        a->TimeShift(dt);
    }
}

} // namespace engine
