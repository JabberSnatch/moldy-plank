#include "iotk.hpp"
#include "vktk.hpp"

#include <iostream>

#define DLLEXPORT __declspec(dllexport)

using hinstance_t = uint64_t;
using hwindow_t = uint64_t;

struct context_t
{
    int dummy;
};

extern "C" {

    DLLEXPORT context_t* ModuleInterface_Create(hinstance_t _hinstance, hwindow_t _hwindow)
    {
        std::cout << "Create" << std::endl;
        vktk::CreateContext(_hinstance, _hwindow);
        return nullptr;
    }

    DLLEXPORT void ModuleInterface_Shutdown(context_t*)
    {
        std::cout << "Shutdown" << std::endl;
    }

    DLLEXPORT void ModuleInterface_Reload(context_t*)
    {
        std::cout << "Reload" << std::endl;
    }

    DLLEXPORT void ModuleInterface_LogicUpdate(context_t*, iotk::input_t const*)
    {
        //std::cout << "LogicUpdate" << std::endl;
    }

    DLLEXPORT void ModuleInterface_DrawFrame(context_t*)
    {
        //std::cout << "DrawFrame" << std::endl;
    }
}
