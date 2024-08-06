#include "win32_context.hpp"

#include <array>
#include <iostream>

#include <windowsx.h>

#undef interface

namespace bstk {

std::unique_ptr<OSContext> CreateContext() { return std::unique_ptr<OSContext>(new Win32Context()); }

} // namespace bstk

struct Win32ModuleInfo
{
    HMODULE handle;
    FILETIME timestamp;
    uint32_t load_index;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Win32Context* context = (Win32Context*)GetPropA(hWnd, "Win32Context");

    LRESULT result = 0;

    switch (uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        result = 1;
        break;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (context)
        {
            auto pWindow = context->windows.find(hWnd);
            if (pWindow != context->windows.end())
            {
                pWindow->second.size[0] = width;
                pWindow->second.size[1] = height;
            }
        }
    } break;

    default:
        result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return result;
}

bstk::OSWindow Win32Context::CreateWindow()
{
    constexpr uint32_t kWidth = 1280;
    constexpr uint32_t kHeight = 720;

    bstk::OSWindow output = {};

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
        return output;

    DWORD styleex = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW
        | WS_VISIBLE
        | WS_SYSMENU
        | WS_MINIMIZEBOX
        | WS_MAXIMIZEBOX
        | WS_THICKFRAME;
    RECT canvas_rect = { 0, 0, kWidth, kHeight };
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
        return output;

    SetPropA(hwnd, "Win32Context", this);

    output.hinstance = (uint64_t)hinstance;
    output.hwindow = (uint64_t)hwnd;
    output.size[0] = kWidth;
    output.size[1] = kHeight;

    windows.emplace(std::make_pair(hwnd, output));

    return output;
}

bool Win32Context::PumpEvents(bstk::OSWindow& _window, iotk::input_t &_state)
{
    _window = windows[(HWND)_window.hwindow];

    static std::array<iotk::eKey, 256> key_map = [](){
        std::array<iotk::eKey, 256> output{};
        output[(std::uint8_t)(VK_TAB & 0xff)] = iotk::eKey::kTab;
        output[(std::uint8_t)(VK_LEFT & 0xff)] = iotk::eKey::kLeft;
        output[(std::uint8_t)(VK_RIGHT & 0xff)] = iotk::eKey::kRight;
        output[(std::uint8_t)(VK_UP & 0xff)] = iotk::eKey::kUp;
        output[(std::uint8_t)(VK_DOWN & 0xff)] = iotk::eKey::kDown;
        output[(std::uint8_t)(VK_PRIOR & 0xff)] = iotk::eKey::kPageUp;
        output[(std::uint8_t)(VK_NEXT & 0xff)] = iotk::eKey::kPageDown;
        output[(std::uint8_t)(VK_HOME & 0xff)] = iotk::eKey::kHome;
        output[(std::uint8_t)(VK_END & 0xff)] = iotk::eKey::kEnd;
        output[(std::uint8_t)(VK_INSERT & 0xff)] = iotk::eKey::kInsert;
        output[(std::uint8_t)(VK_DELETE & 0xff)] = iotk::eKey::kDelete;
        output[(std::uint8_t)(VK_BACK & 0xff)] = iotk::eKey::kBackspace;
        output[(std::uint8_t)(VK_RETURN & 0xff)] = iotk::eKey::kEnter;
        output[(std::uint8_t)(VK_ESCAPE & 0xff)] = iotk::eKey::kEscape;
        return output;
    }();

    MSG msg{ 0 };
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_QUIT:
            windows.erase((HWND)_window.hwindow);
            return false;

        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down |= iotk::kLeftBtn;
        } break;
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down |= iotk::kRightBtn;
        } break;
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down |= iotk::kMiddleBtn;
        } break;

        case WM_LBUTTONUP:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down &= ~iotk::kLeftBtn;
        } break;
        case WM_RBUTTONUP:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down &= ~iotk::kRightBtn;
        } break;
        case WM_MBUTTONUP:
        {
            if (::GetCapture() == NULL)
                ::SetCapture((HWND)_window.hwindow);
            _state.button_down &= ~iotk::kMiddleBtn;
        } break;

        case WM_MOUSEMOVE:
        {
            _state.cursor[0] = GET_X_LPARAM(msg.lParam);
            _state.cursor[1] = GET_Y_LPARAM(msg.lParam);
        } break;

        case WM_MOUSEWHEEL:
        {
            _state.wheel_delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            std::uint32_t km = 0u
                | (((::GetKeyState(VK_CONTROL) & 0x8000) != 0) ? iotk::fKeyMod::kCtrl : 0u)
                | (((::GetKeyState(VK_SHIFT) & 0x8000) != 0) ? iotk::fKeyMod::kShift : 0u)
                | (((::GetKeyState(VK_MENU) & 0x8000) != 0) ? iotk::fKeyMod::kAlt : 0u);
            _state.mod_down = km;

            if (msg.wParam < 256)
            {
                std::uint32_t key = msg.wParam;
                if (key < iotk::eKey::kASCIIBegin || key >= iotk::eKey::kASCIIEnd)
                    key = key_map[key & 0xff];
                else
                    key = (std::uint32_t)std::tolower((unsigned char)key);

                _state.key_down[key] = (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN);
            }
        }

        default:
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return true;
}

