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
    vktk::Swapchain swapchain;
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;

    vktk::Texture font;
    vktk::Buffer staging;
    VkSampler sampler;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;

    uint32_t last_key;
    iotk::input_t lastInput;
};

static bool UpdateSwapchain(context_t* _context, uint32_t const _size[2])
{
    if (_context->swapchain.handle != VK_NULL_HANDLE
        && (_context->swapchain.size[0] != _size[0]
            || _context->swapchain.size[1] != _size[1]))
        _context->swapchain.Release();

    bool result = false;
    if (_context->swapchain.handle == VK_NULL_HANDLE)
    {
        _context->swapchain = _context->vulkan->CreateSwapchain(_size);
        result = true;
    }

    return result;
}

static void LoadMainPipeline(context_t* _context)
{
    constexpr uint32_t kStageCount = 2u;

    static char const vshader[] = R"_(
    #version 450
    const vec2 kTriVertices[] = vec2[3]( vec2(-1.0, 3.0), vec2(-1.0, -1.0), vec2(3.0, -1.0) );
    layout(location = 0) out vec2 texCoords;
    layout(push_constant) uniform DataBlock {
        uint char_index;
    };
    void main() {
        gl_Position = vec4(kTriVertices[gl_VertexIndex], 0.0, 1.0);
        uint char_row = char_index >> 4;
        uint char_col = char_index & 0xf;
        texCoords = ((gl_Position.xy * 0.5 + vec2(0.5)) + vec2(float(char_row), float(char_col))) / 16.0;
    }
    )_";
    static uint32_t const vsize = sizeof(vshader) - 1u;

    static char const fshader[] = R"(
    #version 450
    layout(location = 0) in vec2 texCoords;
    layout(location = 0) out vec4 color;
    layout(set = 0, binding = 0) uniform sampler2D font;
    void main() {
        float fontv = texture(font, texCoords).x;
        color = vec4(0.5, 0.5, 1.0, 1.0) * fontv;
    }
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

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descset_create{};
    descset_create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descset_create.pNext = nullptr;
    descset_create.flags = 0u;
    descset_create.bindingCount = 1;
    descset_create.pBindings = &binding;
    VkDescriptorSetLayout descset_layout{};
    CHECKCALL(vkCreateDescriptorSetLayout, _context->vulkan->device,
              &descset_create, nullptr, &descset_layout);

    VkPushConstantRange push_constants{};
    push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants.offset = 0;
    push_constants.size = 4;

    VkPipelineLayoutCreateInfo pipeline_create{};
    pipeline_create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_create.pNext = nullptr;
    pipeline_create.flags = 0u;
    pipeline_create.setLayoutCount = 1u;
    pipeline_create.pSetLayouts = &descset_layout;
    pipeline_create.pushConstantRangeCount = 1u;
    pipeline_create.pPushConstantRanges = &push_constants;

    CHECKCALL(vkCreatePipelineLayout, _context->vulkan->device, &pipeline_create, nullptr,
              &_context->pipeline_layout);

    _context->graphics_pipeline = _context->vulkan->CreatePipeline(
        _context->pipeline_layout,
        _context->swapchain.render_pass,
        {
            { vmodule, VK_SHADER_STAGE_VERTEX_BIT },
            { fmodule, VK_SHADER_STAGE_FRAGMENT_BIT }
        } );

    VkDescriptorPoolSize descpool_sizes[1] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };

    VkDescriptorPoolCreateInfo descpool_create{};
    descpool_create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descpool_create.pNext = nullptr;
    descpool_create.flags = 0u;
    descpool_create.maxSets = 1u;
    descpool_create.poolSizeCount = sizeof(descpool_sizes) / sizeof(descpool_sizes[0]);
    descpool_create.pPoolSizes = descpool_sizes;
    CHECKCALL(vkCreateDescriptorPool, _context->vulkan->device,
              &descpool_create, nullptr, &_context->descriptor_pool);

    VkDescriptorSetAllocateInfo descset_allocate{};
    descset_allocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descset_allocate.pNext= nullptr;
    descset_allocate.descriptorPool = _context->descriptor_pool;
    descset_allocate.descriptorSetCount = 1u;
    descset_allocate.pSetLayouts = &descset_layout;
    CHECKCALL(vkAllocateDescriptorSets, _context->vulkan->device,
              &descset_allocate, &_context->descriptor_set);

    VkSamplerCreateInfo sampler_create{};
    sampler_create.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create.pNext = nullptr;
    sampler_create.flags = 0u;
    sampler_create.magFilter = VK_FILTER_NEAREST;
    sampler_create.minFilter = VK_FILTER_NEAREST;
    sampler_create.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    CHECKCALL(vkCreateSampler, _context->vulkan->device,
              &sampler_create, nullptr, &_context->sampler);

    vkDestroyShaderModule(_context->vulkan->device, vmodule, nullptr);
    vkDestroyShaderModule(_context->vulkan->device, fmodule, nullptr);
}

