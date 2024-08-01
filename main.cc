#include <cstdint>
#include <memory>

#include "loader/iotk.hpp"
#include "loader/bstk.hpp"

#include <iostream>

#include <chrono>
using StdClock = std::chrono::high_resolution_clock;

int main(int argc, char const** argv)
{
    std::unique_ptr<bstk::OSContext> oscontext = bstk::CreateContext();

    bstk::OSWindow mainwindow = oscontext->CreateWindow();
    bstk::EngineModule module = oscontext->EngineLoad(argv[1], (argc > 2) ? argv[2] : "build.lock");
    bstk::EngineInterface* interface = &module.interface;
    bstk::EngineInterface::context_t* engine = interface->Create(&mainwindow);

    iotk::input_t inputState{};
    StdClock::time_point last_frame_begin = StdClock::now();

    while (oscontext->PumpEvents(mainwindow, inputState))
    {
        if (oscontext->EngineReloadRequired(module))
        {
            oscontext->EngineReloadModule(module);
            interface->Reload(engine);
            last_frame_begin = StdClock::now();
        }

        StdClock::time_point frame_marker = last_frame_begin;
        last_frame_begin = StdClock::now();
        float measured_time = static_cast<float>(
            std::chrono::duration_cast<std::chrono::milliseconds>(last_frame_begin-frame_marker)
            .count());

        inputState.time_delta = measured_time / 1000.f;

        bool keep_running = interface->LogicUpdate(engine, &inputState);
        if (!keep_running)
            break;

        interface->DrawFrame(engine, &mainwindow);
        std::memset(&inputState, 0, sizeof(inputState));
    }

    interface->Shutdown(engine);

    return 0;
}
