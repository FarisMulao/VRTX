#include "Application.h"
#include "../input/Input.h"
#include "../physics/Constraint.h"
#include "../physics/VerletBody.h"
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

Application::Application()
    : m_isRunning(true), m_frameCount(0),
      m_camera(Vec3(0.0f, 5.0f, -35.0f), Vec3(0.0f, -2.0f, 0.0f), 1.0472f,
               16.0f / 9.0f) {
  Input::Init();
  m_window = std::make_unique<Window>(1280, 720, "VRTX | FPS: 0");
}

Application::~Application() {}

void Application::OnResize(int w, int h) {
  // Defer resize to next frame (can't resize during WndProc safely)
  if (w > 0 && h > 0) {
    m_resizePending = true;
    m_pendingWidth = w;
    m_pendingHeight = h;
  }
}

void Application::Init() {
  OutputDebugStringA("[VRTX] Application::Init() started\n");
  auto now = std::chrono::high_resolution_clock::now();
  m_lastTime = now;
  m_fpsTime = now;

  m_d3dContext = std::make_shared<D3D12Context>(
      m_window->GetHWND(), m_window->GetWidth(), m_window->GetHeight());
  OutputDebugStringA("[VRTX] D3D12 Context Initialized successfully\n");

  m_renderer = std::make_unique<Renderer>(m_d3dContext);
  m_renderer->Init();

  m_camera = Camera(Vec3(0.0f, 5.0f, -35.0f), Vec3(0.0f, -2.0f, 0.0f), 1.0472f,
                    (float)m_window->GetWidth() / m_window->GetHeight());
  m_camera.SetFreeCam(true);

  // Register resize callback — fires from WM_SIZE in WndProc
  m_window->SetResizeCallback([this](int w, int h) { OnResize(w, h); });

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(m_window->GetHWND());

  ImGui_ImplDX12_InitInfo init_info = {};
  init_info.Device = m_d3dContext->GetDevice();
  init_info.CommandQueue = m_d3dContext->GetCommandQueue();
  init_info.NumFramesInFlight = 3;
  init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
  init_info.SrvDescriptorHeap = m_d3dContext->GetSrvHeap();
  init_info.LegacySingleSrvCpuDescriptor =
      m_d3dContext->GetSrvHeap()->GetCPUDescriptorHandleForHeapStart();
  init_info.LegacySingleSrvGpuDescriptor =
      m_d3dContext->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
  ImGui_ImplDX12_Init(&init_info);
}

void Application::Run() {
  Init();

  while (m_isRunning) {
    if (!m_window->ProcessMessages()) {
      m_isRunning = false;
      break;
    }

    // Handle deferred resize
    if (m_resizePending) {
      m_resizePending = false;
      m_d3dContext->Resize(m_pendingWidth, m_pendingHeight);
      m_camera.m_aspect = (float)m_pendingWidth / (float)m_pendingHeight;
    }

    Update();
    Render();
    CalculateFPS();

    Input::Update();
  }
}

static void ScreenToRay(int mx, int my, int w, int h, const Camera &cam,
                        Vec3 &outOrigin, Vec3 &outDir) {
  float ndcX = (2.0f * mx / w) - 1.0f;
  float ndcY = 1.0f - (2.0f * my / h);

  outOrigin = cam.m_position;

  float tanHalfFov = std::tan(cam.m_fov * 0.5f);
  float dirX = ndcX * cam.m_aspect * tanHalfFov;
  float dirY = ndcY * tanHalfFov;
  float dirZ = 1.0f;

  Mat4 view = cam.GetViewMatrix();
  Mat4 invView = view.Inverse();

  Vec3 right(invView.m[0][0], invView.m[0][1], invView.m[0][2]);
  Vec3 up(invView.m[1][0], invView.m[1][1], invView.m[1][2]);
  Vec3 forward(invView.m[2][0], invView.m[2][1], invView.m[2][2]);

  outDir = (right * dirX + up * dirY + forward * dirZ).normalized();
}

