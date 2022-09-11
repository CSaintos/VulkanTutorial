#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <iostream>
#include <optional>
#include <limits>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>

struct VkContext
{
  VkInstance instance;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkSurfaceKHR surface;
  VkSwapchainKHR swap_chain;
  std::vector<VkImage> swap_chain_images;
  VkFormat swap_chain_image_format;
  VkExtent2D swap_chain_extent;
  std::vector<VkImageView> swap_chain_image_views;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool isComplete()
  {
    return graphics_family.has_value() && present_family.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

/**
 * A triangle app
 */
class HelloTriangle 
{
public:
  
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:

  const std::vector<const char*> device_extensions =
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  GLFWwindow *window;
  VkContext context;

  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  void initWindow()
  {
    // Initialize GLFW
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  void createInstance()
  {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;

    //aquireExtensions();
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = 0;

    if (vkCreateInstance(&create_info, nullptr, &context.instance) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create instance!");
    }
  }

  /*
  * Checking for extension support
    - we could possibly get a VK_ERROR_EXTENSION_NOT_PRESENT from vkCreateInstance.
      - Essential extensions like the window system interface could resolve this.
    - To retrieve a list of support extensions before creating an instance:
  */
  void aquireExtensions()
  {
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "available extensions:" << std::endl;

    for (const VkExtensionProperties &extension : extensions)
    {
      std::cout << "\t" << extension.extensionName << std::endl;
    }
  }

  void createSurface()
  {
    if (glfwCreateWindowSurface(context.instance, window, nullptr, &context.surface) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create window surface!");
    }
  }

  bool checkDeviceExtensionSupport(const VkPhysicalDevice &device)
  {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const VkExtensionProperties &extension : available_extensions)
    {
      required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
  }

  SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice &device)
  {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &format_count, nullptr);

    if (format_count != 0)
    {
      details.formats.resize(format_count);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
      details.present_modes.resize(present_mode_count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &present_mode_count, details.present_modes.data());
    }

    return details;
  }

  bool isDeviceSuitable(const VkPhysicalDevice &device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensions_supported = checkDeviceExtensionSupport(device);
    bool swap_chain_adequate = false;
    if (extensions_supported)
    {
      SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
      swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    return indices.isComplete() && extensions_supported && swap_chain_adequate;
  }

  void pickPhysicalDevice()
  {
    uint32_t device_count = 0;

    vkEnumeratePhysicalDevices(context.instance, &device_count, nullptr);

    if (device_count == 0)
    {
      throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(context.instance, &device_count, devices.data());
    
    for (const VkPhysicalDevice &device : devices)
    {
      if (isDeviceSuitable(device))
      {
        context.physical_device = device;
        break;
      }
    }

    if (context.physical_device == VK_NULL_HANDLE)
    {
      throw std::runtime_error("Failed to find a suitable GPU!");
    }
  }

  QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice &device)
  {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const VkQueueFamilyProperties &queue_family : queue_families)
    {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphics_family = i;
      }

      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &present_support);

      if (present_support)
      {
        indices.present_family = i;
      }

      if (indices.isComplete())
      {
        break;
      }

      i++;
    }

    return indices;
  }

  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(context.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

    float queue_priority = 1.0f;

    for (uint32_t queue_family : unique_queue_families)
    {
      VkDeviceQueueCreateInfo queue_create_info{};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queue_priority;

      queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    create_info.enabledLayerCount = 0;

    if (vkCreateDevice(context.physical_device, &create_info, nullptr, &context.device) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(context.device, indices.graphics_family.value(), 0, &context.graphics_queue);
    vkGetDeviceQueue(context.device, indices.present_family.value(), 0, &context.present_queue);
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
  {
    for (const auto &available_format : available_formats)
    {
      if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return available_format;
      }
    }

    // fixme, not proper warning/error handling
    return available_formats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes)
  {
    for (const VkPresentModeKHR &available_present_mode : available_present_modes) {
      if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return available_present_mode;
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
    else
    {
      int width, height;
      glfwGetFramebufferSize(window, &width, &height);

      VkExtent2D actual_extent =
      {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
      };

      actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);

      actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

      return actual_extent;
    }
  }

  void createSwapChain()
  {
    SwapChainSupportDetails swap_chain_support = querySwapChainSupport(context.physical_device);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    {
      image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context.surface;
    // Image details
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(context.physical_device);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

    if (indices.graphics_family != indices.present_family) 
    {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(context.device, &create_info, nullptr, &context.swap_chain) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context.device, context.swap_chain, &image_count, nullptr);
    context.swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(context.device, context.swap_chain, &image_count, context.swap_chain_images.data());

    context.swap_chain_image_format = surface_format.format;
    context.swap_chain_extent = extent;
  }

  void createImageViews()
  {
    context.swap_chain_image_views.resize(context.swap_chain_images.size());

    for (size_t i = 0; i < context.swap_chain_images.size(); i++)
    {
      VkImageViewCreateInfo create_info{};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = context.swap_chain_images[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = context.swap_chain_image_format;
      // swizzle color channels
      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      // describe image purpose and image access
      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;

      if (vkCreateImageView(context.device, &create_info, nullptr, &context.swap_chain_image_views[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create image views!");
      }
    }
  }

  void initVulkan()
  {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
  }

  void mainLoop()
  {
    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();
    }
  }

  void cleanup()
  {
    for (VkImageView image_view : context.swap_chain_image_views)
    {
      vkDestroyImageView(context.device, image_view, nullptr);
    }

    vkDestroySwapchainKHR(context.device, context.swap_chain, nullptr);
    vkDestroyDevice(context.device, nullptr);
    vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
    vkDestroyInstance(context.instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main(int argc, char *argv[])
{
  HelloTriangle ht;

  try 
  {
    ht.run();
    
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}