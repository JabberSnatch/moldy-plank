#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

#define CHECKCALL(func, ...)                                            \
    {                                                                   \
        VkResult const vkr = func(__VA_ARGS__);                         \
        if (vkr != VK_SUCCESS)                                          \
            std::cout << #func " " << vktk::VkResultToStr(vkr) << std::endl; \
    }

#define GetInstanceProcAddress(proc, inst, table)                       \
    table.proc = (PFN_vk##proc)vkGetInstanceProcAddr(inst, "vk" #proc);

#define GetDeviceProcAddress(proc, dev, table)                          \
    table.proc = (PFN_vk##proc)vkGetDeviceProcAddr(dev, "vk" #proc);


#define kRequiredInstanceProcs(x)               \
    x(GetPhysicalDeviceSurfaceCapabilitiesKHR)  \
    x(GetPhysicalDeviceSurfaceFormatsKHR)       \
    x(GetPhysicalDeviceSurfaceSupportKHR)       \

#define kRequiredDeviceProcs(x)                 \
    x(CreateSwapchainKHR)                       \
    x(DestroySwapchainKHR)                      \
    x(GetSwapchainImagesKHR)                    \
    x(AcquireNextImageKHR)                      \
    x(QueuePresentKHR)                          \

#define GetInstanceProcAddress_XMacro(proc) GetInstanceProcAddress(proc, INSTANCE, TABLE)
#define GetDeviceProcAddress_XMacro(proc) GetDeviceProcAddress(proc, DEVICE, TABLE)

namespace vktk
{

char const* VkResultToStr(VkResult _vkResult);

struct FPTable
{
#define AsFPTableEntry(proc) PFN_vk##proc proc = nullptr;
    kRequiredInstanceProcs(AsFPTableEntry)

    kRequiredDeviceProcs(AsFPTableEntry)
#undef AsFPTableEntry
};

struct Context;

struct Swapchain {
    uint32_t size[2] = { 0, 0 };
    Context const* vulkan;

    VkFormat format;
    VkRenderPass render_pass;
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    std::vector<VkFramebuffer> framebuffers;

    void Release();
};

struct Context
{
    Context(uint64_t _hinstance, uint64_t _hwindow);
    ~Context();

    VkShaderModule CompileShader_GLSL(VkShaderStageFlagBits _shaderStage,
                                      char const* _shaderName,
                                      char const* _code,
                                      size_t _codeSize);

    struct RenderPassAttachment {
        VkFormat format;
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
    };
    VkRenderPass CreateRenderPass(std::vector<RenderPassAttachment> const& _attachments);

    struct PipelineStage {
        VkShaderModule module;
        VkShaderStageFlagBits stage;
    };
    VkPipeline CreatePipeline(VkPipelineLayout _layout,
                              VkRenderPass _renderPass,
                              std::vector<PipelineStage> const& _stages);

    Swapchain CreateSwapchain(uint32_t const _size[2]);

    FPTable fp = {};

    VkInstance instance = {};
    VkPhysicalDevice physical_device = {};
    VkPhysicalDeviceMemoryProperties memory_properties = {};

    VkSurfaceKHR surface = {};
    VkDevice device = {};
    VkQueue present_queue = {};
    VkCommandPool command_pool = {};
};

} // namespace vktk
