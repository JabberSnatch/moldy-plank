#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "iotk.hpp"

namespace bstk
{

struct EngineInterface
{
    using context_t = void;
    using hinstance_t = uint64_t;
    using hwindow_t = uint64_t;

    using Create_t = context_t* (*)(hinstance_t, hwindow_t);
    using Shutdown_t = void (*)(context_t*);
    using Reload_t = void (*)(context_t*);
    using LogicUpdate_t = void (*)(context_t*, iotk::input_t const*);
    using DrawFrame_t = void (*)(context_t*);

    Create_t Create;
    Shutdown_t Shutdown;
    Reload_t Reload;
    LogicUpdate_t LogicUpdate;
    DrawFrame_t DrawFrame;
};

struct EngineModule
{
    std::string path;
    std::string lockfile;
    void* platform_data;
    EngineInterface interface;
};

struct OSWindow
{
    uint64_t hinstance;
    uint64_t hwindow;
    iotk::input_t state;
};

struct OSContext
{
    OSContext() = default;
    virtual ~OSContext() = default;
    OSContext(OSContext const&) = delete;
    OSContext& operator=(OSContext const&) = delete;

    virtual OSWindow CreateWindow() = 0;
    virtual bool PumpEvents(OSWindow& _window) = 0;

    virtual EngineModule EngineLoad(std::string const& _path, std::string const& _lockfile) = 0;
    virtual void EngineRelease(EngineModule& _module) = 0;
    virtual bool EngineReloadRequired(EngineModule const& _module) = 0;
    virtual void EngineReloadModule(EngineModule& _module) = 0;
};

namespace StubEngine
{
inline void* Create(uint64_t, uint64_t) { return nullptr; }
inline void Shutdown(void*) {}
inline void Reload(void*) {}
inline void LogicUpdate(void*, iotk::input_t const*) {}
inline void DrawFrame(void*) {}
}

struct StubOS : public OSContext
{
    StubOS() = default;
    OSWindow CreateWindow() override { return OSWindow{}; }
    bool PumpEvents(OSWindow&) override { return false; }
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
    void EngineRelease(EngineModule&) {}
    bool EngineReloadRequired(EngineModule const&) { return false; }
    void EngineReloadModule(EngineModule&) {}
};

#ifdef _WIN32
std::unique_ptr<OSContext> CreateContext();
#else
// Implementation is platform-dependant
inline std::unique_ptr<OSContext> CreateContext()
{
    return std::unique_ptr<OSContext>(new StubOS());
}
#endif

}
