#include "iotk.hpp"
#include "vktk.hpp"

#include <iostream>

#define DLLEXPORT __declspec(dllexport)

using hinstance_t = uint64_t;
using hwindow_t = uint64_t;

struct context_t
{
    vktk::Context* renderContext;
};

extern "C" {

    DLLEXPORT context_t* ModuleInterface_Create(hinstance_t _hinstance, hwindow_t _hwindow)
    {
        std::cout << "Create" << std::endl;
        context_t* context = new context_t{};
        context->renderContext = vktk::CreateContext(_hinstance, _hwindow);
        return context;
    }

    DLLEXPORT void ModuleInterface_Shutdown(context_t* _context)
    {
        std::cout << "Shutdown" << std::endl;
        vktk::DeleteContext(_context->renderContext);
        delete _context;
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
