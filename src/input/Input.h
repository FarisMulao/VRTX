#pragma once

class Input {
public:
  static void Init();
  static void Update(); // Call at start of frame

  // Windows Virtual Key Codes
  static void SetKeyDown(int key, bool down);
  static bool IsKeyDown(int key);
  static bool IsKeyPressed(int key); // True only on first frame

  // 0=Left, 1=Right, 2=Middle
  static void SetMouseDown(int button, bool down);
  static bool IsMouseDown(int button);
  static bool IsMouseClicked(int button);

  static void SetMousePos(int x, int y);
  static int GetMouseX();
  static int GetMouseY();
  static int GetMouseDX();
  static int GetMouseDY();

  static void SetMouseScroll(int delta);
  static int GetMouseScrollDelta();

private:
  static bool s_keys[256];
  static bool s_prevKeys[256];
  static bool s_mouseBtns[3];
  static bool s_prevMouseBtns[3];
  static int s_mouseX, s_mouseY;
  static int s_prevMouseX, s_prevMouseY;
  static int s_scrollDelta;
};
