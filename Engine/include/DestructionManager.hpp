#ifndef ENGINE_DESTRUCTION_MANAGER_HPP
#define ENGINE_DESTRUCTION_MANAGER_HPP

#include <list>
#include <cassert>

namespace engine {

// Forward declaration
class DestructionManager;

// ============================================================================
// LatelyDestroyable (from lectures Section 14.1)
// Base class for objects that need deferred destruction
// ============================================================================
class LatelyDestroyable {
protected:
    friend class DestructionManager;
    bool alive = true;
    bool dying = false;
    
    virtual ~LatelyDestroyable() { assert(dying); }
    
    void Delete() { 
        assert(!dying); 
        dying = true; 
        delete this; 
    }
    
public:
    bool IsAlive() const { return alive; }
    
    void Destroy();  // Marks for destruction, actual delete happens in Commit()
    
    LatelyDestroyable() = default;
};

// ============================================================================
// DestructionManager (from lectures Section 14.1)
// Singleton that manages deferred destruction of game objects
// ============================================================================
class DestructionManager {
private:
    std::list<LatelyDestroyable*> dead;
    static DestructionManager* instance;
    
    DestructionManager() = default;
    
public:
    static DestructionManager& Instance();
    
    void Register(LatelyDestroyable* d) {
        assert(!d->IsAlive());
        dead.push_back(d);
    }
    
    void Commit() {
        for (auto* d : dead) {
            d->Delete();
        }
        dead.clear();
    }
    
    bool HasPending() const { return !dead.empty(); }
    size_t GetPendingCount() const { return dead.size(); }
};

// ============================================================================
// Recycled (from lectures Section 14.2)
// Template for object pooling / recycling
// ============================================================================
template <class T>
class Recycled {
protected:
    static std::list<T*> recycler;
    
    static T* TopAndPop() {
        if (recycler.empty()) return nullptr;
        T* x = recycler.front();
        recycler.pop_front();
        return x;
    }
    
public:
    template <class... Types>
    static T* New(Types&&... args) {
        T* recycled = TopAndPop();
        if (recycled) {
            return new (recycled) T(std::forward<Types>(args)...);
        }
        return new T(std::forward<Types>(args)...);
    }
    
    void Recycle() {
        static_cast<T*>(this)->~T();
        recycler.push_back(static_cast<T*>(this));
    }
    
    static void ClearRecycler() {
        for (auto* p : recycler) {
            ::operator delete(p);
        }
        recycler.clear();
    }
    
    static size_t GetRecyclerSize() { return recycler.size(); }
};

template <class T>
std::list<T*> Recycled<T>::recycler;

} // namespace engine

#endif // ENGINE_DESTRUCTION_MANAGER_HPP
