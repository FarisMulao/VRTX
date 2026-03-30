#include "Input.h"
#include <cstring>

bool Input::s_keys[256];
bool Input::s_prevKeys[256];
bool Input::s_mouseBtns[3];
bool Input::s_prevMouseBtns[3];
int Input::s_mouseX = 0;
int Input::s_mouseY = 0;
int Input::s_prevMouseX = 0;
int Input::s_prevMouseY = 0;
int Input::s_scrollDelta = 0;

void Input::Init() {
  std::memset(s_keys, 0, sizeof(s_keys));
  std::memset(s_prevKeys, 0, sizeof(s_prevKeys));
  std::memset(s_mouseBtns, 0, sizeof(s_mouseBtns));
  std::memset(s_prevMouseBtns, 0, sizeof(s_prevMouseBtns));
}

void Input::Update() {
  std::memcpy(s_prevKeys, s_keys, sizeof(s_keys));
  std::memcpy(s_prevMouseBtns, s_mouseBtns, sizeof(s_mouseBtns));
  s_prevMouseX = s_mouseX;
  s_prevMouseY = s_mouseY;
  s_scrollDelta = 0;
}

void Input::SetKeyDown(int key, bool down) {
  if (key >= 0 && key < 256)
    s_keys[key] = down;
}

bool Input::IsKeyDown(int key) {
  if (key >= 0 && key < 256)
    return s_keys[key];
  return false;
}

bool Input::IsKeyPressed(int key) {
  if (key >= 0 && key < 256)
    return s_keys[key] && !s_prevKeys[key];
  return false;
}

void Input::SetMouseDown(int button, bool down) {
  if (button >= 0 && button < 3)
    s_mouseBtns[button] = down;
}

bool Input::IsMouseDown(int button) {
  if (button >= 0 && button < 3)
    return s_mouseBtns[button];
  return false;
}

bool Input::IsMouseClicked(int button) {
  if (button >= 0 && button < 3)
    return s_mouseBtns[button] && !s_prevMouseBtns[button];
  return false;
}

void Input::SetMousePos(int x, int y) {
  s_mouseX = x;
  s_mouseY = y;
}

int Input::GetMouseX() { return s_mouseX; }
int Input::GetMouseY() { return s_mouseY; }

int Input::GetMouseDX() { return s_mouseX - s_prevMouseX; }
int Input::GetMouseDY() { return s_mouseY - s_prevMouseY; }

void Input::SetMouseScroll(int delta) { s_scrollDelta = delta; }
int Input::GetMouseScrollDelta() { return s_scrollDelta; }
