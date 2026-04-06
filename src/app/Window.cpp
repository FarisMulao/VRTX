#include "Window.h"
#include "../input/Input.h"
#include "imgui.h"
#include "imgui_impl_win32.h"

static Window *s_windowInstance = nullptr;

Window::Window(int width, int height, const std::string &title)
    : m_width(width), m_height(height), m_hwnd(nullptr) {
  const char *CLASS_NAME = "VRTXWindowClass";

  WNDCLASSA wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = CLASS_NAME;

  RegisterClassA(&wc);

  DWORD style = WS_OVERLAPPEDWINDOW;
  RECT rect = {0, 0, width, height};
  AdjustWindowRectEx(&rect, style, FALSE, 0);

  m_hwnd = CreateWindowExA(0, CLASS_NAME, title.c_str(), style, CW_USEDEFAULT,
                           CW_USEDEFAULT, rect.right - rect.left,
                           rect.bottom - rect.top, nullptr, nullptr,
                           wc.hInstance, nullptr);

  s_windowInstance = this;

  ShowWindow(m_hwnd, SW_SHOW);
  UpdateWindow(m_hwnd);
}

Window::~Window() {
  s_windowInstance = nullptr;
  if (m_hwnd) {
    DestroyWindow(m_hwnd);
  }
}

bool Window::ProcessMessages() {
  MSG msg = {};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      return false;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return true;
}

void Window::SetTitle(const std::string &title) {
  SetWindowTextA(m_hwnd, title.c_str());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
    return true;

  switch (uMsg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_SIZE: {
    if (s_windowInstance && wParam != SIZE_MINIMIZED) {
      int newW = (int)LOWORD(lParam);
      int newH = (int)HIWORD(lParam);
      if (newW > 0 && newH > 0) {
        s_windowInstance->m_width = newW;
        s_windowInstance->m_height = newH;
        if (s_windowInstance->m_resizeCallback) {
          s_windowInstance->m_resizeCallback(newW, newH);
        }
      }
    }
  } break;

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    Input::SetKeyDown((int)wParam, true);
    break;
  case WM_KEYUP:
  case WM_SYSKEYUP:
    Input::SetKeyDown((int)wParam, false);
    break;

  case WM_LBUTTONDOWN:
    Input::SetMouseDown(0, true);
    break;
  case WM_LBUTTONUP:
    Input::SetMouseDown(0, false);
    break;
  case WM_RBUTTONDOWN:
    Input::SetMouseDown(1, true);
    break;
  case WM_RBUTTONUP:
    Input::SetMouseDown(1, false);
    break;
  case WM_MBUTTONDOWN:
    Input::SetMouseDown(2, true);
    break;
  case WM_MBUTTONUP:
    Input::SetMouseDown(2, false);
    break;

  case WM_MOUSEMOVE:
    Input::SetMousePos((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
    break;

  case WM_MOUSEWHEEL:
    Input::SetMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
    break;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