void Application::Update() {
  auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> dtDur = now - m_lastTime;
  m_lastTime = now;
  float dt = dtDur.count();

  if (Input::IsKeyPressed(VK_ESCAPE)) {
    m_isRunning = false;
  }

  bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
  bool imguiWantsKeyboard = ImGui::GetIO().WantCaptureKeyboard;

  RECT clientRect;
  GetClientRect(m_window->GetHWND(), &clientRect);
  int w = clientRect.right;
  int h = clientRect.bottom;
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  int mx = Input::GetMouseX();
  int my = Input::GetMouseY();
  int dx = Input::GetMouseDX();
  int dy = Input::GetMouseDY();

  if (!imguiWantsMouse && Input::IsMouseDown(1)) {
    m_camera.ProcessMouseLook((float)dx, (float)dy);

    POINT center = {w / 2, h / 2};
    ClientToScreen(m_window->GetHWND(), &center);
    SetCursorPos(center.x, center.y);
    Input::SetMousePos(w / 2, h / 2);
  }

  Vec3 rayOrigin, rayDir;
  ScreenToRay(mx, my, w, h, m_camera, rayOrigin, rayDir);

  m_hoveredOrb = -1;
  if (!imguiWantsMouse) {
    m_hoveredOrb = m_physicsWorld.RayPick(rayOrigin, rayDir);
  }

  for (auto &body : m_physicsWorld.m_bodies) {
    body.highlighted = 0;
  }
  if (m_bondSourceOrb >= 0 &&
      m_bondSourceOrb < (int)m_physicsWorld.m_bodies.size()) {
    m_physicsWorld.m_bodies[m_bondSourceOrb].highlighted = 1;
  }

  if (m_unbindSourceOrb >= 0 &&
      m_unbindSourceOrb < (int)m_physicsWorld.m_bodies.size()) {
    m_physicsWorld.m_bodies[m_unbindSourceOrb].highlighted = 1;
  }

  if ((m_bondSourceOrb >= 0 || m_unbindSourceOrb >= 0) && m_hoveredOrb >= 0 &&
      m_hoveredOrb != m_bondSourceOrb && m_hoveredOrb != m_unbindSourceOrb &&
      m_hoveredOrb < (int)m_physicsWorld.m_bodies.size()) {
    m_physicsWorld.m_bodies[m_hoveredOrb].highlighted = 2;
  }

  if (!imguiWantsMouse) {
    if (Input::IsMouseClicked(0)) {
      int hit = m_physicsWorld.RayPick(rayOrigin, rayDir);
      if (hit >= 0) {
        m_physicsWorld.m_grabbedOrb = hit;
        m_physicsWorld.m_bodies[hit].isPinned = 1;

        Vec3 camForward =
            (m_camera.m_target - m_camera.m_position).normalized();
        Vec3 orbPos = m_physicsWorld.m_bodies[hit].position;
        m_grabDepth = Vec3::dot(orbPos - m_camera.m_position, camForward);
        if (m_grabDepth < 0.5f)
          m_grabDepth = 0.5f;
      }
    }

    if (Input::IsMouseDown(0) && m_physicsWorld.m_grabbedOrb >= 0) {
      // Scroll wheel: adjust grab depth
      int scrollDelta = Input::GetMouseScrollDelta();
      if (scrollDelta != 0) {
        m_grabDepth += scrollDelta * 1.5f;
        if (m_grabDepth < 0.5f)
          m_grabDepth = 0.5f;
        if (m_grabDepth > 200.0f)
          m_grabDepth = 200.0f;
      }

      Vec3 camForward = (m_camera.m_target - m_camera.m_position).normalized();
      float denom = Vec3::dot(rayDir, camForward);
      if (denom > 0.0001f) {
        float t = m_grabDepth / denom;
        m_physicsWorld.m_grabTarget = rayOrigin + rayDir * t;
      }
    }

    if (!Input::IsMouseDown(0) && m_physicsWorld.m_grabbedOrb >= 0) {
      m_physicsWorld.m_bodies[m_physicsWorld.m_grabbedOrb].isPinned = 0;
      m_physicsWorld.m_grabbedOrb = -1;
    }
  }

  if (!imguiWantsKeyboard && Input::IsKeyPressed('E')) {
    m_unbindSourceOrb = -1;

    if (m_bondSourceOrb < 0) {
      if (m_hoveredOrb >= 0) {
        m_bondSourceOrb = m_hoveredOrb;
        OutputDebugStringA("[VRTX] Bond source selected\n");
      }
    } else {
      if (m_hoveredOrb >= 0 && m_hoveredOrb != m_bondSourceOrb) {
        Vec3 posA = m_physicsWorld.m_bodies[m_bondSourceOrb].position;
        Vec3 posB = m_physicsWorld.m_bodies[m_hoveredOrb].position;
        float dist = (posB - posA).length();

        Constraint c = {};
        c.bodyA = (uint32_t)m_bondSourceOrb;
        c.bodyB = (uint32_t)m_hoveredOrb;
        c.restLength = dist;
        c.active = 1;
        m_physicsWorld.m_constraints.push_back(c);

        OutputDebugStringA("[VRTX] Bond created!\n");
      }
      m_bondSourceOrb = -1;
    }
  }

  if (!imguiWantsKeyboard && Input::IsKeyPressed('Q')) {
    m_bondSourceOrb = -1;

    if (m_unbindSourceOrb < 0) {
      if (m_hoveredOrb >= 0) {
        m_unbindSourceOrb = m_hoveredOrb;
        OutputDebugStringA("[VRTX] Unbind source selected\n");
      }
    } else {
      if (m_hoveredOrb >= 0 && m_hoveredOrb != m_unbindSourceOrb) {
        uint32_t a = (uint32_t)m_unbindSourceOrb;
        uint32_t b = (uint32_t)m_hoveredOrb;
        bool found = false;
        for (auto &c : m_physicsWorld.m_constraints) {
          if (!c.active)
            continue;
          if ((c.bodyA == a && c.bodyB == b) ||
              (c.bodyA == b && c.bodyB == a)) {
            c.active = 0;
            found = true;
          }
        }
        if (found) {
          OutputDebugStringA("[VRTX] Bond removed!\n");
        }
      }
      m_unbindSourceOrb = -1;
    }
  }

  bool fwd = Input::IsKeyDown('W');
  bool back = Input::IsKeyDown('S');
  bool left = Input::IsKeyDown('A');
  bool right = Input::IsKeyDown('D');
  bool up = Input::IsKeyDown(VK_SPACE);
  bool down = Input::IsKeyDown(VK_SHIFT);
  m_camera.ProcessMovement(dt, fwd, back, left, right, up, down);

  m_physicsWorld.Update(dt);
  m_camera.Update(dt);

  m_renderer->UpdateInstances(m_physicsWorld.m_bodies);
  m_renderer->UpdateBonds(m_physicsWorld.m_constraints,
                          m_physicsWorld.m_bodies);
}

