#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <cmath>
#include <vector>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct RGBColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Point
{
    int a;
    int b;
};

RGBColor GetColorAtPos(int x, int y)
{
    static HDC dc = GetDC(NULL);
    COLORREF c_Pixel = GetPixel(dc, x, y);

    return RGBColor{(uint8_t)GetRValue(c_Pixel), (uint8_t)GetGValue(c_Pixel), (uint8_t)GetBValue(c_Pixel)};
}

// uses <cmath>
Point GetPxAtAngle(Point origin, float radius, float angle)
{
    float f_AngleRadians = angle * 3.14159 / 180.0f;
    Point p_NewPoint;
    p_NewPoint.a = origin.a + (radius * std::sin(f_AngleRadians));
    p_NewPoint.b = origin.b + (radius * std::cos(f_AngleRadians));

    return p_NewPoint;
}

// uses <vector>
std::vector<Point> GetNPointsInCircle(Point origin, float radius, int n)
{
    std::vector<Point> v_Points;

    for (int i = 0; i < n; i++)
    {
        float f_Angle = (360.0f / n) * i;
        v_Points.push_back(GetPxAtAngle(origin, radius, f_Angle));
    }

    return v_Points;
}

// Main code
int main(int, char**)
{
    //ImGui_ImplWin32_EnableDpiAwareness();

    // Create application window
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Example", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Border] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.55, 0, 0, 1);
    style.Colors[ImGuiCol_Button] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35, 0, 0, 1);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.55, 0, 0, 1);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.85, 0, 0, 1);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    float screen_w = GetSystemMetrics(SM_CXSCREEN);
    float screen_h = GetSystemMetrics(SM_CYSCREEN);

    bool sc_is_active = false;
    bool pause_skill_checker = false;
    bool opened = true;
    bool done = false;

    std::pair<float, float> px_pos[16] = {
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 - 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 - 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 - 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 - 85),
        std::make_pair((screen_w * 0.5) + 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) + 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) + 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) + 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 + 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 + 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 + 85),
        std::make_pair((screen_w * 0.5), (screen_h * 0.5) * 0.972222222 + 85),
        std::make_pair((screen_w * 0.5) - 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) - 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) - 85, (screen_h * 0.5) * 0.972222222),
        std::make_pair((screen_w * 0.5) - 85, (screen_h * 0.5) * 0.972222222),
    };

    ImGui::SetNextWindowPos(ImVec2(5, 5));
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ImGui Window
        ImGui::SetNextWindowSize(ImVec2(screen_w / 8, screen_h / 8));
        if (ImGui::Begin("DBD Skill Checker", & opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::Checkbox("Activate Skill Checker", &sc_is_active);
            ImGui::Checkbox("Enable pause hotkey", &pause_skill_checker);
            if (ImGui::Button("Set pause hotkey")) {
                //code
            }

        
        } ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        // Executes while Skill Checker is active

        /*
        if (sc_is_active) {
            HDC dng = GetDC(NULL);

            // Checks if pause hotkey isn't pressed to determine if operation should be paused
            if (!pause_skill_checker) {
                for (int i = 0; i < 16; i++) {
                    COLORREF c = GetPixel(dng, std::get<0>(px_pos[i]), std::get<1>(px_pos[i]));
                    SetPixel(dng, std::get<0>(px_pos[i]), std::get<1>(px_pos[i]), RGB(154, 255, 0));
                    std::cout << "(" << (int)GetRValue(c) << ", ";
                    std::cout << (int)GetGValue(c) << ", ";
                    std::cout << (int)GetBValue(c) << ")" << std::endl;
                }
            }
        }
        */
        static HDC dng = GetDC(NULL);
        static std::vector<Point> circle_points = GetNPointsInCircle(Point{500, 500}, 50, 64);
        for (Point p : circle_points)
        {
            SetPixel(dng, p.a, p.b, RGB(255, 0, 255));
        }

        SetPixel(dng, 500, 500, RGB(255, 255, 255));


    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
