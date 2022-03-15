#include <cstdint>
#include <memory>

#include "iotk.hpp"
#include "bstk.hpp"

#include <iostream>

int main(int argc, char const** argv)
{
    std::unique_ptr<bstk::OSContext> oscontext = bstk::CreateContext();

    bstk::OSWindow mainwindow = oscontext->CreateWindow();
    bstk::EngineModule module = oscontext->EngineLoad(argv[0]);
    bstk::EngineInterface* interface = &module.interface;
    bstk::EngineInterface::context_t* engine =
        interface->Create(mainwindow.hinstance, mainwindow.hwindow);

    while (oscontext->PumpEvents(mainwindow))
    {
        if (oscontext->EngineReloadRequired(module))
        {
            oscontext->EngineReloadModule(module);
            interface->Reload(engine);
        }

        interface->LogicUpdate(engine, &mainwindow.state);
        interface->DrawFrame(engine);
    }

    interface->Shutdown(engine);

    return 0;
}
