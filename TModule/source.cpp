#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <vector>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <thread>

bool draw = true;
bool loaded = false;

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND hwnd;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

void msg(const char* msg, const char* title) { MessageBoxA(0, msg, title, 0); }

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
    
typedef struct
{
    ImU32 R;
    ImU32 G;
    ImU32 B;
} RGB;

ImU32 Color(RGB color)
{
    return IM_COL32(color.R, color.G, color.B, 255);
}

RGB white = { 255,255,255 };

namespace Render
{
    void DrawRect(int x, int y, int w, int h, RGB color, int thickness)
    {
        ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), Color(color), 0, 0, thickness);
    }
    void Line(float x1, float y1, float x2, float y2, RGB color, float thickness)
    {
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), Color(color), thickness);
    }
    void Circle(float x, float y, float radius, RGB color)
    {
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), radius, Color(color), 0, 1);
    }
    void Text(float x, float y, RGB color, const char* text)
    {
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), Color(color), text);
    }
}

// Create transparent window
HWND CreateTransparentWindow(HINSTANCE hInstance) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"OverlayClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, 
        wc.lpszClassName, L"Overlay", WS_POPUP, 
        0, 0, screenWidth, screenHeight, nullptr, nullptr, hInstance, nullptr
    );

    // set the window to be transparent
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    {
        RECT client_area{};
        GetClientRect(hwnd, &client_area);

        RECT window_area{};
        GetWindowRect(hwnd, &window_area);

        POINT diff{};
        ClientToScreen(hwnd, &diff);

        const MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(hwnd, &margins);
    }

    return hwnd;
}
void InitImGui(HWND hwnd) {
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

void InitWin(HWND uHwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = uHwnd;
    sd.SampleDesc.Count = 1U;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, nullptr, &g_pd3dDeviceContext
    );

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();


}

void main_thread()
{
    // Create the transparent window
    HINSTANCE hInstance = GetModuleHandle(NULL);
    hwnd = CreateTransparentWindow(hInstance);

    if (!hwnd) {
        MessageBoxA(0, "Failed to create transparent window", "Error", 0);
        return;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);

    InitWin(hwnd);

    InitImGui(hwnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        /* rendering */
        Render::DrawRect(
            500,
            500,
            25,
            25,
            white,
            1.5
        );

        ImGui::Render();

        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Transparent background
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }
    ImGui::End();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

    DestroyWindow(hwnd);
}

extern "C" __declspec(dllexport) void Run() {
    if (!loaded)
    {
        std::thread(main_thread).detach();
        loaded = true;
    }
    else
    {
        msg("thread already created", "err");
    }
}

