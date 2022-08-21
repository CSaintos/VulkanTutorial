#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <iostream>
#include <optional>
#include <vector>
#include <stdexcept>
#include <cstdlib>

struct VkContext
{
  VkInstance instance;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphics_queue;
};

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics_family;

  bool isComplete()
  {
    return graphics_family.has_value();
  }
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

  bool isDeviceSuitable(const VkPhysicalDevice &device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
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
        break;
      }

      i++;
    }

    return indices;
  }

  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(context.physical_device);
    float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = indices.graphics_family.value();
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = 0;
    create_info.enabledLayerCount = 0;

    if (vkCreateDevice(context.physical_device, &create_info, nullptr, &context.device) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(context.device, indices.graphics_family.value(), 0, &context.graphics_queue);
  }

  void initVulkan()
  {
    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
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
    vkDestroyDevice(context.device, nullptr);
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