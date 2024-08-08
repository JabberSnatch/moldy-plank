#include "xlib_context.hpp"

#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

//#include <X11/extensions/Xfixes.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <iostream>

namespace bstk {

std::unique_ptr<OSContext> CreateContext() { return std::unique_ptr<OSContext>(new XlibContext()); }

} // namespace bstk

static constexpr long kEventMask =
    StructureNotifyMask
    | ButtonPressMask | ButtonReleaseMask
    | PointerMotionMask
    | KeyPressMask | KeyReleaseMask;

struct XlibWindowData
{
    Atom delete_window_atom;
};

struct XlibModuleInfo
{
    void* hlib;
    time_t timestamp;
    uint32_t load_index;
};

static bool PosixFileExists(char const* _path)
{
    return (access(_path, F_OK) == 0);
}

static time_t PosixLastWriteTime(char const* _path)
{
    struct stat file_stat;
    stat(_path, &file_stat);
    return file_stat.st_mtime;
}

static void PosixCopyFile(char const* _src, char const* _dst)
{
    int dest_file = open(_dst,
                         O_CREAT | O_RDWR | O_TRUNC | O_SYNC,
                         S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_ISUID | S_ISGID);

    int source_file = open(_src, O_RDONLY);
    syncfs(source_file);
    off_t source_size = lseek(source_file, 0, SEEK_END);
    lseek(source_file, 0, SEEK_SET);

    void* source_memory = mmap(NULL, source_size, PROT_READ, MAP_PRIVATE, source_file, 0);
    write(dest_file, source_memory, source_size);
    munmap(source_memory, source_size);

    close(source_file);
    close(dest_file);
}

bstk::OSWindow XlibContext::CreateWindow()
{
    constexpr uint32_t kWidth = 1280;
    constexpr uint32_t kHeight = 720;

    bstk::OSWindow output = {};

    Display* const display = XOpenDisplay(nullptr);
    if (!display) return output;

    Window const root_window = DefaultRootWindow(display);
    Visual* const default_visual = DefaultVisual(display, DefaultScreen(display));

    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display, root_window, default_visual, AllocNone);
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    int const swa_mask = CWColormap | CWBorderPixel | CWEventMask;

    Window window = XCreateWindow(display, DefaultRootWindow(display),
                                  0, 0, kWidth, kHeight, 0,
                                  CopyFromParent, InputOutput,
                                  default_visual, swa_mask, &swa);
    if (!window) return output;

    XlibWindowData* window_data = new XlibWindowData{};
    {
        int wm_protocols_size = 0;
        Atom* wm_protocols = nullptr;
        Status result = XGetWMProtocols(display, window, &wm_protocols, &wm_protocols_size);
        //std::cout << "XGetWMProtocols status " << std::to_string(result) << std::endl;
        //std::cout << "protocols found " << std::to_string(wm_protocols_size) << std::endl;
        XFree(wm_protocols);

        Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", True);
        if (wm_delete_window == None)
            std::cout << "[ERROR] WM_DELETE_WINDOW doesn't exist" << std::endl;
        result = XSetWMProtocols(display, window, &wm_delete_window, 1);
        //std::cout << "XSetWMProtocols status " << std::to_string(result) << std::endl;

        result = XGetWMProtocols(display, window, &wm_protocols, &wm_protocols_size);
        //std::cout << "XGetWMProtocols status " << std::to_string(result) << std::endl;
        //std::cout << "protocols found " << std::to_string(wm_protocols_size) << std::endl;
        XFree(wm_protocols);

        int property_count = 0;
        Atom* x_properties = XListProperties(display, window, &property_count);
        //std::cout << "properties found " << std::to_string(property_count) << std::endl;
        XFree(x_properties);

        window_data->delete_window_atom = wm_delete_window;
        (void)result;
    }

    XSelectInput(display, window, kEventMask);

    XStoreName(display, window, "");
    XMapWindow(display, window);

    output.hinstance = (uint64_t)display;
    output.hwindow = (uint64_t)window;
    output.platform_data = window_data;

    for (;;)
    {
        XWindowAttributes window_attributes = {};
        XGetWindowAttributes(display, window, &window_attributes);

        output.size[0] = window_attributes.width;
        output.size[1] = window_attributes.height;

        if (window_attributes.map_state == IsViewable)
            break;
    }

    return output;
}

