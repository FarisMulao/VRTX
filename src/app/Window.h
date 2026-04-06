#pragma once
#include <string>
#include <windows.h>
#include <functional>

class Window {
public:
  Window(int width, int height, const std::string &title);
  ~Window();

  bool ProcessMessages();
  HWND GetHWND() const { return m_hwnd; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  void SetTitle(const std::string &title);

  // Resize callback — set by Application so D3D12 swap chain can resize
  using ResizeCallback = std::function<void(int, int)>;
  void SetResizeCallback(ResizeCallback cb) { m_resizeCallback = cb; }

private:
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);

  HWND m_hwnd;
  int m_width;
  int m_height;
  ResizeCallback m_resizeCallback;

  friend LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
};
