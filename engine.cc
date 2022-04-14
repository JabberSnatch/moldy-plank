#include "iotk.hpp"
#include "vktk.hpp"

#include <cstdint>
#include <iostream>

#define DLLEXPORT __declspec(dllexport)

using hinstance_t = uint64_t;
using hwindow_t = uint64_t;

struct context_t
{
    std::unique_ptr<vktk::Context> vulkan;

    iotk::input_t lastInput;
};

static void EngineLoad(context_t* _context)
{
    static char const shaderSource[] = "#version 450\n void main() {}";
    static uint32_t const size = sizeof(shaderSource) - 1u;
    VkShaderModule shader =
        _context->vulkan->CompileShader_GLSL(VK_SHADER_STAGE_VERTEX_BIT,
                                             "test",
                                             shaderSource,
                                             size);
    if (shader != VK_NULL_HANDLE)
        vkDestroyShaderModule(_context->vulkan->device, shader, nullptr);
}

// geometry
// camera
// shaders
// textures
// bindings
// structured data
// serialisation

extern "C" {

    DLLEXPORT context_t* ModuleInterface_Create(hinstance_t _hinstance, hwindow_t _hwindow)
    {
        std::cout << "Create" << std::endl;
        context_t* context = new context_t{};
        context->vulkan.reset(new vktk::Context(_hinstance, _hwindow));
        EngineLoad(context);
        return context;
    }

    DLLEXPORT void ModuleInterface_Shutdown(context_t* _context)
    {
        std::cout << "Shutdown" << std::endl;
        delete _context;
    }

    DLLEXPORT void ModuleInterface_Reload(context_t* _context)
    {
        std::cout << "Reload" << std::endl;
        EngineLoad(_context);
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
