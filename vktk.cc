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

} // namespace

#define CHECKCALL(func, ...)                                           \
    {                                                                  \
        VkResult const vkr = func(__VA_ARGS__);                        \
        if (vkr != VK_SUCCESS)                                         \
            std::cout << #func " " << VkResultToStr(vkr) << std::endl; \
    }

#define kRequiredInstanceProcs(x, inst, table)          \
    x(GetPhysicalDeviceSurfaceSupportKHR, inst, table)

#define GET_INSTANCE_PROC_ADDR(proc, inst, table)                 \
    table.##proc =                                                \
        (PFN_vk##proc)vkGetInstanceProcAddr(inst, "vk" #proc);    \

#define GET_DEVICE_PROC_ADDR(proc, dev, table)                    \
    table.##proc =                                                \
        (PFN_vk##proc)vkGetDeviceProcAddr(dev, "vk" #proc);       \

#define FPTableEntry(proc, inst, table)         \
    PFN_vk##proc proc = nullptr;

namespace vktk
{

static struct FPTable
{
    kRequiredInstanceProcs(FPTableEntry, , )
} fp;

void CreateContext(std::uint64_t _hinstance, std::uint64_t _hwindow)
{
    VkInstance instance{};
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

    kRequiredInstanceProcs(GET_INSTANCE_PROC_ADDR, instance, fp);

    VkPhysicalDevice physical_device{};
    {
        uint32_t dev_count = 1u;
        CHECKCALL(vkEnumeratePhysicalDevices, instance, &dev_count, &physical_device);
    }

#ifdef _WIN32
    VkSurfaceKHR surface{};
    {
        VkWin32SurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.hinstance = (HINSTANCE)_hinstance;
        create_info.hwnd = (HWND)_hwindow;
        CHECKCALL(vkCreateWin32SurfaceKHR, instance, &create_info, nullptr, &surface);
    }
#else
#endif

    VkDevice device{};
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
        device_info.enabledExtensionCount = 0;
        device_info.ppEnabledExtensionNames = nullptr;
        device_info.pEnabledFeatures = nullptr;

        CHECKCALL(vkCreateDevice, physical_device, &device_info, nullptr, &device);
    }

    vkDestroyDevice(device, nullptr);
#ifdef _WIN32
    vkDestroySurfaceKHR(instance, surface, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);
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
