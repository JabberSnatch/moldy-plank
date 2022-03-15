#pragma once

#include <cstdint>
#include <memory>

#include "iotk.hpp"

namespace bstk
{

struct EngineInterface
{
    using context_t = void;
    using hinstance_t = uint64_t;
    using hwindow_t = uint64_t;

    context_t* (*Create)(hinstance_t, hwindow_t);
    void (*Shutdown)(context_t*);
    void (*Reload)(context_t*);

    void (*LogicUpdate)(context_t*, iotk::input_t const*);
    void (*DrawFrame)(context_t*);
};

struct EngineModule
{
    char const* path;
    uint64_t platform_handle;
    uint64_t timestamp;
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

    virtual EngineModule EngineLoad(char const* _path) = 0;
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
    EngineModule EngineLoad(char const*) override {
        return EngineModule{
            "",
            0ull,
            0ull,
            EngineInterface{
                StubEngine::Create,
                StubEngine::Shutdown,
                StubEngine::Reload,
                StubEngine::LogicUpdate,
                StubEngine::DrawFrame
            }
        };
    }
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