bool XlibContext::PumpEvents(bstk::OSWindow& _window, iotk::input_t& _state)
{
    XlibWindowData const& window_data = *(XlibWindowData const*)_window.platform_data;

    Display* display = (Display*)_window.hinstance;
    Window window = (Window)_window.hwindow;

    bool run = true;

    XEvent xevent;
    while (XCheckWindowEvent(display, window, kEventMask, &xevent))
    {
        switch(xevent.type)
        {
        case ConfigureNotify:
        {
            XConfigureEvent const& xcevent = xevent.xconfigure;
            _window.size[0] = (uint32_t)xcevent.width;
            _window.size[1] = (uint32_t)xcevent.height;
        } break;

        case ButtonPress:
        {
            XButtonEvent& xbevent = xevent.xbutton;

            if (xbevent.button == Button1)
                _state.button_down |= iotk::kLeftBtn;
            if (xbevent.button == Button2)
                _state.button_down |= iotk::kMiddleBtn;
            if (xbevent.button == Button3)
                _state.button_down |= iotk::kRightBtn;

            if (xbevent.button == Button4)
                _state.wheel_delta = 140;
            if (xbevent.button == Button5)
                _state.wheel_delta = -140;

            std::cout << "button down" << std::endl;
        } break;
        case ButtonRelease:
        {
            XButtonEvent& xbevent = xevent.xbutton;

            if (xbevent.button == Button1)
                _state.button_down &= ~iotk::kLeftBtn;
            if (xbevent.button == Button2)
                _state.button_down &= ~iotk::kMiddleBtn;
            if (xbevent.button == Button3)
                _state.button_down &= ~iotk::kRightBtn;

            std::cout << "button up" << std::endl;
        } break;

        case KeyPress:
        case KeyRelease:
        {
            static std::array<iotk::eKey, 256> key_map = [](){
                std::array<iotk::eKey, 256> output{};
                output[(std::uint8_t)(XK_Tab & 0xff)] = iotk::eKey::kTab;
                output[(std::uint8_t)(XK_Left & 0xff)] = iotk::eKey::kLeft;
                output[(std::uint8_t)(XK_Right & 0xff)] = iotk::eKey::kRight;
                output[(std::uint8_t)(XK_Up & 0xff)] = iotk::eKey::kUp;
                output[(std::uint8_t)(XK_Down & 0xff)] = iotk::eKey::kDown;
                output[(std::uint8_t)(XK_Page_Up & 0xff)] = iotk::eKey::kPageUp;
                output[(std::uint8_t)(XK_Page_Down & 0xff)] = iotk::eKey::kPageDown;
                output[(std::uint8_t)(XK_Home & 0xff)] = iotk::eKey::kHome;
                output[(std::uint8_t)(XK_End & 0xff)] = iotk::eKey::kEnd;
                output[(std::uint8_t)(XK_Insert & 0xff)] = iotk::eKey::kInsert;
                output[(std::uint8_t)(XK_Delete & 0xff)] = iotk::eKey::kDelete;
                output[(std::uint8_t)(XK_BackSpace & 0xff)] = iotk::eKey::kBackspace;
                output[(std::uint8_t)(XK_Return & 0xff)] = iotk::eKey::kEnter;
                output[(std::uint8_t)(XK_Escape & 0xff)] = iotk::eKey::kEscape;
                return output;
            }();

            XKeyEvent& xkevent = xevent.xkey;

            unsigned mod_mask = 0;
            {
                Window a, b; int c, d, e, f;
                XQueryPointer(display, window, &a, &b, &c, &d, &e, &f, &mod_mask);
            }

            char kc = '\0';
            KeySym ks;
            XLookupString(&xkevent, &kc, 1, &ks, nullptr);

            std::uint32_t km = 0u;
            km |= (mod_mask & ControlMask) ? iotk::fKeyMod::kCtrl : 0u;
            km |= (mod_mask & ShiftMask) ? iotk::fKeyMod::kShift : 0u;
            km |= (mod_mask & Mod1Mask) ? iotk::fKeyMod::kAlt : 0u;
            _state.mod_down = km;

            std::uint32_t key = (((unsigned)ks & 0xff00) == 0xff00)
                ? (std::uint32_t)ks
                : (std::uint32_t)kc;

            if (key < iotk::eKey::kASCIIBegin || key >= iotk::eKey::kASCIIEnd)
                key = key_map[key & 0xff];
            else
                key = (std::uint32_t)std::tolower((unsigned char)key);

            _state.key_down[key] = (xevent.type == KeyPress);
        } break;

        case MotionNotify:
        {
            XMotionEvent& xmevent = xevent.xmotion;

            _state.cursor[0] = xmevent.x;
            _state.cursor[1] = xmevent.y;
        } break;

        case DestroyNotify:
        {
            XDestroyWindowEvent const& xdwevent = xevent.xdestroywindow;
            //std::cout << "window destroy" << std::endl;
            run = !(xdwevent.display == display && xdwevent.window == window);
        } break;
        default: break;
        }
    }

    if (XCheckTypedWindowEvent(display, window, ClientMessage, &xevent))
    {
        //std::cout << "Client message" << std::endl;
        //std::cout << XGetAtomName(display, xevent.xclient.message_type) << std::endl;
        run = !(xevent.xclient.data.l[0] == window_data.delete_window_atom);
    }

    if (!run)
        delete (XlibWindowData*)_window.platform_data;

    return run;
}

