#pragma once

#include "Types.hpp"
#include <SFML/Window.hpp>
#include <functional>
#include <unordered_map>

namespace engine {

//key codes matching common game controls
enum class KeyCode {
    Unknown = -1,
//arrow keys
    Left, Right, Up, Down,
//letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
//numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
//function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
//special keys
    Space, Enter, Escape, Tab, Backspace,
    LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
    Home, End, PageUp, PageDown, Insert, Delete,
    Plus, Minus, Equals, Period, Comma,
//count
    KeyCount
};

enum class MouseButton {
    Left, Right, Middle,
    ButtonCount
};

//input manager - singleton
class InputManager {
private:
    static InputManager* instance;
    
    bool currentKeys[static_cast<int>(KeyCode::KeyCount)] = {false};
    bool previousKeys[static_cast<int>(KeyCode::KeyCount)] = {false};
    
    bool currentMouse[static_cast<int>(MouseButton::ButtonCount)] = {false};
    bool previousMouse[static_cast<int>(MouseButton::ButtonCount)] = {false};
    
    Point mousePos;
    Point mouseDelta;
    bool windowClosed = false;
    
    InputManager() = default;
    
    KeyCode SFMLKeyToKeyCode(sf::Keyboard::Key key);
    
public:
    static InputManager& Instance();
    
    void Poll();  //process SFML events
    void Update();
    
//keyboard
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyJustPressed(KeyCode key) const;
    bool IsKeyJustReleased(KeyCode key) const;
    
//mouse
    bool IsMousePressed(MouseButton btn) const;
    bool IsMouseJustPressed(MouseButton btn) const;
    bool IsMouseJustReleased(MouseButton btn) const;
    Point GetMousePosition() const { return mousePos; }
    Point GetMouseDelta() const { return mouseDelta; }
    
//window
    bool IsWindowClosed() const { return windowClosed; }
    void ResetWindowClosed() { windowClosed = false; }
};

//convenience functions
inline InputManager& GetInput() { return InputManager::Instance(); }

inline bool IsKeyPressed(KeyCode key) { return GetInput().IsKeyPressed(key); }
inline bool IsKeyJustPressed(KeyCode key) { return GetInput().IsKeyJustPressed(key); }
inline bool IsKeyJustReleased(KeyCode key) { return GetInput().IsKeyJustReleased(key); }

}  //namespace engine
