#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "iotk.hpp"

namespace bstk
{

using hinstance_t = uint64_t;
using hwindow_t = uint64_t;

struct OSWindow
{
    hinstance_t hinstance;
    hwindow_t hwindow;
    uint32_t size[2];
    void* platform_data;
};

struct EngineInterface
{
    using context_t = void;

    using Create_t = context_t* (*)(OSWindow const*);
    using Shutdown_t = void (*)(context_t*);
    using Reload_t = void (*)(context_t*);
    using LogicUpdate_t = bool (*)(context_t*, iotk::input_t const*);
    using DrawFrame_t = void (*)(context_t*, bstk::OSWindow const*);

    Create_t Create;
    Shutdown_t Shutdown;
    Reload_t Reload;
    LogicUpdate_t LogicUpdate;
    DrawFrame_t DrawFrame;
};

using PlatformData = void*;

struct EngineModule
{
    std::string path;
    std::string lockfile;
    PlatformData platform_data;
    EngineInterface interface;
};

struct OSContext
{
    OSContext() = default;
    virtual ~OSContext() = default;
    OSContext(OSContext const&) = delete;
    OSContext& operator=(OSContext const&) = delete;

    virtual OSWindow CreateWindow() = 0;
    virtual bool PumpEvents(OSWindow& _window, iotk::input_t& _state) = 0;

    virtual EngineModule EngineLoad(std::string const& _path, std::string const& _lockfile) = 0;
    virtual void EngineRelease(EngineModule& _module) = 0;
    virtual bool EngineReloadRequired(EngineModule const& _module) = 0;
    virtual PlatformData EngineReloadModule(EngineModule& _module) = 0;
    virtual void EngineReleasePlatformData(PlatformData _data) = 0;
};

namespace StubEngine
{
inline void* Create(OSWindow const*) { return nullptr; }
inline void Shutdown(void*) {}
inline void Reload(void*) {}
inline bool LogicUpdate(void*, iotk::input_t const*) { return true; }
inline void DrawFrame(void*, OSWindow const*) {}
}

struct StubOS : public OSContext
{
    StubOS() = default;
    OSWindow CreateWindow() override { return OSWindow{}; }
    bool PumpEvents(OSWindow&, iotk::input_t&) override { return false; }
    EngineModule EngineLoad(std::string const&, std::string const&) override {
        return EngineModule{
            "",
            "",
            nullptr,
            EngineInterface{
                StubEngine::Create,
                StubEngine::Shutdown,
                StubEngine::Reload,
                StubEngine::LogicUpdate,
                StubEngine::DrawFrame
            }
        };
    }
    void EngineRelease(EngineModule&) override {}
    bool EngineReloadRequired(EngineModule const&) override { return false; }
    PlatformData EngineReloadModule(EngineModule&) override { return nullptr; }
    void EngineReleasePlatformData(PlatformData) override {}
};

#if defined(_WIN32) || defined(__unix__)
std::unique_ptr<OSContext> CreateContext();
#else
// Implementation is platform-dependant
inline std::unique_ptr<OSContext> CreateContext()
{
    return std::unique_ptr<OSContext>(new StubOS());
}
#endif

} // namespace bstk
