#include "WhirlCircle.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>

namespace whirl {

// Helper function to parse JSON-like structure
static bool ParsePolygon(const std::string& data, size_t startPos,
                        std::vector<Point2D>& points, float baseX, float baseY) {
    points.clear();

    size_t polygonStart = data.find("\"polygon\":[", startPos);
    if (polygonStart == std::string::npos) return false;

    size_t polygonEnd = data.find("]", polygonStart);
    if (polygonEnd == std::string::npos) return false;

    size_t pos = polygonStart + 11;  // Skip "\"polygon\":["

    while (pos < polygonEnd) {
        // Find x value
        size_t xPos = data.find("\"x\":", pos);
        if (xPos == std::string::npos || xPos > polygonEnd) break;

        size_t xValStart = xPos + 4;
        size_t xValEnd = data.find_first_of(",}", xValStart);
        std::string xStr = data.substr(xValStart, xValEnd - xValStart);
        float x = std::stof(xStr);

        // Find y value
        size_t yPos = data.find("\"y\":", xValEnd);
        if (yPos == std::string::npos || yPos > polygonEnd) break;

        size_t yValStart = yPos + 4;
        size_t yValEnd = data.find_first_of(",}", yValStart);
        std::string yStr = data.substr(yValStart, yValEnd - yValStart);
        float y = std::stof(yStr);

        points.push_back(Point2D(baseX + x, baseY + y));

        pos = data.find("{", yValEnd);
        if (pos == std::string::npos || pos > polygonEnd) break;
    }

    return !points.empty();
}

// Triangle contains point test
bool TriangleTeleportZone::ContainsPoint(float x, float y) const {
    // Use barycentric coordinates for triangle test
    // Treating first 3 vertices as the triangle
    Point2D v0 = vertices[0];
    Point2D v1 = vertices[1];
    Point2D v2 = vertices[2];

    float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);
    float a = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denom;
    float b = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denom;
    float c = 1 - a - b;

    return (a >= 0 && a <= 1 && b >= 0 && b <= 1 && c >= 0 && c <= 1);
}

int TriangleTeleportZone::GetClosestEdge(float x, float y) const {
    // Calculate distance to entry edge (vertices 0-1)
    float dx1 = vertices[1].x - vertices[0].x;
    float dy1 = vertices[1].y - vertices[0].y;
    float len1 = std::sqrt(dx1*dx1 + dy1*dy1);

    float t1 = ((x - vertices[0].x) * dx1 + (y - vertices[0].y) * dy1) / (len1 * len1);
    t1 = std::max(0.0f, std::min(1.0f, t1));
    float closestX1 = vertices[0].x + t1 * dx1;
    float closestY1 = vertices[0].y + t1 * dy1;
    float dist1 = std::sqrt((x - closestX1)*(x - closestX1) + (y - closestY1)*(y - closestY1));

    // Calculate distance to exit edge (vertices 2-3 or 2-0 depending on structure)
    int v2Idx = 2;
    int v3Idx = 3;
    float dx2 = vertices[v3Idx].x - vertices[v2Idx].x;
    float dy2 = vertices[v3Idx].y - vertices[v2Idx].y;
    float len2 = std::sqrt(dx2*dx2 + dy2*dy2);

    float t2 = ((x - vertices[v2Idx].x) * dx2 + (y - vertices[v2Idx].y) * dy2) / (len2 * len2);
    t2 = std::max(0.0f, std::min(1.0f, t2));
    float closestX2 = vertices[v2Idx].x + t2 * dx2;
    float closestY2 = vertices[v2Idx].y + t2 * dy2;
    float dist2 = std::sqrt((x - closestX2)*(x - closestX2) + (y - closestY2)*(y - closestY2));

    return (dist1 < dist2) ? 0 : 1;
}

Point2D WhirlCircleLoader::CalculateCenter(const std::vector<Point2D>& points) {
    if (points.empty()) return Point2D(0, 0);

    float sumX = 0, sumY = 0;
    for (const auto& p : points) {
        sumX += p.x;
        sumY += p.y;
    }

    return Point2D(sumX / points.size(), sumY / points.size());
}

float WhirlCircleLoader::CalculateRadius(const Point2D& center, const std::vector<Point2D>& points) {
    if (points.empty()) return 0;

    float sumDist = 0;
    for (const auto& p : points) {
        float dx = p.x - center.x;
        float dy = p.y - center.y;
        sumDist += std::sqrt(dx*dx + dy*dy);
    }

    return sumDist / points.size();
}

