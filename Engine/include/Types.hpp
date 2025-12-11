#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace engine {

// Basic types
using Dim = unsigned short;
using Color = unsigned int;
using RGBValue = unsigned char;
using Alpha = unsigned char;
using GridIndex = unsigned char;
using timestamp_t = uint64_t;
using byte = unsigned char;

// Structures
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}
};

struct Point {
    int x = 0, y = 0;
    Point() = default;
    Point(int _x, int _y) : x(_x), y(_y) {}
};

struct RGB {
    RGBValue r = 0, g = 0, b = 0;
    RGB() = default;
    RGB(RGBValue _r, RGBValue _g, RGBValue _b) : r(_r), g(_g), b(_b) {}
};

struct RGBA : public RGB {
    Alpha a = 255;
    RGBA() = default;
    RGBA(RGBValue _r, RGBValue _g, RGBValue _b, Alpha _a = 255) 
        : RGB(_r, _g, _b), a(_a) {}
};

// Enums
enum class BitDepth { bits8 = 1, bits16, bits24, bits32 };

enum class AnimatorState {
    Finished = 0,
    Running = 1,
    Stopped = 2
};

// Helper functions
inline Color MakeColor(RGBValue r, RGBValue g, RGBValue b, Alpha a = 255) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

inline RGBValue GetRed(Color c)   { return (c >> 16) & 0xFF; }
inline RGBValue GetGreen(Color c) { return (c >> 8) & 0xFF; }
inline RGBValue GetBlue(Color c)  { return c & 0xFF; }
inline Alpha GetAlpha(Color c)    { return (c >> 24) & 0xFF; }

template <typename T>
inline int Sign(T x) { return (x > 0) ? 1 : ((x < 0) ? -1 : 0); }

} // namespace engine
