#include "TunnelSystem.hpp"
#include <fstream>
#include <cmath>
#include <iostream>
#include <sstream>

//simple JSON parsing (for the specific structure we have)
//in a real project, you'd use a JSON library like nlohmann/json

namespace app {

TunnelSystem::TunnelSystem() {}

TunnelSystem::~TunnelSystem() {}

bool TunnelSystem::LoadFromFile(const std::string& /*jsonPath*/) {
//for now, we'll hardcode the tunnel data from our analysis
//in production, you'd parse the JSON file
    
    std::cout << "Loading tunnel data..." << std::endl;
    
//TUNNEL 1 (ID: 77) - Down-Right
    {
        Tunnel t1;
        t1.id = 77;
        t1.name = "Tunnel 1";
        t1.direction = "down-right";
        t1.entryX = 5903.0f;  //first checkpoint X
        t1.entryY = 885.5f;  //first checkpoint Y
        t1.exitX = 6145.33f;
        t1.exitY = 1154.0f;
        t1.totalLength = 472.71f;
        
//center path waypoints
        t1.centerPath = {
            {5888.33f, 880.33f, 0}, {5926.33f, 881.33f, 1}, {5960.0f, 886.33f, 2},
            {5982.66f, 897.0f, 3}, {6001.66f, 915.67f, 4}, {6011.33f, 934.67f, 5},
            {6014.33f, 961.67f, 6}, {6005.0f, 984.33f, 7}, {5987.0f, 1005.67f, 8},
            {5975.0f, 1024.67f, 9}, {5968.33f, 1048.67f, 10}, {5972.33f, 1078.67f, 11},
            {5981.66f, 1101.33f, 12}, {6002.66f, 1120.33f, 13}, {6030.66f, 1133.67f, 14},
            {6090.33f, 1136.0f, 15}, {6111.66f, 1119.67f, 16}
        };
        
//checkpoints (ordered from start to end)
        t1.checkpoints = {
            {0, 5903.0f, 885.5f, 0.0f, 0.0f},
            {1, 5920.5f, 885.5f, 38.0f, 8.0f},
            {2, 5940.5f, 887.5f, 38.0f, 8.0f},
            {3, 5958.5f, 893.5f, 72.1f, 15.2f},
            {4, 5978.0f, 905.0f, 97.1f, 20.5f},
            {5, 5996.5f, 922.0f, 123.7f, 26.2f},
            {6, 6010.0f, 941.0f, 145.1f, 30.7f},
            {7, 6013.5f, 963.5f, 172.2f, 36.4f},
            {8, 6005.0f, 990.0f, 196.7f, 41.6f},
            {9, 5990.5f, 1013.5f, 224.6f, 47.5f},
            {10, 5973.5f, 1032.0f, 247.1f, 52.3f},
            {11, 5970.5f, 1058.5f, 272.0f, 57.5f},
            {12, 5971.5f, 1086.5f, 302.3f, 63.9f},
            {13, 5982.5f, 1113.0f, 326.8f, 69.1f},
            {14, 6004.5f, 1128.0f, 355.1f, 75.1f},
            {15, 6024.0f, 1138.5f, 386.1f, 81.7f},
            {16, 6041.0f, 1142.5f, 386.1f, 81.7f},
            {17, 6062.5f, 1143.5f, 445.8f, 94.3f},
            {18, 6082.5f, 1144.5f, 445.8f, 94.3f},
            {19, 6103.0f, 1145.0f, 445.8f, 94.3f},
            {20, 6121.5f, 1145.0f, 472.7f, 100.0f},
            {21, 6138.0f, 1145.0f, 472.7f, 100.0f}
        };
        
        tunnels.push_back(t1);
    }
    
//TUNNEL 2 (ID: 100) - Down-Right
    {
        Tunnel t2;
        t2.id = 100;
        t2.name = "Tunnel 2";
        t2.direction = "down-right";
        t2.entryX = 6412.5f;  //first checkpoint X
        t2.entryY = 1142.5f;  //first checkpoint Y
        t2.exitX = 6656.5f;
        t2.exitY = 1409.0f;
        t2.totalLength = 501.6f;
        
//center path waypoints
        t2.centerPath = {
            {6400.75f, 1136.0f, 0}, {6437.5f, 1136.5f, 1}, {6470.5f, 1141.25f, 2},
            {6494.25f, 1150.0f, 3}, {6507.75f, 1164.25f, 4}, {6523.0f, 1185.5f, 5},
            {6526.5f, 1205.75f, 6}, {6522.25f, 1231.25f, 7}, {6507.0f, 1251.5f, 8},
            {6488.0f, 1277.0f, 9}, {6482.0f, 1300.0f, 10}, {6482.0f, 1329.75f, 11},
            {6492.5f, 1352.75f, 12}, {6509.75f, 1375.25f, 13}, {6536.0f, 1388.25f, 14},
            {6569.75f, 1391.5f, 15}, {6611.75f, 1391.5f, 16}, {6656.25f, 1392.0f, 17}
        };
        
//checkpoints
        t2.checkpoints = {
            {0, 6412.5f, 1142.5f, 0.0f, 0.0f},
            {1, 6433.0f, 1143.5f, 36.8f, 7.3f},
            {2, 6453.0f, 1143.5f, 36.8f, 7.3f},
            {3, 6475.5f, 1149.0f, 70.1f, 14.0f},
            {4, 6495.5f, 1160.5f, 95.4f, 19.0f},
            {5, 6512.0f, 1176.0f, 115.0f, 22.9f},
            {6, 6523.5f, 1204.5f, 161.7f, 32.2f},
            {7, 6523.5f, 1233.0f, 187.6f, 37.4f},
            {8, 6508.5f, 1257.5f, 212.9f, 42.5f},
            {9, 6493.0f, 1280.0f, 244.7f, 48.8f},
            {10, 6482.0f, 1305.0f, 268.5f, 53.5f},
            {11, 6479.5f, 1333.5f, 298.3f, 59.5f},
            {12, 6494.0f, 1362.5f, 323.5f, 64.5f},
            {13, 6516.5f, 1384.5f, 351.9f, 70.2f},
            {14, 6541.5f, 1396.0f, 381.2f, 76.0f},
            {15, 6570.0f, 1399.0f, 415.1f, 82.8f},
            {16, 6595.0f, 1399.0f, 457.1f, 91.1f},
            {17, 6625.0f, 1399.0f, 457.1f, 91.1f},
            {18, 6646.5f, 1400.5f, 501.6f, 100.0f}
        };
        
        tunnels.push_back(t2);
    }
    
//TUNNEL 3 (ID: 120) - Down-Left (REVERSED!)
    {
        Tunnel t3;
        t3.id = 120;
        t3.name = "Tunnel 3";
        t3.direction = "down-left";
        t3.entryX = 6447.5f;  //START at the TOP (was last waypoint)
        t3.entryY = 640.0f;  //TOP position (smaller Y)
        t3.exitX = 6655.0f;  //END at the BOTTOM (was first waypoint)
        t3.exitY = 880.25f;  //BOTTOM position (larger Y)
        t3.totalLength = 445.91f;
        
//center path waypoints - REVERSED ORDER (top to bottom)
        t3.centerPath = {
            {6447.5f, 640.0f, 0}, {6435.75f, 642.0f, 1}, {6443.75f, 631.75f, 2},
            {6477.5f, 637.5f, 3}, {6490.25f, 649.25f, 4}, {6496.25f, 661.5f, 5},
            {6504.5f, 676.0f, 6}, {6508.5f, 694.5f, 7}, {6504.0f, 709.0f, 8},
            {6505.5f, 735.5f, 9}, {6501.25f, 771.0f, 10}, {6490.5f, 803.0f, 11},
            {6495.25f, 826.0f, 12}, {6506.0f, 853.25f, 13}, {6533.5f, 873.5f, 14},
            {6579.0f, 880.25f, 15}, {6655.0f, 880.25f, 16}
        };
        
//checkpoints - REVERSED ORDER (top to bottom)
        t3.checkpoints = {
            {0, 6462.0f, 636.0f, 0.0f, 0.0f},
            {1, 6412.0f, 630.5f, 12.0f, 2.7f},
            {2, 6434.5f, 630.0f, 24.9f, 5.6f},
            {3, 6486.0f, 644.0f, 76.5f, 17.2f},
            {4, 6509.0f, 664.5f, 106.8f, 23.9f},
            {5, 6522.5f, 692.5f, 125.7f, 28.2f},
            {6, 6503.5f, 751.0f, 167.5f, 37.6f},
            {7, 6518.5f, 727.5f, 167.5f, 37.6f},
            {8, 6485.5f, 779.0f, 203.2f, 45.6f},
            {9, 6480.5f, 814.5f, 237.0f, 53.1f},
            {10, 6490.0f, 838.5f, 260.5f, 58.4f},
            {11, 6502.0f, 862.0f, 289.8f, 65.0f},
            {12, 6550.0f, 885.5f, 323.9f, 72.6f},
            {13, 6523.5f, 879.0f, 323.9f, 72.6f},
            {14, 6608.5f, 887.5f, 369.9f, 82.9f},
            {15, 6581.0f, 887.5f, 369.9f, 82.9f},
            {16, 6652.0f, 889.0f, 445.9f, 100.0f},
            {17, 6633.5f, 888.5f, 445.9f, 100.0f}
        };
        
        tunnels.push_back(t3);
    }
    
    std::cout << "Loaded " << tunnels.size() << " tunnels successfully!" << std::endl;
    return true;
}

int TunnelSystem::CheckTunnelEntry(float x, float y) const {
//check each tunnel's entry area
//using distance check to entry point
    
    for (size_t i = 0; i < tunnels.size(); ++i) {
        float dx = x - tunnels[i].entryX;
        float dy = y - tunnels[i].entryY;
        float distSq = dx * dx + dy * dy;
        float dist = std::sqrt(distSq);
        
//special handling for Tunnel 3 (upper tunnel) - larger radius
        float radius = (i == 2) ? 80.0f : 60.0f;
        
        if (dist < radius) {
            std::cout << "TUNNEL ENTRY: " << tunnels[i].name << " at distance " << dist << "px (limit: " << radius << "px)" << std::endl;
            return static_cast<int>(i);
        }
    }
    
    return -1;  //not in any tunnel
}

void TunnelSystem::EnterTunnel(int tunnelIndex) {
    if (tunnelIndex < 0 || tunnelIndex >= static_cast<int>(tunnels.size())) {
        return;
    }
    
    std::cout << "=== ENTERING TUNNEL " << tunnels[tunnelIndex].id 
              << " (" << tunnels[tunnelIndex].name << ") ===" << std::endl;
    
    active = true;
    currentTunnelIndex = tunnelIndex;
    currentWaypointIndex = 0;
    pathProgress = 0.0f;
}

bool TunnelSystem::UpdateTunnelMovement(float& sonicX, float& sonicY, float& facingDir, float deltaTime) {
    if (!active || currentTunnelIndex < 0) {
        return false;
    }
    
    const Tunnel& tunnel = tunnels[currentTunnelIndex];
    
//calculate how much to move along path this frame
    float distanceToMove = tunnelSpeed * deltaTime;
    
//get current waypoint and next waypoint
    if (currentWaypointIndex >= static_cast<int>(tunnel.centerPath.size()) - 1) {
        std::cout << "=== EXITED TUNNEL " << tunnel.id << " ===" << std::endl;
        return false;  //signal tunnel complete
    }
    
    const TunnelWaypoint& currentWP = tunnel.centerPath[currentWaypointIndex];
    const TunnelWaypoint& nextWP = tunnel.centerPath[currentWaypointIndex + 1];
    
//calculate distance between waypoints
    float dx = nextWP.x - currentWP.x;
    float dy = nextWP.y - currentWP.y;
    float segmentLength = std::sqrt(dx * dx + dy * dy);
    
//move along current segment
    float segmentProgress = (distanceToMove / segmentLength);
    pathProgress += segmentProgress;
    
//check if we've reached next waypoint
    if (pathProgress >= 1.0f) {
        pathProgress = 0.0f;
        currentWaypointIndex++;
        
//recheck if we've finished
        if (currentWaypointIndex >= static_cast<int>(tunnel.centerPath.size()) - 1) {
            std::cout << "=== EXITED TUNNEL " << tunnel.id << " ===" << std::endl;
            return false;
        }
        
        const TunnelWaypoint& newCurrent = tunnel.centerPath[currentWaypointIndex];
        const TunnelWaypoint& newNext = tunnel.centerPath[currentWaypointIndex + 1];
        dx = newNext.x - newCurrent.x;
        dy = newNext.y - newCurrent.y;
    }
    
//interpolate position along current segment
    const TunnelWaypoint& wp1 = tunnel.centerPath[currentWaypointIndex];
    const TunnelWaypoint& wp2 = tunnel.centerPath[currentWaypointIndex + 1];
    
    sonicX = wp1.x + (wp2.x - wp1.x) * pathProgress + tunnel.offsetX;
    sonicY = wp1.y + (wp2.y - wp1.y) * pathProgress + tunnel.offsetY;
    
    if (dx > 0.1f) {
        facingDir = 1.0f;  //right
    } else if (dx < -0.1f) {
        facingDir = -1.0f;  //left
    }
    
    return true;  //continue tunnel movement
}

void TunnelSystem::ExitTunnel() {
    active = false;
    currentTunnelIndex = -1;
    currentWaypointIndex = 0;
    pathProgress = 0.0f;
}

float TunnelSystem::GetProgress() const {
    if (!active || currentTunnelIndex < 0) {
        return 0.0f;
    }
    
    const Tunnel& tunnel = tunnels[currentTunnelIndex];
    
//calculate overall progress through tunnel
    int totalWaypoints = static_cast<int>(tunnel.centerPath.size()) - 1;
    if (totalWaypoints <= 0) return 0.0f;
    
    float overallProgress = (currentWaypointIndex + pathProgress) / totalWaypoints;
    return overallProgress * 100.0f;  //return as percentage
}

}  //namespace app
