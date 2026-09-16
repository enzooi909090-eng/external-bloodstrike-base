#include "window.h"

static const wchar_t CLASS_NAME[] = L"ovl_fullscreen";

c_overlay::c_overlay(HWND target_window)
    : m_hWnd(nullptr), m_hInst(GetModuleHandle(nullptr)), current_width(0), current_height(0)
{
    running = false;
    next_frame_time = std::chrono::high_resolution_clock::now();
}

c_overlay::~c_overlay()
{
    destroy();
}

bool c_overlay::create()
{
    if (!init_window())
        return false;
    if (!init_d3d())
        return false;

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_pd3dDevice.Get(), m_pd3dDeviceContext.Get());

    ImGuiStyle &st = ImGui::GetStyle();
    ImGui::GetStyle().AntiAliasedLines = false;
    ImGui::GetStyle().AntiAliasedFill = false;
    ImGui::GetStyle().AntiAliasedLinesUseTex = false;

    st.WindowRounding = 0.f;
    st.ChildRounding = 0.f;
    st.FrameRounding = 0.f;
    st.PopupRounding = 0.f;
    st.ScrollbarRounding = 0.f;
    st.GrabRounding = 0.f;
    st.TabRounding = 0.f;
    st.WindowBorderSize = 1.f;
    st.ChildBorderSize = 1.f;
    st.PopupBorderSize = 1.f;
    st.FrameBorderSize = 1.f;
    st.ScrollbarSize = 14.f;
    st.GrabMinSize = 12.f;
    st.WindowPadding = ImVec2(8, 8);
    st.FramePadding = ImVec2(6, 4);
    st.ItemSpacing = ImVec2(8, 6);
    st.ItemInnerSpacing = ImVec2(6, 4);
    st.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    auto &c = st.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
    c[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.f);
    c[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.f);
    c[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
    c[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.f);
    c[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
    c[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.f);
    c[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.11f, 1.f);
    c[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
    c[ImGuiCol_TabActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.f);
    c[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
    c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.75f, 0.75f, 0.75f, 1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 1.f, 1.f, 1.f);
    c[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.85f, 0.85f, 1.f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.f);
    c[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.20f, 1.f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.f);

    running = true;
    return true;
}

void c_overlay::destroy()
{
    if (!m_hWnd)
        return;

    running = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_d3d();
    DestroyWindow(m_hWnd);
    UnregisterClassW(CLASS_NAME, m_hInst);

    m_hWnd = nullptr;
}

void c_overlay::start()
{
    auto interval = std::chrono::duration<double>(1.0 / desired_fps);
    auto now = std::chrono::high_resolution_clock::now();
    if (next_frame_time > now)
        std::this_thread::sleep_until(next_frame_time);
    next_frame_time += std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(interval);

    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT)
            running = false;
    }

    if (!running)
        return;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    UINT w = rc.right;
    UINT h = rc.bottom;

    if ((int)w != current_width || (int)h != current_height)
    {
        current_width = w;
        current_height = h;
        ImGui_ImplDX11_InvalidateDeviceObjects();
        cleanup_render_target();
        m_pSwapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        create_render_target();
        ImGui_ImplDX11_CreateDeviceObjects();
    }

    const float clr[4] = {0, 0, 0, 0};
    m_pd3dDeviceContext->OMSetRenderTargets(1, m_mainRenderTargetView.GetAddressOf(), nullptr);
    m_pd3dDeviceContext->ClearRenderTargetView(m_mainRenderTargetView.Get(), clr);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void c_overlay::end()
{
    if (!running)
        return;
    ImGui::Render();
    m_pd3dDeviceContext->OMSetRenderTargets(1, m_mainRenderTargetView.GetAddressOf(), nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_pSwapChain->Present(0, 0);
}

void c_overlay::set_click_through(bool enable)
{
    LONG ex = GetWindowLongW(m_hWnd, GWL_EXSTYLE);

    if (enable)
        ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    else
    {
        ex &= ~WS_EX_TRANSPARENT;
        ex &= ~WS_EX_NOACTIVATE;
        SetForegroundWindow(m_hWnd);
    }

    SetWindowLongW(m_hWnd, GWL_EXSTYLE, ex);
    SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

HWND c_overlay::window() const
{
    return m_hWnd;
}

bool c_overlay::init_window()
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);

    RECT r;
    GetClientRect(GetDesktopWindow(), &r);

    m_hWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME,
        L"",
        WS_POPUP,
        0, 0,
        r.right, r.bottom,
        nullptr, nullptr, m_hInst, this);

    if (!m_hWnd)
        return false;

    MARGINS m = {-1};
    DwmExtendFrameIntoClientArea(m_hWnd, &m);
    SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA);

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    return true;
}

bool c_overlay::init_d3d()
{
    RECT rc;
    GetClientRect(m_hWnd, &rc);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = rc.right;
    sd.BufferDesc.Height = rc.bottom;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL fl;
    D3D_FEATURE_LEVEL fls[] = {D3D_FEATURE_LEVEL_11_0};

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            fls, 1, D3D11_SDK_VERSION,
            &sd, &m_pSwapChain, &m_pd3dDevice,
            &fl, &m_pd3dDeviceContext)))
        return false;

    create_render_target();
    return true;
}

void c_overlay::cleanup_d3d()
{
    cleanup_render_target();
    if (m_pd3dDeviceContext)
        m_pd3dDeviceContext->ClearState();
    m_pSwapChain.Reset();
    m_pd3dDeviceContext.Reset();
    m_pd3dDevice.Reset();
}

void c_overlay::create_render_target()
{
    ComPtr<ID3D11Texture2D> bb;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&bb));
    m_pd3dDevice->CreateRenderTargetView(bb.Get(), nullptr, &m_mainRenderTargetView);
}

void c_overlay::cleanup_render_target()
{
    m_mainRenderTargetView.Reset();
}

LRESULT CALLBACK c_overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    c_overlay *o = (c_overlay *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    if (msg == WM_CREATE)
    {
        auto cs = (CREATESTRUCTW *)lParam;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return 0;
    }

    if (o)
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}