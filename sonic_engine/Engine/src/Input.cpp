#include "Input.hpp"
#include "Bitmap.hpp"  //for Graphics::Instance()
#include <cstring>

namespace engine {

InputManager* InputManager::instance = nullptr;

InputManager& InputManager::Instance() {
    if (!instance) {
        instance = new InputManager();
    }
    return *instance;
}

KeyCode InputManager::SFMLKeyToKeyCode(sf::Keyboard::Key key) {
    using K = sf::Keyboard::Key;
    switch (key) {
        case K::Left:   return KeyCode::Left;
        case K::Right:  return KeyCode::Right;
        case K::Up:     return KeyCode::Up;
        case K::Down:   return KeyCode::Down;
        
        case K::A: return KeyCode::A;
        case K::B: return KeyCode::B;
        case K::C: return KeyCode::C;
        case K::D: return KeyCode::D;
        case K::E: return KeyCode::E;
        case K::F: return KeyCode::F;
        case K::G: return KeyCode::G;
        case K::H: return KeyCode::H;
        case K::I: return KeyCode::I;
        case K::J: return KeyCode::J;
        case K::K: return KeyCode::K;
        case K::L: return KeyCode::L;
        case K::M: return KeyCode::M;
        case K::N: return KeyCode::N;
        case K::O: return KeyCode::O;
        case K::P: return KeyCode::P;
        case K::Q: return KeyCode::Q;
        case K::R: return KeyCode::R;
        case K::S: return KeyCode::S;
        case K::T: return KeyCode::T;
        case K::U: return KeyCode::U;
        case K::V: return KeyCode::V;
        case K::W: return KeyCode::W;
        case K::X: return KeyCode::X;
        case K::Y: return KeyCode::Y;
        case K::Z: return KeyCode::Z;
        
        case K::Num0: return KeyCode::Num0;
        case K::Num1: return KeyCode::Num1;
        case K::Num2: return KeyCode::Num2;
        case K::Num3: return KeyCode::Num3;
        case K::Num4: return KeyCode::Num4;
        case K::Num5: return KeyCode::Num5;
        case K::Num6: return KeyCode::Num6;
        case K::Num7: return KeyCode::Num7;
        case K::Num8: return KeyCode::Num8;
        case K::Num9: return KeyCode::Num9;
        
        case K::F1:  return KeyCode::F1;
        case K::F2:  return KeyCode::F2;
        case K::F3:  return KeyCode::F3;
        case K::F4:  return KeyCode::F4;
        case K::F5:  return KeyCode::F5;
        case K::F6:  return KeyCode::F6;
        case K::F7:  return KeyCode::F7;
        case K::F8:  return KeyCode::F8;
        case K::F9:  return KeyCode::F9;
        case K::F10: return KeyCode::F10;
        case K::F11: return KeyCode::F11;
        case K::F12: return KeyCode::F12;
        
        case K::Space:     return KeyCode::Space;
        case K::Enter:     return KeyCode::Enter;
        case K::Escape:    return KeyCode::Escape;
        case K::Tab:       return KeyCode::Tab;
        case K::Backspace: return KeyCode::Backspace;
        case K::LShift:    return KeyCode::LShift;
        case K::RShift:    return KeyCode::RShift;
        case K::LControl:  return KeyCode::LCtrl;
        case K::RControl:  return KeyCode::RCtrl;
        case K::LAlt:      return KeyCode::LAlt;
        case K::RAlt:      return KeyCode::RAlt;
        case K::Home:      return KeyCode::Home;
        case K::End:       return KeyCode::End;
        case K::PageUp:    return KeyCode::PageUp;
        case K::PageDown:  return KeyCode::PageDown;
        case K::Insert:    return KeyCode::Insert;
        case K::Delete:    return KeyCode::Delete;
        case K::Add:       return KeyCode::Plus;
        case K::Subtract:  return KeyCode::Minus;
        case K::Equal:     return KeyCode::Equals;
        case K::Period:    return KeyCode::Period;
        case K::Comma:     return KeyCode::Comma;
        
        default: return KeyCode::Unknown;
    }
}

void InputManager::Poll() {
    auto& window = Graphics::Instance().GetWindow();
    
    Point prevMousePos = mousePos;
    sf::Event event;
    
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            windowClosed = true;
        }
        else if (event.type == sf::Event::KeyPressed) {
            KeyCode kc = SFMLKeyToKeyCode(event.key.code);
            if (kc != KeyCode::Unknown) {
                currentKeys[static_cast<int>(kc)] = true;
            }
        }
        else if (event.type == sf::Event::KeyReleased) {
            KeyCode kc = SFMLKeyToKeyCode(event.key.code);
            if (kc != KeyCode::Unknown) {
                currentKeys[static_cast<int>(kc)] = false;
            }
        }
        else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left)
                currentMouse[static_cast<int>(MouseButton::Left)] = true;
            else if (event.mouseButton.button == sf::Mouse::Right)
                currentMouse[static_cast<int>(MouseButton::Right)] = true;
            else if (event.mouseButton.button == sf::Mouse::Middle)
                currentMouse[static_cast<int>(MouseButton::Middle)] = true;
        }
        else if (event.type == sf::Event::MouseButtonReleased) {
            if (event.mouseButton.button == sf::Mouse::Left)
                currentMouse[static_cast<int>(MouseButton::Left)] = false;
            else if (event.mouseButton.button == sf::Mouse::Right)
                currentMouse[static_cast<int>(MouseButton::Right)] = false;
            else if (event.mouseButton.button == sf::Mouse::Middle)
                currentMouse[static_cast<int>(MouseButton::Middle)] = false;
        }
        else if (event.type == sf::Event::MouseMoved) {
            mousePos.x = event.mouseMove.x;
            mousePos.y = event.mouseMove.y;
        }
    }
    
    mouseDelta.x = mousePos.x - prevMousePos.x;
    mouseDelta.y = mousePos.y - prevMousePos.y;
}

void InputManager::Update() {
    std::memcpy(previousKeys, currentKeys, sizeof(currentKeys));
    std::memcpy(previousMouse, currentMouse, sizeof(currentMouse));
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx >= 0 && idx < static_cast<int>(KeyCode::KeyCount))
        return currentKeys[idx];
    return false;
}

bool InputManager::IsKeyJustPressed(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx >= 0 && idx < static_cast<int>(KeyCode::KeyCount))
        return currentKeys[idx] && !previousKeys[idx];
    return false;
}

bool InputManager::IsKeyJustReleased(KeyCode key) const {
    int idx = static_cast<int>(key);
    if (idx >= 0 && idx < static_cast<int>(KeyCode::KeyCount))
        return !currentKeys[idx] && previousKeys[idx];
    return false;
}

bool InputManager::IsMousePressed(MouseButton btn) const {
    int idx = static_cast<int>(btn);
    if (idx >= 0 && idx < static_cast<int>(MouseButton::ButtonCount))
        return currentMouse[idx];
    return false;
}

bool InputManager::IsMouseJustPressed(MouseButton btn) const {
    int idx = static_cast<int>(btn);
    if (idx >= 0 && idx < static_cast<int>(MouseButton::ButtonCount))
        return currentMouse[idx] && !previousMouse[idx];
    return false;
}

bool InputManager::IsMouseJustReleased(MouseButton btn) const {
    int idx = static_cast<int>(btn);
    if (idx >= 0 && idx < static_cast<int>(MouseButton::ButtonCount))
        return !currentMouse[idx] && previousMouse[idx];
    return false;
}

}  //namespace engine
