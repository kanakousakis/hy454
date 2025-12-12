#pragma once

#include "Types.hpp"
#include <chrono>

namespace engine {

class SystemClock {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    
    TimePoint startTime;
    static SystemClock* instance;
    
    SystemClock() : startTime(Clock::now()) {}

public:
    static SystemClock& Instance();
    
    timestamp_t GetMillis() const {
        auto now = Clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime).count();
    }
    
    timestamp_t GetMicros() const {
        auto now = Clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now - startTime).count();
    }
    
    void Reset() {
        startTime = Clock::now();
    }
};

// Convenience functions
inline timestamp_t GetSystemTime() { 
    return SystemClock::Instance().GetMillis(); 
}

inline timestamp_t GetGameTime() { 
    return SystemClock::Instance().GetMillis(); 
}

} // namespace engine
