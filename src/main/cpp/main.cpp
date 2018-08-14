#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>

#include <iostream>
#include <limits>
#include <vector>

int main(int argc, char** argv) {
    if (!glfwInit()) {
        throw "GLFW failed to init!";
    }

    if (!glfwVulkanSupported()) {
        throw "Vulkan not supported!";
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

    VkInstance instance = VK_NULL_HANDLE;
    auto ret = vkCreateInstance(&instanceCI, nullptr, &instance);

    if (VK_SUCCESS != ret) {
        throw "Unable to create Vulkan Instance!";
    }

    std::uint32_t nGPUs = 0;
    vkEnumeratePhysicalDevices(instance, &nGPUs, nullptr);
    auto gpus = std::vector<VkPhysicalDevice> (nGPUs);
    vkEnumeratePhysicalDevices(instance, &nGPUs, gpus.data());

    std::uint32_t nQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &nQueueFamilies, nullptr);
    auto familyProperties = std::vector<VkQueueFamilyProperties>(nQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &nQueueFamilies, familyProperties.data());

    std::uint32_t graphicsQueueFamilyId = std::numeric_limits<std::uint32_t>::max();
    for (std::uint32_t i = 0; i < nQueueFamilies; i++) {
        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyId = i;
            break;
        }
    }

    if (graphicsQueueFamilyId == std::numeric_limits<std::uint32_t>::max()) {
        glfwTerminate();
        return -1;
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

    VkDevice device = VK_NULL_HANDLE;
    vkCreateDevice(gpus[0], &deviceCI, nullptr, &device);

    int width = 800;
    int height = 600;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto window = glfwCreateWindow(width, height, appCI.pApplicationName, nullptr, nullptr);

    glfwGetFramebufferSize(window, &width, &height);

    VkSurfaceKHR surface;
    ret = glfwCreateWindowSurface(instance, window, nullptr, &surface);

    if (VK_SUCCESS != ret) {
        throw "Could not init Vulkan surface!";
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwDestroyWindow(window);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwTerminate();

    return 0;
}