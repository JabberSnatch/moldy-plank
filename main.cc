#include <cstdint>
#include <memory>

#include "iotk.hpp"
#include "bstk.hpp"

#include <iostream>

int main(int argc, char const** argv)
{
    std::unique_ptr<bstk::OSContext> oscontext = bstk::CreateContext();

    bstk::OSWindow mainwindow = oscontext->CreateWindow();
    bstk::EngineModule module = oscontext->EngineLoad(argv[1], (argc > 2) ? argv[2] : "");
    bstk::EngineInterface* interface = &module.interface;
    bstk::EngineInterface::context_t* engine = interface->Create(&mainwindow);

    iotk::input_t inputState{};

    while (oscontext->PumpEvents(mainwindow, inputState))
    {
        if (oscontext->EngineReloadRequired(module))
        {
            oscontext->EngineReloadModule(module);
            interface->Reload(engine);
        }

        interface->LogicUpdate(engine, &inputState);
        interface->DrawFrame(engine, &mainwindow);
    }

    interface->Shutdown(engine);

    return 0;
}
