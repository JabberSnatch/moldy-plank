#include "iotk.hpp"
#include "vktk.hpp"
#include "bstk.hpp"

#include <cstdint>
#include <iostream>

#if defined(_WIN32)
#define DLLEXPORT __declspec(dllexport)
#elif defined(__unix__)
#define DLLEXPORT __attribute__((visibility("default")))
#else
#define DLLEXPORT
#endif

struct context_t
{
    bstk::OSWindow window;
    std::unique_ptr<vktk::Context> vulkan;

    iotk::input_t lastInput;
};

static void EngineLoad(context_t* _context, uint32_t _size[2])
{
    constexpr uint32_t kStageCount = 2u;

    static char const vshader[] = R"(
    #version 450
    const vec2 kTriVertices[] = vec2[3]( vec2(-1.0, 3.0), vec2(-1.0, -1.0), vec2(3.0, -1.0) );
    void main() { gl_Position = vec4(kTriVertices[gl_VertexIndex], 0.0, 1.0); }
    )";
    static uint32_t const vsize = sizeof(vshader) - 1u;

    static char const fshader[] = R"(
    #version 450
    layout(location = 0) out vec4 color;
    void main() { color = vec4(1.0, 0.0, 1.0, 1.0); }
    )";
    static uint32_t const fsize = sizeof(fshader) - 1u;

    VkShaderModule vmodule =
        _context->vulkan->CompileShader_GLSL(VK_SHADER_STAGE_VERTEX_BIT,
                                             "vshader",
                                             vshader,
                                             vsize);

    VkShaderModule fmodule =
        _context->vulkan->CompileShader_GLSL(VK_SHADER_STAGE_FRAGMENT_BIT,
                                             "fshader",
                                             fshader,
                                             fsize);

    VkPipelineLayoutCreateInfo pipelineLayout{};
    pipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayout.pNext = nullptr;
    pipelineLayout.flags = 0u;
    pipelineLayout.setLayoutCount = 0u;
    pipelineLayout.pSetLayouts = nullptr;
    pipelineLayout.pushConstantRangeCount = 0u;
    pipelineLayout.pPushConstantRanges = nullptr;

    VkPipelineLayout pipeline_layout{};
    CHECKCALL(vkCreatePipelineLayout, _context->vulkan->device, &pipelineLayout, nullptr,
              &pipeline_layout);

    VkRenderPass render_pass = _context->vulkan->CreateRenderPass(
        {
            { _context->vulkan->swapchain_format,
              VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
        } );

    VkPipeline graphics_pipeline = _context->vulkan->CreatePipeline(
        pipeline_layout,
        render_pass,
        {
            { vmodule, VK_SHADER_STAGE_VERTEX_BIT },
            { fmodule, VK_SHADER_STAGE_FRAGMENT_BIT }
        } );

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0u;
    VkSemaphore acquire_semaphore{};
    vkCreateSemaphore(_context->vulkan->device, &semaphore_info, nullptr, &acquire_semaphore);

    uint32_t swapchainIndex = 0u;
    CHECKCALL(_context->vulkan->fp.AcquireNextImageKHR,
              _context->vulkan->device,
              _context->vulkan->swapchain,
              UINT64_MAX,
              acquire_semaphore,
              VK_NULL_HANDLE,
              &swapchainIndex);

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.flags = 0u;
    framebufferInfo.renderPass = render_pass;
    framebufferInfo.attachmentCount = 1u;
    framebufferInfo.pAttachments = &_context->vulkan->swapchain_views[swapchainIndex];
    framebufferInfo.width = _size[0];
    framebufferInfo.height = _size[1];
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer{};
    CHECKCALL(vkCreateFramebuffer, _context->vulkan->device, &framebufferInfo, nullptr, &framebuffer);

    VkCommandBufferAllocateInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.pNext = nullptr;
    command_buffer_info.commandPool = _context->vulkan->command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer{};
    CHECKCALL(vkAllocateCommandBuffers, _context->vulkan->device, &command_buffer_info, &command_buffer);

    VkCommandBufferBeginInfo cmd_begin_info{};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = nullptr;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_begin_info.pInheritanceInfo = nullptr;
    vkBeginCommandBuffer(command_buffer, &cmd_begin_info);

    VkViewport viewport{};
    viewport.x = 0.f; viewport.y = 0.f;
    viewport.width = (float)_size[0]; viewport.height = (float)_size[1];
    viewport.minDepth = 0.f; viewport.maxDepth = 1.f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{ VkOffset2D{ 0, 0 }, VkExtent2D{ _size[0], _size[1] } };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkClearValue clear_value{};
    clear_value.color.float32[0] = 0.f; clear_value.color.float32[1] = 1.f;
    clear_value.color.float32[2] = 1.f; clear_value.color.float32[3] = 0.f;
    clear_value.depthStencil.depth = 0.f;
    clear_value.depthStencil.stencil = 0;
    VkRenderPassBeginInfo rp_begin_info{};
    rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin_info.pNext = nullptr;
    rp_begin_info.renderPass = render_pass;
    rp_begin_info.framebuffer = framebuffer;
    rp_begin_info.renderArea = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ _size[0], _size[1] } };
    rp_begin_info.clearValueCount = 1;
    rp_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdDraw(command_buffer, 3u, 1u, 0u, 0u);
    vkCmdEndRenderPass(command_buffer);

    vkEndCommandBuffer(command_buffer);

    VkPipelineStageFlags wait_dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1u;
    submit_info.pWaitSemaphores = &acquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_dst_stage;
    submit_info.commandBufferCount = 1u;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0u;
    submit_info.pSignalSemaphores = nullptr;
    vkQueueSubmit(_context->vulkan->present_queue, 1, &submit_info, VK_NULL_HANDLE);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 0u;
    present_info.pWaitSemaphores = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_context->vulkan->swapchain;
    present_info.pImageIndices = &swapchainIndex;
    present_info.pResults = nullptr;
    CHECKCALL(_context->vulkan->fp.QueuePresentKHR,
              _context->vulkan->present_queue, &present_info);

    vkQueueWaitIdle(_context->vulkan->present_queue);

    vkDestroySemaphore(_context->vulkan->device, acquire_semaphore, nullptr);
    vkDestroyFramebuffer(_context->vulkan->device, framebuffer, nullptr);

    if (vmodule != VK_NULL_HANDLE)
        vkDestroyShaderModule(_context->vulkan->device, vmodule, nullptr);
    if (fmodule != VK_NULL_HANDLE)
        vkDestroyShaderModule(_context->vulkan->device, fmodule, nullptr);

    vkDestroyPipeline(_context->vulkan->device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(_context->vulkan->device, pipeline_layout, nullptr);
    vkDestroyRenderPass(_context->vulkan->device, render_pass, nullptr);
}

// geometry
// camera
// shaders
// textures
// bindings
// structured data
// serialisation

extern "C" {

    DLLEXPORT context_t* ModuleInterface_Create(bstk::OSWindow const* _window)
    {
        std::cout << "Create" << std::endl;
        context_t* context = new context_t{};
        context->window = *_window;
        context->vulkan.reset(new vktk::Context(_window->hinstance, _window->hwindow));
        EngineLoad(context, context->window.size);
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
        EngineLoad(_context, _context->window.size);
    }

    DLLEXPORT void ModuleInterface_LogicUpdate(context_t* _context, iotk::input_t const* _input)
    {
        _context->lastInput = *_input;
        //std::cout << "LogicUpdate" << std::endl;
    }

    DLLEXPORT void ModuleInterface_DrawFrame(context_t*)
    {
        //std::cout << "DrawFrame" << std::endl;
    }
}