void Application::Render() {
  if (!m_d3dContext)
    return;

  m_d3dContext->ResetCommandList();
  auto cmdList = m_d3dContext->GetCommandList();

  auto rtv = m_d3dContext->GetCurrentBackBufferView();
  auto currentBuffer = m_d3dContext->GetCurrentBackBuffer();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = currentBuffer;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  cmdList->ResourceBarrier(1, &barrier);

  const float clearColor[] = {0.06f, 0.06f, 0.1f, 1.0f};
  cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

  auto dsv = m_d3dContext->GetDepthStencilView();
  cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0,
                                 nullptr);
  cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

  int vpW = m_window->GetWidth();
  int vpH = m_window->GetHeight();
  D3D12_VIEWPORT vp = {0.0f, 0.0f, (float)vpW, (float)vpH, 0.0f, 1.0f};
  D3D12_RECT sr = {0, 0, vpW, vpH};
  cmdList->RSSetViewports(1, &vp);
  cmdList->RSSetScissorRects(1, &sr);

  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("VRTX Controls");

  // === Container selection ===
  int prevContainer = m_selectedContainer;
  ImGui::RadioButton("Sphere Container", &m_selectedContainer, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Cube Container", &m_selectedContainer, 1);

  if (prevContainer != m_selectedContainer) {
    m_d3dContext->Flush();
    if (m_selectedContainer == 0) {
      m_physicsWorld.m_containerType = ContainerType::Sphere;
      m_renderer->RebuildContainerSphere(m_physicsWorld.m_containerRadius);
    } else {
      m_physicsWorld.m_containerType = ContainerType::Cube;
      m_renderer->RebuildContainerCube(m_physicsWorld.m_containerHalfSize);
    }
  }

  ImGui::Separator();

  // === Physics toggles ===
  ImGui::Checkbox("Freeze Time", &m_physicsWorld.m_isPaused);

  bool gravEnabled = m_physicsWorld.m_gravityEnabled;
  if (ImGui::Checkbox("Gravity", &gravEnabled)) {
    m_physicsWorld.m_gravityEnabled = gravEnabled;
  }

  ImGui::Separator();

  // === Bond type ===
  int bondTypeInt = (int)m_physicsWorld.m_bondType;
  ImGui::Text("Bond Type:");
  ImGui::SameLine();
  ImGui::RadioButton("Tether", &bondTypeInt, 0);
  ImGui::SameLine();
  ImGui::RadioButton("Rigid", &bondTypeInt, 1);
  m_physicsWorld.m_bondType = (PhysicsWorld::BondType)bondTypeInt;

  // Bond/unbind mode status
  if (m_bondSourceOrb >= 0) {
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.6f, 1.0f),
                       "Bond: Source #%d selected. Press E on target.",
                       m_bondSourceOrb);
  } else if (m_unbindSourceOrb >= 0) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                       "Unbind: Source #%d selected. Press Q on target.",
                       m_unbindSourceOrb);
  } else {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "E: Bond | Q: Unbind");
  }

  ImGui::Separator();

  // === Spawn parameters ===
  ImGui::InputInt("Spawn Count", &m_spawnCount);
  if (m_spawnCount < 1)
    m_spawnCount = 1;
  if (m_spawnCount > 5000)
    m_spawnCount = 5000;

  ImGui::SliderFloat("Spawn Radius", &m_spawnRadius, 0.05f, 1.0f, "%.2f");
  ImGui::SliderFloat("Spawn Mass", &m_spawnMass, 0.1f, 100.0f, "%.1f",
                     ImGuiSliderFlags_Logarithmic);

  // Mass color preview
  float pr, pg, pb;
  PhysicsWorld::MassToColor(m_spawnMass, 0.1f, 100.0f, pr, pg, pb);
  ImGui::ColorButton("Mass Color Preview", ImVec4(pr, pg, pb, 1.0f),
                     ImGuiColorEditFlags_NoTooltip, ImVec2(20, 20));
  ImGui::SameLine();
  ImGui::Text("Preview");

  if (ImGui::Button("Spawn Orbs")) {
    for (int i = 0; i < m_spawnCount; i++) {
      float x = (rand() % 100 - 50) / 10.0f;
      float z = (rand() % 100 - 50) / 10.0f;
      float y = 8.0f + (rand() % 20) / 10.0f;
      m_physicsWorld.SpawnOrb(Vec3(x, y, z), m_spawnRadius, m_spawnMass);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear All")) {
    m_physicsWorld.m_bodies.clear();
    m_physicsWorld.m_constraints.clear();
    m_bondSourceOrb = -1;
    m_unbindSourceOrb = -1;
    m_hoveredOrb = -1;
  }

  ImGui::Separator();

  // === Stats ===
  ImGui::Text("Total Orbs: %zu", m_physicsWorld.m_bodies.size());
  ImGui::Text("Total Bonds: %zu", m_physicsWorld.m_constraints.size());

  ImGui::End();

  m_renderer->Render(m_camera);

  ImGui::Render();

  ID3D12DescriptorHeap *descriptorHeaps[] = {m_d3dContext->GetSrvHeap()};
  cmdList->SetDescriptorHeaps(1, descriptorHeaps);
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  cmdList->ResourceBarrier(1, &barrier);

  m_d3dContext->ExecuteCommandList();
  m_d3dContext->Present();
}

void Application::CalculateFPS() {
  m_frameCount++;
  auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = now - m_fpsTime;

  if (diff.count() >= 1.0) {
    std::string title = "VRTX | FPS: " + std::to_string(m_frameCount);
    m_window->SetTitle(title);
    m_frameCount = 0;
    m_fpsTime = now;
  }
}
