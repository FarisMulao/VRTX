#include "Application.h"
#include "../input/Input.h"
#include "../physics/VerletBody.h"
#include "../physics/Constraint.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

Application::Application() 
    : m_isRunning(true), m_frameCount(0), 
      m_camera(Vec3(0.0f, 5.0f, -35.0f), Vec3(0.0f, -2.0f, 0.0f), 1.0472f, 16.0f/9.0f)
{
    Input::Init();
    m_window = std::make_unique<Window>(1280, 720, "VRTX | FPS: 0");
}

Application::~Application() {
}

void Application::Init() {
    OutputDebugStringA("[VRTX] Application::Init() started\n");
    auto now = std::chrono::high_resolution_clock::now();
    m_lastTime = now;
    m_fpsTime = now;
    
    m_d3dContext = std::make_shared<D3D12Context>(m_window->GetHWND(), m_window->GetWidth(), m_window->GetHeight());
    OutputDebugStringA("[VRTX] D3D12 Context Initialized successfully\n");

    m_renderer = std::make_unique<Renderer>(m_d3dContext);
    m_renderer->Init();

    m_camera = Camera(Vec3(0.0f, 5.0f, -35.0f), Vec3(0.0f, -2.0f, 0.0f), 1.0472f, (float)m_window->GetWidth() / m_window->GetHeight());

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
    init_info.LegacySingleSrvCpuDescriptor = m_d3dContext->GetSrvHeap()->GetCPUDescriptorHandleForHeapStart();
    init_info.LegacySingleSrvGpuDescriptor = m_d3dContext->GetSrvHeap()->GetGPUDescriptorHandleForHeapStart();
    ImGui_ImplDX12_Init(&init_info);
}

void Application::Run() {
    Init();

    while (m_isRunning) {
        if (!m_window->ProcessMessages()) {
            m_isRunning = false;
            break;
        }

        Update();
        Render();
        CalculateFPS();
        
        Input::Update();
    }
}