static void TestDraw(context_t* _context, uint32_t const _size[2])
{
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0u;
    VkSemaphore acquire_semaphore{};
    vkCreateSemaphore(_context->vulkan->device, &semaphore_info, nullptr, &acquire_semaphore);

    uint32_t swapchainIndex = 0u;
    CHECKCALL(_context->vulkan->fp.AcquireNextImageKHR,
              _context->vulkan->device,
              _context->swapchain.handle,
              UINT64_MAX,
              acquire_semaphore,
              VK_NULL_HANDLE,
              &swapchainIndex);

    VkImageViewCreateInfo image_view_create{};
    image_view_create.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create.pNext = nullptr;
    image_view_create.flags = 0u;
    image_view_create.image = _context->font.image;
    image_view_create.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create.format = VK_FORMAT_R8_UNORM;
    image_view_create.components = VkComponentMapping{};
    image_view_create.subresourceRange = VkImageSubresourceRange{
        VK_IMAGE_ASPECT_COLOR_BIT,
        0u, 1u, 0u, 1u
    };
    VkImageView image_view{};
    CHECKCALL(vkCreateImageView, _context->vulkan->device,
              &image_view_create, nullptr, &image_view);

    VkDescriptorImageInfo image_info{};
    image_info.sampler = _context->sampler;
    image_info.imageView = image_view;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_write{};
    desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_write.pNext = nullptr;
    desc_write.dstSet = _context->descriptor_set;
    desc_write.dstBinding = 0u;
    desc_write.dstArrayElement = 0u;
    desc_write.descriptorCount = 1u;
    desc_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_write.pImageInfo = &image_info;
    desc_write.pBufferInfo = nullptr;
    desc_write.pTexelBufferView = nullptr;
    vkUpdateDescriptorSets(_context->vulkan->device, 1u, &desc_write, 0u, nullptr);

    vktk::CommandBuffer command_buffer = _context->vulkan->CreateCommandBuffer();
    command_buffer.Begin();

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
    rp_begin_info.renderPass = _context->swapchain.render_pass;
    rp_begin_info.framebuffer = _context->swapchain.framebuffers[swapchainIndex];
    rp_begin_info.renderArea = VkRect2D{
        VkOffset2D{ 0, 0 },
        VkExtent2D{ _context->swapchain.size[0], _context->swapchain.size[1] }
    };
    rp_begin_info.clearValueCount = 1;
    rp_begin_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _context->graphics_pipeline);
    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _context->pipeline_layout, 0u,
                            1u, &_context->descriptor_set,
                            0u, nullptr);

    vkCmdPushConstants(command_buffer,
                       _context->pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0u, 4u, &_context->last_key);

    vkCmdDraw(command_buffer, 3u, 1u, 0u, 0u);
    vkCmdEndRenderPass(command_buffer);

    command_buffer.End();

    VkPipelineStageFlags wait_dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 1u;
    submit_info.pWaitSemaphores = &acquire_semaphore;
    submit_info.pWaitDstStageMask = &wait_dst_stage;
    submit_info.commandBufferCount = 1u;
    submit_info.pCommandBuffers = &command_buffer.handle;
    submit_info.signalSemaphoreCount = 0u;
    submit_info.pSignalSemaphores = nullptr;
    vkQueueSubmit(_context->vulkan->present_queue, 1, &submit_info, VK_NULL_HANDLE);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = 0u;
    present_info.pWaitSemaphores = nullptr;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_context->swapchain.handle;
    present_info.pImageIndices = &swapchainIndex;
    present_info.pResults = nullptr;
    CHECKCALL(_context->vulkan->fp.QueuePresentKHR,
              _context->vulkan->present_queue, &present_info);

    vkQueueWaitIdle(_context->vulkan->present_queue);

    vkDestroySemaphore(_context->vulkan->device, acquire_semaphore, nullptr);
}

static void EngineLoad(context_t* _context)
{
}

// geometry
// camera
// structured data
// serialisation

#define BMPTK_IMPLEMENTATION
#include "bmp/bmptk.h"
#include <vector>
#include <fstream>

std::vector<uint8_t> LoadFile(char const* _path)
{
    std::ifstream source_file(_path, std::ios_base::binary);

    source_file.seekg(0, std::ios_base::end);
    size_t size = source_file.tellg();
    source_file.seekg(0, std::ios_base::beg);

    std::vector<uint8_t> memory{};
    memory.resize(size);

    source_file.read((char*)memory.data(), size);

    return memory;
}

