#include "volk.h"
#include <GLFW/glfw3.h>

#include <cstdint>

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

struct context {
    GLFWwindow * pWindow;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;

    context();

    ~context();
};

context init();

std::string translateVulkanResult(VkResult result);

std::string translateVulkanResult(VkResult result) {
    switch (result) {
        // Success codes
        case VK_SUCCESS:
            return "Command successfully completed.";
        case VK_NOT_READY:
            return "A fence or query has not yet completed.";
        case VK_TIMEOUT:
            return "A wait operation has not completed in the specified time.";
        case VK_EVENT_SET:
            return "An event is signaled.";
        case VK_EVENT_RESET:
            return "An event is unsignaled.";
        case VK_INCOMPLETE:
            return "A return array was too small for the result.";
        case VK_SUBOPTIMAL_KHR:
            return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "Initialization of an object could not be completed for implementation-specific reasons.";
        case VK_ERROR_DEVICE_LOST:
            return "The logical or physical device has been lost.";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "A requested format is not supported on this device.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the "
                    "swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue"
                    "presenting to the surface.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an"
            " image.";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "A validation layer found an error.";
        default: {
            auto msg = std::stringstream();

            msg << "Unknown VkResult: 0x" << std::hex << result;

            return msg.str();
        }
    }
}

void vkAssert(VkResult result) {
    if (VK_SUCCESS != result) {
        throw std::runtime_error(translateVulkanResult(result));
    }
}

context::context() {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW failed to init!");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Vulkan not supported!");
    }

    if (VK_SUCCESS != volkInitialize()) {
        throw std::runtime_error("Volk could not be initialized!");
    }

    auto instanceLayers = std::vector<const char *> ();
    auto instanceExtensions = std::vector<const char *> ();
    auto deviceExtensions = std::vector<const char *> ();

    instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
    instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    std::uint32_t nInstanceExtensions = 0;
    auto pRequiredInstanceExtensions = glfwGetRequiredInstanceExtensions(&nInstanceExtensions);

    for (std::uint32_t i = 0; i < nInstanceExtensions; i++) {
        instanceExtensions.push_back(pRequiredInstanceExtensions[i]);
        std::cout << "Require extension: " << pRequiredInstanceExtensions[i] << std::endl;
    }

    VkApplicationInfo appCI {};
    appCI.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appCI.apiVersion = VK_MAKE_VERSION(1, 0, 2);
    appCI.applicationVersion = 1;
    appCI.pApplicationName = "GLFW test";
    appCI.pEngineName = "vkcam";
    appCI.engineVersion = 1;

    VkInstanceCreateInfo instanceCI {};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appCI;
    instanceCI.enabledLayerCount = instanceLayers.size();
    instanceCI.ppEnabledLayerNames = instanceLayers.data();
    instanceCI.enabledExtensionCount = instanceExtensions.size();
    instanceCI.ppEnabledExtensionNames = instanceExtensions.data();

    vkAssert(vkCreateInstance(&instanceCI, nullptr, &instance));
    volkLoadInstance(instance);

    std::uint32_t nGPUs = 0;
    vkAssert(vkEnumeratePhysicalDevices(instance, &nGPUs, nullptr));
    auto pGPUs = std::make_unique<VkPhysicalDevice[]>(nGPUs);
    vkAssert(vkEnumeratePhysicalDevices(instance, &nGPUs, pGPUs.get()));

    std::cout << "Available GPUs:\n";
    for (std::uint32_t i = 0; i < nGPUs; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(pGPUs[i], &properties);

        std::cout << "GPU[" << std::dec << i << "]:";
        std::cout << "\n\tName: " << properties.deviceName;
        std::cout << "\n\tType: ";

        switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                std::cout << "other\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "CPU\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "Discrete GPU\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "Integrated GPU\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "Virtual GPU\n";
                break;
            default:
                std::cout << "Unknown\n";
                break;
        }
    }
    std::cout << std::endl;

    //TODO: select the proper GPU
    physicalDevice = pGPUs[0];

    std::uint32_t nQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, nullptr);
    auto familyProperties = std::make_unique<VkQueueFamilyProperties[]> (nQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, familyProperties.get());

    constexpr std::uint32_t INVALID_QUEUE_FAMILY_ID = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t graphicsQueueFamilyId = INVALID_QUEUE_FAMILY_ID;
    for (std::uint32_t i = 0; i < nQueueFamilies; i++) {
        std::cout << "Queue[" << std::dec << i << "]:";
        std::cout << "\n\tQueue Count: " << std::dec << familyProperties[i].queueCount;
        std::cout << "\n\tQueue Flags: ";

        bool hasFlags = false;

        if (familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            std::cout << "COMPUTE";
            hasFlags = true;
        }

        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (hasFlags) {
                std::cout << " | ";
            }

            std::cout << "GRAPHICS";
            hasFlags = true;
            if (INVALID_QUEUE_FAMILY_ID == graphicsQueueFamilyId) {
                graphicsQueueFamilyId = i;
            }
        }

        if (familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (hasFlags) {
                std::cout << " | ";
            }

            std::cout << "TRANSFER";
            hasFlags = true;
        }

        if (familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            if (hasFlags) {
                std::cout << " | ";
            }

            std::cout << "SPARSE BINDING";
            hasFlags = true;
        }

        if (familyProperties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
            if (hasFlags) {
                std::cout << " | ";
            }

            std::cout << "PROTECTED";
            hasFlags = true;
        }
        std::cout << "\n";
    }

    std::cout << std::endl;

    if (INVALID_QUEUE_FAMILY_ID == graphicsQueueFamilyId) {
        throw std::runtime_error("Unable to find Graphics Queue Family!");
    }

    const float priorities[] {1.0F};

    VkDeviceQueueCreateInfo queueCI {};
    queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCI.queueCount = 1;
    queueCI.queueFamilyIndex = graphicsQueueFamilyId;
    queueCI.pQueuePriorities = priorities;

    VkDeviceCreateInfo deviceCI {};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = 1;
    deviceCI.pQueueCreateInfos = &queueCI;
    deviceCI.enabledExtensionCount = deviceExtensions.size();
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

    vkAssert(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
    volkLoadDevice(device);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, appCI.pApplicationName, nullptr, nullptr);

    if (VK_SUCCESS != glfwCreateWindowSurface(instance, pWindow, nullptr, &surface)) {
        throw std::runtime_error("Could not init Vulkan surface!");
    }
}

context::~context() {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwDestroyWindow(pWindow);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}

int main(int argc, char** argv) {
    context ctx;

    std::uint32_t nSurfaceFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &nSurfaceFormats, nullptr);
    auto pSurfaceFormats = std::make_unique<VkSurfaceFormatKHR[]> (nSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &nSurfaceFormats, pSurfaceFormats.get());

    std::cout << "Supported SurfaceFormats:\n";
    for (std::uint32_t i = 0; i < nSurfaceFormats; i++) {
        std::cout << "\tSurfaceFormat[" << std::dec << i << "]:\n";
        std::cout << "\t\tFormat: 0x" << std::hex << pSurfaceFormats[i].format;
        std::cout << "\n\t\tColorSpace: 0x" << std::hex << pSurfaceFormats[i].colorSpace;
        std::cout << "\n";
    }

    std::cout << std::endl;

    return 0;
}