// Build a ray from screen pixel coordinates through the camera
static void ScreenToRay(int mx, int my, int w, int h, const Camera& cam, Vec3& outOrigin, Vec3& outDir) {
    // Normalized device coordinates [-1, 1]
    float ndcX = (2.0f * mx / w) - 1.0f;
    float ndcY = 1.0f - (2.0f * my / h); // Flip Y
    
    // Unproject using inverse VP
    Mat4 view = cam.GetViewMatrix();
    Mat4 proj = cam.GetProjectionMatrix();
    Mat4 vp = view * proj;
    Mat4 invVP = vp.Inverse();
    
    // Near and far points in NDC
    outOrigin = cam.m_position;
    
    
    float fovY = cam.m_fov;
    float aspect = cam.m_aspect;
    float tanHalfFov = std::tan(fovY * 0.5f);
    
    // Camera local direction
    float dirX = ndcX * aspect * tanHalfFov;
    float dirY = ndcY * tanHalfFov;
    float dirZ = 1.0f; // Forward in LH
    
    
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
    
    // V key: toggle free cam
    if (Input::IsKeyPressed('V')) {
        m_camera.SetFreeCam(!m_camera.IsFreeCam());
    }
    
    // P key: toggle pause (freeze time)
    if (Input::IsKeyPressed('P')) {
        m_physicsWorld.m_isPaused = !m_physicsWorld.m_isPaused;
    }
    
    // G key: toggle gravity
    if (Input::IsKeyPressed('G')) {
        m_physicsWorld.m_gravity = m_physicsWorld.m_gravity * -1.0f;
        OutputDebugStringA("[VRTX] Gravity toggled!\n");
    }
    
    bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;
    
    if (!imguiWantsMouse) {
        int mx = Input::GetMouseX();
        int my = Input::GetMouseY();
        int dx = Input::GetMouseDX();
        int dy = Input::GetMouseDY();
        int w = m_window->GetWidth();
        int h = m_window->GetHeight();
        
        // Free cam mouse look
        if (m_camera.IsFreeCam()) {
            if (Input::IsMouseDown(1)) { // Right click to look around
                m_camera.ProcessMouseLook((float)dx, (float)dy);
                
                POINT center = { w / 2, h / 2 };
                ClientToScreen(m_window->GetHWND(), &center);
                SetCursorPos(center.x, center.y);
                Input::SetMousePos(w / 2, h / 2); 
            }
        }
        
        Vec3 rayOrigin, rayDir;
        ScreenToRay(mx, my, w, h, m_camera, rayOrigin, rayDir);
        
        if (Input::IsMouseClicked(0)) {
            int hit = m_physicsWorld.RayPick(rayOrigin, rayDir);
            if (hit >= 0) {
                m_physicsWorld.m_grabbedOrb = hit;
                m_physicsWorld.m_bodies[hit].isPinned = 1;
            }
        }
        
        if (Input::IsMouseDown(0) && m_physicsWorld.m_grabbedOrb >= 0) {
            int gi = m_physicsWorld.m_grabbedOrb;
            Vec3 orbPos = m_physicsWorld.m_bodies[gi].position;
            
            Vec3 camForward = (m_camera.m_target - m_camera.m_position).normalized();
            float orbDepth = Vec3::dot(orbPos - m_camera.m_position, camForward);
            if (orbDepth < 0.1f) orbDepth = 0.1f;
            
            float t = orbDepth / Vec3::dot(rayDir, camForward);
            m_physicsWorld.m_grabTarget = rayOrigin + rayDir * t;
        }
        
        if (!Input::IsMouseDown(0) && m_physicsWorld.m_grabbedOrb >= 0) {
            m_physicsWorld.m_bodies[m_physicsWorld.m_grabbedOrb].isPinned = 0;
            m_physicsWorld.m_grabbedOrb = -1;
        }
        
        // Right click: bond creation (ONLY if freecam is off)
        if (!m_camera.IsFreeCam()) {
            if (Input::IsMouseClicked(1)) {
                int hit = m_physicsWorld.RayPick(rayOrigin, rayDir);
                if (hit >= 0) {
                    m_bondStartOrb = hit;
                }
            }
            
            if (!Input::IsMouseDown(1) && m_bondStartOrb >= 0) {
                int hit = m_physicsWorld.RayPick(rayOrigin, rayDir);
                if (hit >= 0 && hit != m_bondStartOrb) {
                    Vec3 posA = m_physicsWorld.m_bodies[m_bondStartOrb].position;
                    Vec3 posB = m_physicsWorld.m_bodies[hit].position;
                    float dist = (posB - posA).length();
                    
                    Constraint c = {};
                    c.bodyA = (uint32_t)m_bondStartOrb;
                    c.bodyB = (uint32_t)hit;
                    c.restLength = dist;
                    c.active = 1;
                    m_physicsWorld.m_constraints.push_back(c);
                    
                    OutputDebugStringA("[VRTX] Bond created!\n");
                }
                m_bondStartOrb = -1;
            }
        }
    }
    
    // Free cam movement
    if (m_camera.IsFreeCam()) {
        bool fwd = Input::IsKeyDown('W');
        bool back = Input::IsKeyDown('S');
        bool left = Input::IsKeyDown('A');
        bool right = Input::IsKeyDown('D');
        bool up = Input::IsKeyDown(VK_SPACE);
        bool down = Input::IsKeyDown(VK_SHIFT);
        m_camera.ProcessMovement(dt, fwd, back, left, right, up, down);
    }
    
    m_physicsWorld.Update(dt);
    m_camera.Update(dt);
    
    m_renderer->UpdateInstances(m_physicsWorld.m_bodies);
    m_renderer->UpdateBonds(m_physicsWorld.m_constraints, m_physicsWorld.m_bodies);
}

