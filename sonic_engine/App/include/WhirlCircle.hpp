#pragma once

#include "Engine.hpp"
#include "SonicPlayer.hpp"
#include <vector>
#include <cmath>
#include <functional>
#include <iostream>

namespace whirl {

struct Point2D {
    float x, y;
    Point2D(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

struct WhirlCircleData {
    Point2D center;
    float radius;
    std::vector<Point2D> pathPoints;  // Polygon points tracing the circle
    int objectId;

    WhirlCircleData() : radius(0), objectId(0) {}
};

struct TriangleTeleportZone {
    Point2D vertices[4];  // Triangle vertices (actually 4 points in your case)
    Point2D entryEdge[2];   // First edge (entry point)
    Point2D exitEdge[2];    // Opposite edge (exit point)
    int objectId;

    TriangleTeleportZone() : objectId(0) {}

    // Check if point is inside triangle
    bool ContainsPoint(float x, float y) const;

    // Get which edge the point is closest to
    int GetClosestEdge(float x, float y) const;
};

class WhirlCircleLoader {
public:
    static bool LoadFromTMJ(const char* filename,
                           std::vector<WhirlCircleData>& circles,
                           std::vector<TriangleTeleportZone>& teleportZones);

private:
    static Point2D CalculateCenter(const std::vector<Point2D>& points);
    static float CalculateRadius(const Point2D& center, const std::vector<Point2D>& points);
};

class SpinLoopHandler {
private:
    app::SonicPlayer* player;
    engine::TickAnimator* tickAnimator;
    engine::TickAnimation* tickAnim;

    Point2D center;
    float radius;
    float rotationSpeedRadians;
    const double pi = 3.14159265359;
    const double angleRadiansOffset = -pi * 0.5;  // Start at -90 degrees (bottom)

    bool isActive;
    bool allowSecondLoop;
    int completedLoops;

    std::function<void(void)> onBegin;
    std::function<void(void)> onEnd;
    std::function<void(float)> onSpin;
    std::function<bool(void)> predSpinMore;

    float ToDegrees(float radians) const {
        return 180.0f * radians / pi;
    }

    float ComputeAngleRadians(uint64_t totalDelay) const {
        return float(totalDelay) * rotationSpeedRadians;
    }

    Point2D ComputePosition(float angleRadians) const {
        angleRadians += angleRadiansOffset;  // Offset to start at bottom
        float x = center.x + radius * std::cos(angleRadians);
        float y = center.y + radius * std::sin(angleRadians);
        return Point2D(x, y);
    }

    bool CheckIfSpinsCompleted(float* angleDegrees) {
        if (*angleDegrees >= 720.0f) {
            *angleDegrees = 720.0f;
            completedLoops = 2;
            return true;
        }
        else if (*angleDegrees >= 360.0f) {
            if (!predSpinMore || !predSpinMore()) {
                *angleDegrees = 360.0f;
                completedLoops = 1;
                return true;
            }
            else {
                completedLoops = 1;
                return false;
            }
        }
        return false;
    }

    void AlignPlayer(const Point2D& pos, float degrees) {
        if (player) {
            player->SetPosition(pos.x, pos.y);
            if (onSpin)
                onSpin(degrees);
        }
    }

    void Progress(uint64_t totalDelay) {
        auto radians = ComputeAngleRadians(totalDelay);
        auto pos = ComputePosition(radians);
        auto degrees = ToDegrees(radians);
        auto done = CheckIfSpinsCompleted(&degrees);

        AlignPlayer(pos, degrees);

        if (done)
            End();
    }

public:
    SpinLoopHandler()
        : player(nullptr), tickAnimator(nullptr), tickAnim(nullptr),
          radius(0), rotationSpeedRadians(0), isActive(false),
          allowSecondLoop(false), completedLoops(0) {}

    ~SpinLoopHandler() {
        Destroy();
    }

    void Create(const Point2D& _center,
                float _radius,
                uint64_t fullSpinDelayMsecs,
                app::SonicPlayer* _player) {
        player = _player;
        radius = _radius;
        center = _center;
        rotationSpeedRadians = float(2.0 * pi / double(fullSpinDelayMsecs));

        tickAnimator = new engine::TickAnimator();
        tickAnim = new engine::TickAnimation("tick.anim.spinloop", 0, 1, false);
        isActive = false;
        completedLoops = 0;
    }

    void Destroy() {
        if (tickAnimator) {
            delete tickAnimator;
            tickAnimator = nullptr;
        }
        if (tickAnim) {
            delete tickAnim;
            tickAnim = nullptr;
        }
    }

    void Begin() {
        if (!tickAnimator || isActive) return;

        isActive = true;
        completedLoops = 0;

        tickAnimator->SetOnAction(
            [this](engine::Animator* animator, const engine::Animation&) {
                Progress(static_cast<engine::TickAnimator*>(animator)->GetElapsedTime());
            }
        );

        tickAnimator->Start(tickAnim, engine::GetSystemTime());

        if (onBegin)
            onBegin();
    }

    void End() {
        if (!tickAnimator || !isActive) return;

        tickAnimator->Stop();
        isActive = false;

        if (onEnd)
            onEnd();
    }

    bool IsActive() const { return isActive; }
    int GetCompletedLoops() const { return completedLoops; }

    void SetOnBegin(const std::function<void(void)>& f) { onBegin = f; }
    void SetOnEnd(const std::function<void(void)>& f) { onEnd = f; }
    void SetOnSpin(const std::function<void(float)>& f) { onSpin = f; }
    void SetPredSpinMore(const std::function<bool(void)>& f) { predSpinMore = f; }
};

class WhirlCircleManager {
private:
    std::vector<WhirlCircleData> circles;
    std::vector<TriangleTeleportZone> teleportZones;
    std::vector<SpinLoopHandler*> handlers;
    app::SonicPlayer* sonic;

    bool lastInTeleportZone;
    size_t lastZoneIndex;

public:
    WhirlCircleManager() : sonic(nullptr), lastInTeleportZone(false), lastZoneIndex(0) {}

    ~WhirlCircleManager() {
        Clear();
    }

    bool LoadFromFile(const char* filename) {
        Clear();
        return WhirlCircleLoader::LoadFromTMJ(filename, circles, teleportZones);
    }

    void Initialize(app::SonicPlayer* player) {
        sonic = player;

        // Create handlers for each circle
        for (const auto& circle : circles) {
            SpinLoopHandler* handler = new SpinLoopHandler();
            handler->Create(circle.center, circle.radius, 2000, player);  // 2 second spin

            // Set up callbacks
            handler->SetOnBegin([player]() {
                // Disable normal physics
                player->DisablePhysics();
            });

            handler->SetOnEnd([player]() {
                // Re-enable physics
                player->EnablePhysics();
            });

            handler->SetPredSpinMore([]() -> bool {
                // Check if player is holding jump button for second loop
                return engine::IsKeyPressed(engine::KeyCode::Space);
            });

            handlers.push_back(handler);
        }
    }

    void Update() {
        if (!sonic) return;

        float sonicX = sonic->GetX();
        float sonicY = sonic->GetY();

        // Debug: Show when Sonic is in the whirl area
        static int debugCounter = 0;
        if (sonicX >= 5200.0f && sonicX <= 5350.0f && debugCounter++ % 60 == 0) {
            std::cout << "WHIRL DEBUG: Sonic at (" << sonicX << ", " << sonicY
                      << ") Zones: " << teleportZones.size()
                      << " Handlers: " << handlers.size() << std::endl;

            // Check distance to first zone vertices
            if (!teleportZones.empty()) {
                const auto& zone = teleportZones[0];
                std::cout << "  Zone vertices: [0]=(" << zone.vertices[0].x << "," << zone.vertices[0].y
                          << ") [1]=(" << zone.vertices[1].x << "," << zone.vertices[1].y << ")" << std::endl;
            }
        }

        // Check triangle teleport zones (trigger whirl on entry)
        for (size_t i = 0; i < teleportZones.size(); ++i) {
            const auto& zone = teleportZones[i];
            bool isInZone = zone.ContainsPoint(sonicX, sonicY);

            if (isInZone && !lastInTeleportZone) {
                // Just entered zone - trigger whirl animation
                std::cout << "WHIRL: Entered triangle zone at (" << sonicX << ", " << sonicY << ")" << std::endl;

                // Start the whirl for this circle (match zone index to handler index)
                if (i < handlers.size() && !handlers[i]->IsActive()) {
                    std::cout << "WHIRL: Starting spin loop!" << std::endl;
                    handlers[i]->Begin();
                }

                lastInTeleportZone = true;
                lastZoneIndex = i;
            }
            else if (!isInZone && lastInTeleportZone && lastZoneIndex == i) {
                lastInTeleportZone = false;
            }
        }
    }

    void Clear() {
        for (auto* handler : handlers) {
            delete handler;
        }
        handlers.clear();
        circles.clear();
        teleportZones.clear();
        sonic = nullptr;
        lastInTeleportZone = false;
        lastZoneIndex = 0;
    }

private:
    void HandleTeleport(const TriangleTeleportZone& zone, float sonicX, float sonicY) {
        if (!sonic) return;

        // Determine which edge sonic entered from
        int edgeIndex = zone.GetClosestEdge(sonicX, sonicY);

        // Teleport to opposite edge
        if (edgeIndex == 0) {
            // Entered from entry edge, teleport to exit edge
            sonic->SetPosition(zone.exitEdge[0].x, zone.exitEdge[0].y);
        } else {
            // Entered from exit edge, teleport to entry edge
            sonic->SetPosition(zone.entryEdge[0].x, zone.entryEdge[0].y);
        }
    }
};

} // namespace whirl
