#pragma once
#include "../graphics/Camera.h"
#include "../graphics/D3D12Context.h"
#include "../graphics/Renderer.h"
#include "../physics/PhysicsWorld.h"
#include "Window.h"
#include <chrono>
#include <memory>

class Application {
public:
  Application();
  ~Application();

  void Run();

private:
  void Init();
  void Update();
  void Render();
  void CalculateFPS();

  std::unique_ptr<Window> m_window;
  std::shared_ptr<D3D12Context> m_d3dContext;
  std::unique_ptr<Renderer> m_renderer;
  Camera m_camera;
  PhysicsWorld m_physicsWorld;

  bool m_isRunning;
  int m_bondStartOrb = -1;

  // UI state
  int m_spawnCount = 100;
  int m_selectedContainer = 0;

  std::chrono::high_resolution_clock::time_point m_lastTime;
  std::chrono::high_resolution_clock::time_point m_fpsTime;
  int m_frameCount;
};
