#pragma once
#include <string>
#include <windows.h>

class Window {
public:
  Window(int width, int height, const std::string &title);
  ~Window();

  bool ProcessMessages();
  HWND GetHWND() const { return m_hwnd; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  void SetTitle(const std::string &title);

private:
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);

  HWND m_hwnd;
  int m_width;
  int m_height;
};
