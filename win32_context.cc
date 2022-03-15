#include "win32_context.hpp"

#include <iostream>

namespace bstk {

std::unique_ptr<OSContext> CreateContext() { return std::unique_ptr<OSContext>(new Win32Context()); }

} // namespace bstk


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Win32Context* context = (Win32Context*)GetPropA(hWnd, "Win32Context");

    switch (uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        return 1;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

bstk::OSWindow Win32Context::CreateWindow()
{
    HINSTANCE hinstance = GetModuleHandleA(NULL);

    WNDCLASSEXA winclass{};
    winclass.cbSize = sizeof(WNDCLASSEXA);
    winclass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    winclass.lpfnWndProc = (WNDPROC)WndProc;
    winclass.cbClsExtra = 0;
    winclass.cbWndExtra = 0;
    winclass.hInstance = hinstance;
    winclass.hIcon = NULL;
    winclass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    winclass.hbrBackground = NULL;
    winclass.lpszMenuName = NULL;
    winclass.lpszClassName = "LoaderWindowClass";
    winclass.hIconSm = NULL;

    if (!RegisterClassExA(&winclass))
        return bstk::OSWindow{ 0ull, 0ull, iotk::input_t{} };

    DWORD styleex = WS_EX_APPWINDOW;
    DWORD style = WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
    RECT canvas_rect = { 0, 0, 1280, 720 };
    AdjustWindowRectEx(&canvas_rect, style, FALSE, styleex);

    HWND hwnd = CreateWindowExA(styleex,
                                "LoaderWindowClass",
                                "Loader",
                                style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                canvas_rect.right - canvas_rect.left,
                                canvas_rect.bottom - canvas_rect.top,
                                NULL,
                                NULL,
                                hinstance,
                                NULL);
    if (!hwnd)
        return bstk::OSWindow{ 0ull, 0ull, iotk::input_t{} };

    SetPropA(hwnd, "Win32Context", this);

    return bstk::OSWindow{ (uint64_t)hinstance, (uint64_t)hwnd, iotk::input_t{} };
}

bool Win32Context::PumpEvents(bstk::OSWindow& _window)
{
    MSG msg{ 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_QUIT:
            return false;

        default:
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return true;
}

bstk::EngineModule Win32Context::EngineLoad(char const* _path)
{
    return bstk::EngineModule{
        "",
        0ull,
        0ull,
        bstk::EngineInterface{
            bstk::StubEngine::Create,
            bstk::StubEngine::Shutdown,
            bstk::StubEngine::Reload,
            bstk::StubEngine::LogicUpdate,
            bstk::StubEngine::DrawFrame
        }
    };
}

bool Win32Context::EngineReloadRequired(bstk::EngineModule const& _module)
{
    return false;
}

void Win32Context::EngineReloadModule(bstk::EngineModule& _module)
{
}
