#include "vktk.hpp"

#include <shaderc/shaderc.hpp>

#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{

static char const* kInstanceExtensions[] = {
    "VK_KHR_surface",
#ifdef _WIN32
    "VK_KHR_win32_surface",
#endif
};
static std::uint32_t const kInstanceExtCount =
    sizeof(kInstanceExtensions) / sizeof(kInstanceExtensions[0]);

static char const* kDeviceExtensions[] = {
    "VK_KHR_swapchain"
};
static std::uint32_t const kDeviceExtCount =
    sizeof(kDeviceExtensions) / sizeof(kDeviceExtensions[0]);

} // namespace

namespace vktk
{

Context::Context(std::uint64_t _hinstance, std::uint64_t _hwindow)
{
    {
        VkApplicationInfo app_info{};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pNext = nullptr;
        app_info.pApplicationName = "";
        app_info.applicationVersion = 0;
        app_info.pEngineName = "";
        app_info.engineVersion = 0;
        app_info.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo instance_info{};
        instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pNext = nullptr;
        instance_info.flags = 0;
        instance_info.pApplicationInfo = &app_info;
        instance_info.enabledLayerCount = 0;
        instance_info.ppEnabledLayerNames = nullptr;
        instance_info.enabledExtensionCount = kInstanceExtCount;
        instance_info.ppEnabledExtensionNames = kInstanceExtensions;

        CHECKCALL(vkCreateInstance, &instance_info, nullptr, &instance);
    }

    #define INSTANCE instance
    #define TABLE fp
    kRequiredInstanceProcs(GetInstanceProcAddress_XMacro)
    #undef INSTANCE
    #undef TABLE

    {
        uint32_t dev_count = 1u;
        CHECKCALL(vkEnumeratePhysicalDevices, instance, &dev_count, &physical_device);
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    }

#ifdef _WIN32
    {
        VkWin32SurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.hinstance = (HINSTANCE)_hinstance;
        create_info.hwnd = (HWND)_hwindow;
        CHECKCALL(vkCreateWin32SurfaceKHR, instance, &create_info, nullptr, &surface);
    }
#endif

    {
        std::uint32_t queueFamilyCount = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount,
                                                 nullptr);
        std::vector<VkQueueFamilyProperties> queue_properties{ queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queueFamilyCount,
                                                 queue_properties.data());

        std::uint32_t queueIndex = ~0u;
        for (std::uint32_t index = 0u; index < queue_properties.size(); ++index)
        {
            VkBool32 canPresent = VK_FALSE;
            fp.GetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface,
                                                  &canPresent);
            if (queue_properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                canPresent == VK_TRUE)
            {
                queueIndex = index;
                break;
            }
        }

        static float const kQueuePriority = 0.f;
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.flags = 0;
        queue_info.queueFamilyIndex = queueIndex;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &kQueuePriority;

        VkDeviceCreateInfo device_info{};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = nullptr;
        device_info.flags = 0;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledLayerCount = 0;
        device_info.ppEnabledLayerNames = nullptr;
        device_info.enabledExtensionCount = kDeviceExtCount;
        device_info.ppEnabledExtensionNames = kDeviceExtensions;
        device_info.pEnabledFeatures = nullptr;

        CHECKCALL(vkCreateDevice, physical_device, &device_info, nullptr, &device);

        vkGetDeviceQueue(device, queueIndex, 0, &present_queue);
    }

    #define DEVICE device
    #define TABLE fp
    kRequiredDeviceProcs(GetDeviceProcAddress_XMacro)
    #undef DEVICE
    #undef TABLE

    {
        VkFormat swapchain_format{};
        VkColorSpaceKHR color_space{};
        {
            std::uint32_t format_count = 1;
            VkSurfaceFormatKHR surface_format{};
            fp.GetPhysicalDeviceSurfaceFormatsKHR(physical_device,
                                                  surface,
                                                  &format_count, &surface_format);

            swapchain_format = surface_format.format;
            if (surface_format.format == VK_FORMAT_UNDEFINED)
                swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

            color_space = surface_format.colorSpace;
        }

        VkSurfaceCapabilitiesKHR surface_capabilities{};
        fp.GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device,
                                                   surface,
                                                   &surface_capabilities);

        VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
        if (surface_capabilities.currentExtent.width = (uint32_t)-1)
            swapchain_extent = surface_capabilities.minImageExtent;

        VkSwapchainCreateInfoKHR swapchain_info{};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.pNext = nullptr;
        swapchain_info.flags = 0;
        swapchain_info.surface = surface;
        swapchain_info.minImageCount = 2;
        swapchain_info.imageFormat = swapchain_format;
        swapchain_info.imageColorSpace = color_space;
        swapchain_info.imageExtent = swapchain_extent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
        swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchain_info.clipped = VK_TRUE;
        swapchain_info.oldSwapchain = VK_NULL_HANDLE;

        CHECKCALL(fp.CreateSwapchainKHR, device, &swapchain_info, nullptr, &swapchain);

        uint32_t image_count = 0u;
        fp.GetSwapchainImagesKHR(device, swapchain, &image_count,
                                       nullptr);
        swapchain_images.resize(image_count);
        swapchain_views.resize(image_count);
        swapchain_framebuffers.resize(image_count);
        fp.GetSwapchainImagesKHR(device, swapchain, &image_count,
                                 swapchain_images.data());

        for (uint32_t index = 0u; index < image_count; ++index)
        {
            VkImageViewCreateInfo image_view_info{};
            image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_info.pNext = nullptr;
            image_view_info.flags = 0;
            image_view_info.image = swapchain_images[index];
            image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_info.format = swapchain_format;
            image_view_info.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
            };
            image_view_info.subresourceRange = {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1
            };

            CHECKCALL(vkCreateImageView, device, &image_view_info, nullptr,
                      &swapchain_views[index]);
        }
    }
}

Context::~Context()
{
    for (VkImageView view : swapchain_views)
        vkDestroyImageView(device, view, nullptr);
    fp.DestroySwapchainKHR(device, swapchain, nullptr);

#ifdef _WIN32
    vkDestroySurfaceKHR(instance, surface, nullptr);
#endif

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}


VkShaderModule Context::CompileShader_GLSL(VkShaderStageFlagBits _shaderStage,
                                           char const* _shaderName,
                                           char const* _code,
                                           size_t _codeSize)
{
    shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source;
    switch (_shaderStage)
    {
    case VK_SHADER_STAGE_VERTEX_BIT:
        shader_kind = shaderc_vertex_shader; break;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        shader_kind = shaderc_tess_control_shader; break;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        shader_kind = shaderc_tess_evaluation_shader; break;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        shader_kind = shaderc_geometry_shader; break;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        shader_kind = shaderc_fragment_shader; break;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        shader_kind = shaderc_compute_shader; break;
    default: break;
    }

    shaderc::SpvCompilationResult compilation =
        shaderc::Compiler{}.CompileGlslToSpv(_code, _codeSize, shader_kind, _shaderName);

    if (compilation.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cout << compilation.GetErrorMessage() << std::endl;
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0u;
    create_info.codeSize = (compilation.end() - compilation.begin()) * 4;
    create_info.pCode = compilation.begin();

    VkShaderModule output{};
    CHECKCALL(vkCreateShaderModule, device, &create_info, nullptr, &output);
    return output;
}

static char const* VkResultToStr(VkResult _vkResult)
{
    switch(_vkResult)
    {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
    default: return "";
    }
}

} // namespace vktk
