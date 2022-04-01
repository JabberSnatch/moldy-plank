#include "win32_context.hpp"

#include <iostream>

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
