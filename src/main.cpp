#include "app/Application.h"
#include <shellscalingapi.h>
#include <windows.h>

#pragma comment(lib, "Shcore.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  // Enable Per-Monitor DPI awareness so mouse coords and window sizes
  // are reported in physical pixels, not scaled logical pixels.
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  Application app;
  app.Run();
  return 0;
}