bstk::EngineModule XlibContext::EngineLoad(std::string const& _path, std::string const& _lockfile)
{
    bstk::EngineModule module{
        _path,
        _lockfile,
        new XlibModuleInfo{},
        bstk::EngineInterface{
            bstk::StubEngine::Create,
            bstk::StubEngine::Shutdown,
            bstk::StubEngine::Reload,
            bstk::StubEngine::LogicUpdate,
            bstk::StubEngine::DrawFrame
        }
    };

    EngineReloadModule(module);
    return module;
}

void XlibContext::EngineRelease(bstk::EngineModule& _module)
{
    XlibModuleInfo& moduleInfo = *(XlibModuleInfo*)_module.platform_data;
    if (moduleInfo.hlib)
        dlclose(moduleInfo.hlib);
    delete (XlibModuleInfo*)_module.platform_data;
}

bool XlibContext::EngineReloadRequired(bstk::EngineModule const& _module)
{
    XlibModuleInfo& moduleInfo = *(XlibModuleInfo*)_module.platform_data;
    time_t lastWriteTime = PosixLastWriteTime(_module.path.c_str());
    return lastWriteTime > moduleInfo.timestamp;
}

void XlibContext::EngineReloadModule(bstk::EngineModule& _module)
{
    XlibModuleInfo& moduleInfo = *(XlibModuleInfo*)_module.platform_data;

    if (!PosixFileExists(_module.path.c_str()))
        return;

    bool hasLockFile = (_module.lockfile != "");
    for (uint32_t readIndex = 0u; readIndex < 128; ++readIndex)
    {
        if (!hasLockFile
            || !PosixFileExists(_module.lockfile.c_str()))
        {
            std::string altpath = _module.path;
            altpath[altpath.size() - 1] = '_';
            altpath += std::to_string(moduleInfo.load_index++);
            moduleInfo.load_index &= 0xff;

            time_t lastWriteTime = PosixLastWriteTime(_module.path.c_str());

            PosixCopyFile(_module.path.c_str(), altpath.c_str());

            void* hlib = dlopen(altpath.c_str(), RTLD_LAZY);
            if (!hlib)
                std::cout << "hlib not found "
                          << dlerror()
                          << std::endl;

            bstk::EngineInterface interface{
                (bstk::EngineInterface::Create_t)dlsym(hlib, "ModuleInterface_Create"),
                (bstk::EngineInterface::Shutdown_t)dlsym(hlib, "ModuleInterface_Shutdown"),
                (bstk::EngineInterface::Reload_t)dlsym(hlib, "ModuleInterface_Reload"),
                (bstk::EngineInterface::LogicUpdate_t)dlsym(hlib, "ModuleInterface_LogicUpdate"),
                (bstk::EngineInterface::DrawFrame_t)dlsym(hlib, "ModuleInterface_DrawFrame"),
            };

            if (!interface.Create)
                std::cout << "Create not found" << std::endl;

            _module.interface = interface;
            moduleInfo.timestamp = lastWriteTime;
            if (moduleInfo.hlib)
                dlclose(moduleInfo.hlib);
            moduleInfo.hlib = hlib;
            std::cout << "load attempt " << readIndex << std::endl;
            break;
        }

        usleep(100);
    }
}