bool WhirlCircleLoader::LoadFromTMJ(const char* filename,
                                    std::vector<WhirlCircleData>& circles,
                                    std::vector<TriangleTeleportZone>& teleportZones) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return false;
    }

    // Read entire file
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Find the Whirl layer
    size_t whirlLayerPos = content.find("\"name\":\"Whirl\"");
    if (whirlLayerPos == std::string::npos) {
        std::cerr << "Whirl layer not found in " << filename << std::endl;
        return false;
    }

    // Find objects array in Whirl layer
    size_t objectsPos = content.find("\"objects\":[", whirlLayerPos);
    if (objectsPos == std::string::npos) {
        std::cerr << "No objects found in Whirl layer" << std::endl;
        return false;
    }

    // Find the end of the Whirl layer
    size_t layerEnd = content.find("}]", whirlLayerPos);

    // Parse object id:140 (circle)
    size_t obj140Pos = content.find("\"id\":140", objectsPos);
    if (obj140Pos != std::string::npos && obj140Pos < layerEnd) {
        // Get base position
        size_t xPos = content.find("\"x\":", obj140Pos);
        size_t xValEnd = content.find(",", xPos + 4);
        float baseX = std::stof(content.substr(xPos + 4, xValEnd - (xPos + 4)));

        size_t yPos = content.find("\"y\":", xValEnd);
        size_t yValEnd = content.find_first_of("\n}", yPos + 4);
        float baseY = std::stof(content.substr(yPos + 4, yValEnd - (yPos + 4)));

        // Parse polygon
        WhirlCircleData circle;
        circle.objectId = 140;

        if (ParsePolygon(content, obj140Pos, circle.pathPoints, baseX, baseY)) {
            circle.center = CalculateCenter(circle.pathPoints);
            circle.radius = CalculateRadius(circle.center, circle.pathPoints);

            std::cout << "Loaded Whirl Circle:" << std::endl;
            std::cout << "  Center: (" << circle.center.x << ", " << circle.center.y << ")" << std::endl;
            std::cout << "  Radius: " << circle.radius << std::endl;
            std::cout << "  Points: " << circle.pathPoints.size() << std::endl;

            circles.push_back(circle);
        }
    }

    // Parse object id:141 (triangle teleport zone)
    size_t obj141Pos = content.find("\"id\":141", objectsPos);
    std::cout << "DEBUG: Searching for object 141, found at pos: " << obj141Pos << std::endl;
    if (obj141Pos != std::string::npos) {
        std::cout << "DEBUG: Object 141 found, parsing..." << std::endl;

        // Find the end of the polygon array first, then search for base x,y after it
        size_t polygonEnd = content.find("]", obj141Pos);
        if (polygonEnd == std::string::npos) {
            std::cout << "ERROR: Could not find polygon end" << std::endl;
            return !circles.empty();
        }

        // Get base position (search after polygon array)
        size_t xPos = content.find("\"x\":", polygonEnd);
        size_t xValEnd = content.find(",", xPos + 4);
        float baseX = std::stof(content.substr(xPos + 4, xValEnd - (xPos + 4)));

        size_t yPos = content.find("\"y\":", xValEnd);
        size_t yValEnd = content.find_first_of("\n}", yPos + 4);
        float baseY = std::stof(content.substr(yPos + 4, yValEnd - (yPos + 4)));

        // Parse polygon
        std::vector<Point2D> points;
        std::cout << "DEBUG: Parsing polygon with baseX=" << baseX << " baseY=" << baseY << std::endl;
        if (ParsePolygon(content, obj141Pos, points, baseX, baseY)) {
            std::cout << "DEBUG: ParsePolygon returned " << points.size() << " points" << std::endl;
            if (!points.empty()) {
                std::cout << "DEBUG: First point: (" << points[0].x << ", " << points[0].y << ")" << std::endl;
            }
            TriangleTeleportZone zone;
            zone.objectId = 141;

            // Copy vertices (up to 4)
            for (size_t i = 0; i < std::min(points.size(), size_t(4)); ++i) {
                zone.vertices[i] = points[i];
            }

            // Set entry edge (first two vertices)
            zone.entryEdge[0] = zone.vertices[0];
            zone.entryEdge[1] = zone.vertices[1];

            // Set exit edge (last two vertices)
            zone.exitEdge[0] = zone.vertices[2];
            zone.exitEdge[1] = zone.vertices[3];

            std::cout << "Loaded Triangle Teleport Zone:" << std::endl;
            std::cout << "  Vertices: " << points.size() << std::endl;
            for (size_t i = 0; i < points.size(); ++i) {
                std::cout << "    [" << i << "]: (" << zone.vertices[i].x << ", " << zone.vertices[i].y << ")" << std::endl;
            }

            teleportZones.push_back(zone);
        }
    }

    return !circles.empty();
}

} // namespace whirl
