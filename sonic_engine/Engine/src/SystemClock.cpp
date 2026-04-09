#include "SystemClock.hpp"

namespace engine {

SystemClock* SystemClock::instance = nullptr;

SystemClock& SystemClock::Instance() {
    if (!instance) {
        instance = new SystemClock();
    }
    return *instance;
}

}  //namespace engine