bstk::EngineModule Win32Context::EngineLoad(std::string const& _path, std::string const& _lockfile)
{
    bstk::EngineModule module{
        _path,
        _lockfile,
        new Win32ModuleInfo{},
        bstk::EngineInterface{}
    };

    EngineReloadModule(module);
    return module;
}

void Win32Context::EngineRelease(bstk::EngineModule& _module)
{
    Win32ModuleInfo& moduleInfo = *(Win32ModuleInfo*)_module.platform_data;
    FreeLibrary(moduleInfo.handle);
    delete _module.platform_data;
}

bool Win32Context::EngineReloadRequired(bstk::EngineModule const& _module)
{
    Win32ModuleInfo& moduleInfo = *(Win32ModuleInfo*)_module.platform_data;

    FILETIME lastWriteTime{};
    WIN32_FILE_ATTRIBUTE_DATA data{};
    GetFileAttributesExA(_module.path.c_str(), GetFileExInfoStandard, &data);
    lastWriteTime = data.ftLastWriteTime;
    return (CompareFileTime(&lastWriteTime, &moduleInfo.timestamp) != 0);
}

void Win32Context::EngineReloadModule(bstk::EngineModule& _module)
{
    Win32ModuleInfo& moduleInfo = *(Win32ModuleInfo*)_module.platform_data;
    FreeLibrary(moduleInfo.handle);

    bool hasLockFile = (_module.lockfile != "");
    WIN32_FILE_ATTRIBUTE_DATA dummy;
    for (uint32_t readIndex = 0u; readIndex < 128; ++readIndex)
    {
        if (!hasLockFile
            || !GetFileAttributesExA(_module.lockfile.c_str(), GetFileExInfoStandard, &dummy))
        {
            FILETIME lastWriteTime{};
            WIN32_FILE_ATTRIBUTE_DATA data{};
            GetFileAttributesExA(_module.path.c_str(), GetFileExInfoStandard, &data);
            lastWriteTime = data.ftLastWriteTime;

            HANDLE file = CreateFileA(_module.path.c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      NULL,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

            std::string altpath = _module.path;
            altpath[altpath.size() - 1] = '_';
            altpath += std::to_string(moduleInfo.load_index++);
            moduleInfo.load_index &= 0xff;

            for (uint32_t copyIndex = 0u; copyIndex < 128u; ++copyIndex)
                if (CopyFileA(_module.path.c_str(), altpath.c_str(), FALSE))
                    break;

            CloseHandle(file);

            HMODULE module = LoadLibraryA(TEXT(altpath.c_str()));
            if (module == NULL)
                std::cout << "Module not found" << std::endl;

            bstk::EngineInterface interface{
                (bstk::EngineInterface::Create_t)GetProcAddress(module, "ModuleInterface_Create"),
                (bstk::EngineInterface::Shutdown_t)GetProcAddress(module, "ModuleInterface_Shutdown"),
                (bstk::EngineInterface::Reload_t)GetProcAddress(module, "ModuleInterface_Reload"),
                (bstk::EngineInterface::LogicUpdate_t)GetProcAddress(module, "ModuleInterface_LogicUpdate"),
                (bstk::EngineInterface::DrawFrame_t)GetProcAddress(module, "ModuleInterface_DrawFrame"),
            };

            _module.interface = interface;
            moduleInfo.timestamp = lastWriteTime;
            moduleInfo.handle = module;
            std::cout << "load attempt " << readIndex << std::endl;
            break;
        }

        Sleep(100);
    }
}