void Application::Render() {
    if (!m_d3dContext) return;
    
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
    
    const float clearColor[] = { 0.06f, 0.06f, 0.1f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    
    auto dsv = m_d3dContext->GetDepthStencilView();
    cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    
    D3D12_VIEWPORT vp = { 0.0f, 0.0f, (float)m_window->GetWidth(), (float)m_window->GetHeight(), 0.0f, 1.0f };
    D3D12_RECT sr = { 0, 0, m_window->GetWidth(), m_window->GetHeight() };
    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &sr);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("VRTX Controls");
    
    bool isFreeCam = m_camera.IsFreeCam();
    if (ImGui::Checkbox("Free Cam (V)", &isFreeCam)) {
        m_camera.SetFreeCam(isFreeCam);
    }
    
    ImGui::Separator();
    
    int prevContainer = m_selectedContainer;
    ImGui::RadioButton("Sphere Container", &m_selectedContainer, 0); ImGui::SameLine();
    ImGui::RadioButton("Cube Container", &m_selectedContainer, 1);
    
    if (prevContainer != m_selectedContainer) {
        if (m_selectedContainer == 0) {
            m_physicsWorld.m_containerType = ContainerType::Sphere;
            m_renderer->RebuildContainerSphere(m_physicsWorld.m_containerRadius);
        } else {
            m_physicsWorld.m_containerType = ContainerType::Cube;
            m_renderer->RebuildContainerCube(m_physicsWorld.m_containerHalfSize);
        }
    }
    
    ImGui::Separator();
    
    ImGui::Checkbox("Freeze Time (P)", &m_physicsWorld.m_isPaused);
    
    int bondTypeInt = (int)m_physicsWorld.m_bondType;
    ImGui::Text("Bond Type:"); ImGui::SameLine();
    ImGui::RadioButton("Tether", &bondTypeInt, 0); ImGui::SameLine();
    ImGui::RadioButton("Rod (Rigid)", &bondTypeInt, 1);
    m_physicsWorld.m_bondType = (PhysicsWorld::BondType)bondTypeInt;

    ImGui::Separator();

    ImGui::InputInt("Spawn Count", &m_spawnCount);
    if (m_spawnCount < 1) m_spawnCount = 1;
    if (m_spawnCount > 5000) m_spawnCount = 5000;
    
    if (ImGui::Button("Spawn Orbs")) {
        for (int i = 0; i < m_spawnCount; i++) {
            float x = (rand() % 100 - 50) / 10.0f;
            float z = (rand() % 100 - 50) / 10.0f;
            float y = 8.0f + (rand() % 20) / 10.0f;
            m_physicsWorld.SpawnOrb(Vec3(x, y, z), 0.3f);
        }
    }
    if (ImGui::Button("Toggle Gravity (G)")) {
        m_physicsWorld.m_gravity = m_physicsWorld.m_gravity * -1.0f;
    }
    if (ImGui::Button("Clear All")) {
        m_physicsWorld.m_bodies.clear();
        m_physicsWorld.m_constraints.clear();
    }
    ImGui::Separator();
    ImGui::Text("Total Orbs: %zu", m_physicsWorld.m_bodies.size());
    ImGui::Text("Total Bonds: %zu", m_physicsWorld.m_constraints.size());
    ImGui::Separator();
    ImGui::TextWrapped("LMB+Drag: Grab orb");
    if (m_camera.IsFreeCam()) {
        ImGui::TextWrapped("RMB+Drag: Look around");
        ImGui::TextWrapped("W/A/S/D/Space/Shift: Move");
    } else {
        ImGui::TextWrapped("RMB+Drag: Bond two orbs");
    }
    ImGui::TextWrapped("P: Freeze Time");
    ImGui::TextWrapped("G: Toggle gravity");
    ImGui::End();

    m_renderer->Render(m_camera);

    ImGui::Render();

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_d3dContext->GetSrvHeap() };
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
