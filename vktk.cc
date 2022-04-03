#include "vktk.hpp"

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{

static char const* VkResultToStr(VkResult _vkResult);

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

#define CHECKCALL(func, ...)                                           \
    {                                                                  \
        VkResult const vkr = func(__VA_ARGS__);                        \
        if (vkr != VK_SUCCESS)                                         \
            std::cout << #func " " << VkResultToStr(vkr) << std::endl; \
    }

#define GetInstanceProcAddress(proc, inst, table)                       \
    table.##proc = (PFN_vk##proc)vkGetInstanceProcAddr(inst, "vk" #proc); \

#define GetDeviceProcAddress(proc, dev, table)                          \
    table.##proc = (PFN_vk##proc)vkGetDeviceProcAddr(dev, "vk" #proc);  \


#define kRequiredInstanceProcs(x)               \
    x(GetPhysicalDeviceSurfaceCapabilitiesKHR)  \
    x(GetPhysicalDeviceSurfaceFormatsKHR)       \
    x(GetPhysicalDeviceSurfaceSupportKHR)       \

#define kRequiredDeviceProcs(x)                 \
    x(CreateSwapchainKHR)                       \
    x(DestroySwapchainKHR)                      \

#define GetInstanceProcAddress_XMacro(proc)     \
    GetInstanceProcAddress(proc, INSTANCE, TABLE)

#define GetDeviceProcAddress_XMacro(proc)       \
    GetDeviceProcAddress(proc, DEVICE, TABLE)

namespace vktk
{

struct FPTable
{
#define AsFPTableEntry(proc) PFN_vk##proc proc = nullptr;
    kRequiredInstanceProcs(AsFPTableEntry)

    kRequiredDeviceProcs(AsFPTableEntry)
#undef AsFPTableEntry
};

struct ContextData
{
    FPTable fp;

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue present_queue;

    VkSwapchainKHR swapchain;
};

Context* CreateContext(std::uint64_t _hinstance, std::uint64_t _hwindow)
{
    ContextData* ctxt = new ContextData{};

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

        CHECKCALL(vkCreateInstance, &instance_info, nullptr, &ctxt->instance);
    }

    #define INSTANCE ctxt->instance
    #define TABLE ctxt->fp
    kRequiredInstanceProcs(GetInstanceProcAddress_XMacro)
    #undef INSTANCE
    #undef TABLE

    {
        uint32_t dev_count = 1u;
        CHECKCALL(vkEnumeratePhysicalDevices, ctxt->instance, &dev_count, &ctxt->physical_device);
        vkGetPhysicalDeviceMemoryProperties(ctxt->physical_device, &ctxt->memory_properties);
    }

#ifdef _WIN32
    {
        VkWin32SurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.hinstance = (HINSTANCE)_hinstance;
        create_info.hwnd = (HWND)_hwindow;
        CHECKCALL(vkCreateWin32SurfaceKHR, ctxt->instance, &create_info, nullptr, &ctxt->surface);
    }
#endif

    {
        std::uint32_t queueFamilyCount = 0u;
        vkGetPhysicalDeviceQueueFamilyProperties(ctxt->physical_device, &queueFamilyCount,
                                                 nullptr);
        std::vector<VkQueueFamilyProperties> queue_properties{ queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(ctxt->physical_device, &queueFamilyCount,
                                                 queue_properties.data());

        std::uint32_t queueIndex = ~0u;
        for (std::uint32_t index = 0u; index < queue_properties.size(); ++index)
        {
            VkBool32 canPresent = VK_FALSE;
            ctxt->fp.GetPhysicalDeviceSurfaceSupportKHR(ctxt->physical_device, index, ctxt->surface,
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

        CHECKCALL(vkCreateDevice, ctxt->physical_device, &device_info, nullptr, &ctxt->device);

        vkGetDeviceQueue(ctxt->device, queueIndex, 0, &ctxt->present_queue);
    }

    #define DEVICE ctxt->device
    #define TABLE ctxt->fp
    kRequiredDeviceProcs(GetDeviceProcAddress_XMacro)
    #undef DEVICE
    #undef TABLE

    {
        VkFormat swapchain_format{};
        VkColorSpaceKHR color_space{};
        {
            std::uint32_t format_count = 1;
            VkSurfaceFormatKHR surface_format{};
            ctxt->fp.GetPhysicalDeviceSurfaceFormatsKHR(ctxt->physical_device,
                                                        ctxt->surface,
                                                        &format_count, &surface_format);

            swapchain_format = surface_format.format;
            if (surface_format.format == VK_FORMAT_UNDEFINED)
                swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;

            color_space = surface_format.colorSpace;
        }

        VkSurfaceCapabilitiesKHR surface_capabilities{};
        ctxt->fp.GetPhysicalDeviceSurfaceCapabilitiesKHR(ctxt->physical_device,
                                                         ctxt->surface,
                                                         &surface_capabilities);

        VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
        if (surface_capabilities.currentExtent.width = (uint32_t)-1)
            swapchain_extent = surface_capabilities.minImageExtent;

        VkSwapchainCreateInfoKHR swapchain_info{};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.pNext = nullptr;
        swapchain_info.flags = 0;
        swapchain_info.surface = ctxt->surface;
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

        CHECKCALL(ctxt->fp.CreateSwapchainKHR, ctxt->device, &swapchain_info, nullptr, &ctxt->swapchain);
    }

    return (Context*)ctxt;
}

void DeleteContext(Context* _context)
{
    ContextData* ctxt = (ContextData*)_context;
    ctxt->fp.DestroySwapchainKHR(ctxt->device, ctxt->swapchain, nullptr);
    vkDestroyDevice(ctxt->device, nullptr);
#ifdef _WIN32
    vkDestroySurfaceKHR(ctxt->instance, ctxt->surface, nullptr);
#endif
    vkDestroyInstance(ctxt->instance, nullptr);
    delete ctxt;
}

} // namespace vktk

namespace {

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

} // namespace
