#include "iotk.hpp"
#include "vktk.hpp"
#include "bstk.hpp"

#include <cstdint>
#include <iostream>

#define DLLEXPORT __declspec(dllexport)

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

    VkPipelineShaderStageCreateInfo shaderStages[kStageCount];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].flags = 0u;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vmodule;
    shaderStages[0].pName = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].flags = 0u;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fmodule;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

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

    VkAttachmentDescription renderPassAttachment{};
    renderPassAttachment.flags = 0u;
    renderPassAttachment.format = _context->vulkan->swapchain_format;
    renderPassAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    renderPassAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    renderPassAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderPassAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderPassAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachment{};
    colorAttachment.attachment = 0u;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription renderSubpass{};
    renderSubpass.flags = 0u;
    renderSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    renderSubpass.inputAttachmentCount = 0u;
    renderSubpass.pInputAttachments = nullptr;
    renderSubpass.colorAttachmentCount = 1u;
    renderSubpass.pColorAttachments = &colorAttachment;
    renderSubpass.pResolveAttachments = nullptr;
    renderSubpass.pDepthStencilAttachment = nullptr;
    renderSubpass.preserveAttachmentCount = 0u;
    renderSubpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPass{};
    renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPass.pNext = nullptr;
    renderPass.flags = 0u;
    renderPass.attachmentCount = 1u;
    renderPass.pAttachments = &renderPassAttachment;
    renderPass.subpassCount = 1u;
    renderPass.pSubpasses = &renderSubpass;
    renderPass.dependencyCount = 0u;
    renderPass.pDependencies = nullptr;

    VkRenderPass render_pass{};
    CHECKCALL(vkCreateRenderPass, _context->vulkan->device, &renderPass, nullptr,
              &render_pass);

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.flags = 0u;
    viewportState.viewportCount = 0u;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 0u;
    viewportState.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.pNext = nullptr;
    rasterizationState.flags = 0u;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_TRUE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.f;
    rasterizationState.depthBiasClamp = 0.f;
    rasterizationState.depthBiasSlopeFactor = 0.f;
    rasterizationState.lineWidth = 1.f;

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = nullptr;
    vertexInputState.flags = 0u;
    vertexInputState.vertexBindingDescriptionCount = 0u;
    vertexInputState.pVertexBindingDescriptions = nullptr;
    vertexInputState.vertexAttributeDescriptionCount = 0u;
    vertexInputState.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext = nullptr;
    inputAssemblyState.flags = 0u;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkDynamicState state = VK_DYNAMIC_STATE_VIEWPORT;
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = nullptr;
    dynamicState.flags = 0u;
    dynamicState.dynamicStateCount = 1u;
    dynamicState.pDynamicStates = &state;

    VkGraphicsPipelineCreateInfo createPipeline{};
    createPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createPipeline.pNext = nullptr;
    createPipeline.flags = 0u;
    createPipeline.stageCount = kStageCount;
    createPipeline.pStages = shaderStages;
    createPipeline.pVertexInputState = &vertexInputState;
    createPipeline.pInputAssemblyState = &inputAssemblyState;
    createPipeline.pTessellationState = nullptr;
    createPipeline.pViewportState = nullptr;
    createPipeline.pRasterizationState = &rasterizationState;
    createPipeline.pMultisampleState = nullptr;
    createPipeline.pDepthStencilState = nullptr;
    createPipeline.pColorBlendState = nullptr;
    createPipeline.pDynamicState = &dynamicState;
    createPipeline.layout = pipeline_layout;
    createPipeline.renderPass = render_pass;
    createPipeline.subpass = 0;
    createPipeline.basePipelineHandle = 0;
    createPipeline.basePipelineIndex = 0;

    VkPipeline graphics_pipeline{};
    CHECKCALL(vkCreateGraphicsPipelines, _context->vulkan->device, VK_NULL_HANDLE,
              1u, &createPipeline, nullptr, &graphics_pipeline);

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.flags = 0u;
    framebufferInfo.renderPass = render_pass;
    framebufferInfo.attachmentCount = 1u;
    framebufferInfo.pAttachments = &_context->vulkan->swapchain_views[0];
    framebufferInfo.width = _size[0];
    framebufferInfo.height = _size[1];
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer{};
    CHECKCALL(vkCreateFramebuffer, _context->vulkan->device, &framebufferInfo, nullptr, &framebuffer);

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
