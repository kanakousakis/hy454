#ifndef TUNNEL_SYSTEM_HPP
#define TUNNEL_SYSTEM_HPP

#include <vector>
#include <string>

namespace app {

//single checkpoint along tunnel path
struct TunnelCheckpoint {
    int index;
    float x, y;
    float distanceAlongPath;
    float progressPercent;
};

//center path waypoint for smooth movement
struct TunnelWaypoint {
    float x, y;
    int segment;
};

//a single tunnel with its path
struct Tunnel {
    int id;
    std::string name;
    std::string direction;  //"down-right", "up-left", etc.
    float entryX, entryY;
    float exitX, exitY;
    float totalLength;
    
    std::vector<TunnelWaypoint> centerPath;
    std::vector<TunnelCheckpoint> checkpoints;
    
//for moving tunnels
    float offsetX = 0.0f;
    float offsetY = 0.0f;
};

class TunnelSystem {
public:
    TunnelSystem();
    ~TunnelSystem();
    
//load tunnel data from JSON file
    bool LoadFromFile(const std::string& jsonPath);
    
//check if position is inside any tunnel entrance (polygon collision)
//returns tunnel index, or -1 if not in any tunnel
    int CheckTunnelEntry(float x, float y) const;
    
//activate tunnel mode
    void EnterTunnel(int tunnelIndex);
    
//returns false when tunnel is complete
    bool UpdateTunnelMovement(float& sonicX, float& sonicY, float& facingDir, float deltaTime);
    
//exit tunnel mode
    void ExitTunnel();
    
//get current tunnel progress (0-100%)
    float GetProgress() const;
    
//check if currently in a tunnel
    bool IsActive() const { return active; }
    
//get current tunnel ID (-1 if none)
    int GetCurrentTunnelId() const { return active ? tunnels[currentTunnelIndex].id : -1; }
    
private:
    std::vector<Tunnel> tunnels;
    
//current state
    bool active = false;
    int currentTunnelIndex = -1;
    int currentWaypointIndex = 0;
    float pathProgress = 0.0f;  //0.0 to 1.0 along current segment
    
//movement settings
    float tunnelSpeed = 300.0f;  //pixels per second
    
//helper functions
    bool IsPointInTunnelPolygon(float x, float y, int tunnelIndex) const;
    void InterpolatePath(const Tunnel& tunnel, float& x, float& y) const;
};

}  //namespace app

#endif  //TUNNEL_SYSTEM_HPP