extern "C" {

    DLLEXPORT context_t* ModuleInterface_Create(bstk::OSWindow const* _window)
    {
        std::cout << "Create" << std::endl;
        context_t* context = new context_t{};
        context->window = *_window;
        context->vulkan.reset(new vktk::Context(_window->hinstance, _window->hwindow));

        UpdateSwapchain(context, context->window.size);
        LoadMainPipeline(context);

        std::vector<uint8_t> fontfile = LoadFile("../font.bmp");
        bmptk::BitmapFile bmp{};
        bmptk::Result result = bmptk::LoadBMP(fontfile.data(), &bmp);

        context->staging = context->vulkan->CreateBuffer(
            abs(bmp.width)*abs(bmp.height),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        void* mappedData = nullptr;
        CHECKCALL(vkMapMemory, context->vulkan->device,
                  context->staging.memory, 0u, VK_WHOLE_SIZE, 0u, &mappedData);

        uint8_t* imageBuffer = (uint8_t*)mappedData;
        for (uint32_t y = 0; y < abs(bmp.height); ++y)
        {
            for (uint32_t x = 0; x < abs(bmp.width); ++x)
            {
                bmptk::PixelValue src = bmptk::GetPixel(&bmp, x, y);
                uint8_t* dst = imageBuffer + x + y*abs(bmp.width);
                *dst = src.red;
            }
        }

        vkUnmapMemory(context->vulkan->device, context->staging.memory);

        context->font = context->vulkan->CreateTexture(
            abs(bmp.width), abs(bmp.height), 1,
            VK_IMAGE_TYPE_2D, VK_FORMAT_R8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT);

        vktk::CommandBuffer command_buffer = context->vulkan->CreateCommandBuffer();
        command_buffer.Begin();

        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_NONE;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.srcQueueFamilyIndex = 0u;
        image_memory_barrier.dstQueueFamilyIndex = 0u;
        image_memory_barrier.image = context->font.image;
        image_memory_barrier.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0u, 1u, 0u, 1u
        };

        VkBufferMemoryBarrier buffer_memory_barrier{};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        buffer_memory_barrier.srcQueueFamilyIndex = 0u;
        buffer_memory_barrier.dstQueueFamilyIndex = 0u;
        buffer_memory_barrier.buffer = context->staging.buffer;
        buffer_memory_barrier.offset = 0u;
        buffer_memory_barrier.size = abs(bmp.width)*abs(bmp.height);

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0u,
                             0u, nullptr,
                             1u, &buffer_memory_barrier,
                             1u, &image_memory_barrier);

        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset = 0u;
        copy_region.bufferRowLength = 0u;
        copy_region.bufferImageHeight = 0u;
        copy_region.imageSubresource = VkImageSubresourceLayers {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0u, 0u, 1u
        };
        copy_region.imageOffset = VkOffset3D{ 0u, 0u, 0u };
        copy_region.imageExtent = VkExtent3D{
            (uint32_t)abs(bmp.width),
            (uint32_t)abs(bmp.height), 1u };

        vkCmdCopyBufferToImage(command_buffer,
                               context->staging.buffer,
                               context->font.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copy_region);

        image_memory_barrier.oldLayout = image_memory_barrier.newLayout;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.srcAccessMask = image_memory_barrier.dstAccessMask;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0u,
                             0u, nullptr,
                             0u, nullptr,
                             1u, &image_memory_barrier);

        command_buffer.End();

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.waitSemaphoreCount = 0u;
        submit_info.pWaitSemaphores = nullptr;
        submit_info.pWaitDstStageMask = nullptr;
        submit_info.commandBufferCount = 1u;
        submit_info.pCommandBuffers = &command_buffer.handle;
        submit_info.signalSemaphoreCount = 0u;
        submit_info.pSignalSemaphores = nullptr;
        vkQueueSubmit(context->vulkan->present_queue, 1, &submit_info, VK_NULL_HANDLE);

        return context;
    }

    DLLEXPORT void ModuleInterface_Shutdown(context_t* _context)
    {
        std::cout << "Shutdown" << std::endl;
        vkDestroyPipeline(_context->vulkan->device, _context->graphics_pipeline, nullptr);
        _context->swapchain.Release();
        delete _context;
    }

    DLLEXPORT void ModuleInterface_Reload(context_t* _context)
    {
        std::cout << "Reload" << std::endl;

        vkDestroyPipeline(_context->vulkan->device, _context->graphics_pipeline, nullptr);
        _context->swapchain.Release();

        _context->vulkan.release();
        _context->vulkan.reset(new vktk::Context(_context->window.hinstance,
                                                 _context->window.hwindow));

        UpdateSwapchain(_context, _context->window.size);
        LoadMainPipeline(_context);
    }

    DLLEXPORT void ModuleInterface_LogicUpdate(context_t* _context, iotk::input_t const* _input)
    {
        for (uint32_t index = 1u; index < 256; ++index)
        {
            if (_context->lastInput.key_down[index] != _input->key_down[index])
            {
                if (_input->key_down[index])
                {
                    std::cout << "key down " << std::hex << index << std::endl;
                    if (index > (uint32_t)iotk::kASCIIBegin)
                        _context->last_key = index - (uint32_t)iotk::kASCIIBegin;
                }
                else
                    std::cout << "key up " << std::hex << index << std::endl;
            }
        }

        _context->lastInput = *_input;
    }

    DLLEXPORT void ModuleInterface_DrawFrame(context_t* _context, bstk::OSWindow const* _window)
    {
        //std::cout << "DrawFrame" << std::endl;
        _context->window = *_window;
        if (UpdateSwapchain(_context, _window->size))
        {
            vkDestroyPipeline(_context->vulkan->device, _context->graphics_pipeline, nullptr);
            LoadMainPipeline(_context);
        }
        TestDraw(_context, _window->size);
    }
}
