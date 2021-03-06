#include <iostream>
#include <set>

#include "engine.h"

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#elif !defined(NDEBUG)
PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugUtilsMessengerEXT *pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const *pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}
#endif

namespace Engine
{

vk::UniqueInstance createInstance(std::string const &appName, std::string const &engineName,
                                  vk::InstanceCreateInfo &createdCreateInfo, std::vector<std::string> const &layers,
                                  std::vector<std::string> const &extensions, uint32_t apiVersion)
{
#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    static vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

    std::vector<char const *> enabledLayers;
    enabledLayers.reserve(layers.size());
    for (auto const &layer : layers)
    {
        enabledLayers.push_back(layer.data());
    }
#if !defined(NDEBUG)
    // Enable standard validation layer to find as much errors as possible!
    if (std::find(layers.begin(), layers.end(), "VK_LAYER_KHRONOS_validation") == layers.end())
    {
        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
#endif

    std::vector<char const *> enabledExtensions;
    enabledExtensions.reserve(extensions.size());
    for (auto const &ext : extensions)
    {
        enabledExtensions.push_back(ext.data());
    }
    enabledExtensions.push_back("VK_KHR_surface");
#if VK_USE_PLATFORM_XCB_KHR
    enabledExtensions.push_back("VK_KHR_xcb_surface");
#endif
#if VK_USE_PLATFORM_WIN32_KHR
    enabledExtensions.push_back("VK_KHR_win32_surface");
    enabledExtensions.push_back("VK_KHR_display");
    enabledExtensions.push_back("VK_KHR_get_display_properties2");
    enabledExtensions.push_back("VK_KHR_get_surface_capabilities2");
    enabledExtensions.push_back("VK_KHR_surface");
#endif
#if !defined(NDEBUG)
    if (std::find(extensions.begin(), extensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extensions.end())
    {
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#endif

    for (auto &layer : enabledLayers)
        std::cout << layer;

    // create a UniqueInstance
    vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, apiVersion);
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, checked_cast<uint32_t>(enabledLayers.size()),
                                              enabledLayers.data(), checked_cast<uint32_t>(enabledExtensions.size()),
                                              enabledExtensions.data());
    vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

    createdCreateInfo = instanceCreateInfo;

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    // initialize function pointers for instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
#else
#if !defined(NDEBUG)
    static bool initialized = false;
    if (!initialized)
    {
        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
        assert(pfnVkCreateDebugUtilsMessengerEXT && pfnVkDestroyDebugUtilsMessengerEXT);
        initialized = true;
    }
#endif
#endif

    return instance;
}
#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                    void *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

vk::UniqueDebugUtilsMessengerEXT createDebugUtilsMessenger(vk::UniqueInstance &instance)
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    return instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, &debugCallback));
}
#endif

bool checkDeviceExtensionSupport(vk::PhysicalDevice phyDev)
{
    std::vector<vk::ExtensionProperties> availExts = phyDev.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &ext : availExts)
        requiredExtensions.erase(ext.extensionName);

    return requiredExtensions.empty();
}

inline void querySwapchainSupport(vk::PhysicalDevice phyDev, vk::SurfaceKHR surface,
                                  SwapChainSupportDetails &swapchainSupportDetails)
{
    swapchainSupportDetails.capabilities = phyDev.getSurfaceCapabilitiesKHR(surface);
    swapchainSupportDetails.formats = phyDev.getSurfaceFormatsKHR(surface);
    swapchainSupportDetails.presentModes = phyDev.getSurfacePresentModesKHR(surface);
}

inline bool isPhyDevSuitable(vk::PhysicalDevice phyDev, vk::SurfaceKHR surface,
                             SwapChainSupportDetails &swapchainSupportDetails)
{
    vk::PhysicalDeviceProperties phyDevProps = phyDev.getProperties();
    vk::PhysicalDeviceFeatures phyDevFeats = phyDev.getFeatures();

    querySwapchainSupport(phyDev, surface, swapchainSupportDetails);

    bool deviceSupportsExts = checkDeviceExtensionSupport(phyDev);
    bool swapchainSupported = !swapchainSupportDetails.formats.empty() && !swapchainSupportDetails.presentModes.empty();

    return phyDevProps.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && phyDevFeats.geometryShader &&
           phyDevFeats.multiViewport && deviceSupportsExts && swapchainSupported;
}

uint32_t Engine::GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < m_phyDevMemProps.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((m_phyDevMemProps.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

void Engine::InitVulkan(std::vector<std::string> const &extraExtensions)
{
#ifndef NDEBUG
    SetupDebugMessenger();
#endif

#ifdef _WIN32
    if (extraExtensions.size() == 0)
    {
        vk::Win32SurfaceCreateInfoKHR win32SurfaceCreateInfo(vk::Win32SurfaceCreateFlagsKHR(), GetModuleHandle(NULL),
                                                             m_window);
        m_surface = m_instance->createWin32SurfaceKHR(win32SurfaceCreateInfo);
    }
#else
    if (extraExtensions.size() == 0)
    {
        vk::XcbSurfaceCreateInfoKHR xcbSurfaceCreateInfo(vk::XcbSurfaceCreateFlagBitsKHR(), m_xConnection, m_window);
        m_surface = m_instance->createXcbSurfaceKHR(xcbSurfaceCreateInfo);
    }
#endif

    std::vector<vk::PhysicalDevice> phyDevs = m_instance->enumeratePhysicalDevices();

    for (const auto &phyDev : phyDevs)
    {
        if (isPhyDevSuitable(phyDev, m_surface, m_swapchainSupportDetails))
        {
            m_phyDev = phyDev;
            break;
        }
    }

    if (!m_phyDev)
    {
        std::cerr << "No suitable device found" << std::endl;
        return;
    }

    m_phyDevMemProps = m_phyDev.getMemoryProperties();
    m_phyDevProps = m_phyDev.getProperties();

    if (extraExtensions.size() == 0)
        SetupDevice();
}

void Engine::SetupDevice()
{
    FindQueueFamilies();
    CreateQueues();
    CreateSwapChain();
    CreateImageViews();
    CreateDepthStencil();
    CreateCommandPools();
}

void Engine::ChooseSwapSurfaceFormat()
{
    for (const auto &availFormat : m_swapchainSupportDetails.formats)
    {
        if (availFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            m_swapchainFormat = availFormat;
    }

    m_swapchainFormat = m_swapchainSupportDetails.formats[0];
}

void Engine::ChooseSwapExtent()
{
    vk::SurfaceCapabilitiesKHR caps = m_swapchainSupportDetails.capabilities;
    if (caps.currentExtent.width != UINT32_MAX)
        m_extent = caps.currentExtent;
    else
    {
        vk::Extent2D actualExtent = {m_windowExtents.width, m_windowExtents.height};

        actualExtent.width =
            std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, actualExtent.width));
        actualExtent.height =
            std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, actualExtent.height));

        m_extent = actualExtent;
    }
}

void Engine::CreateSwapChain()
{
    querySwapchainSupport(m_phyDev, m_surface, m_swapchainSupportDetails);

    ChooseSwapSurfaceFormat();
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
    ChooseSwapExtent();

    uint32_t imageCount = m_swapchainSupportDetails.capabilities.minImageCount + 1;
    if (m_swapchainSupportDetails.capabilities.maxImageCount > 0 &&
        imageCount > m_swapchainSupportDetails.capabilities.maxImageCount)
        imageCount = m_swapchainSupportDetails.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo(vk::SwapchainCreateFlagsKHR(), m_surface, imageCount,
                                                   m_swapchainFormat.format, m_swapchainFormat.colorSpace, m_extent, 1,
                                                   vk::ImageUsageFlagBits::eColorAttachment);

    uint32_t queueFamilyIndices[] = {m_graphicsQueueFamilyIndex, m_presentQueueFamilyIndex};

    if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
    {
        // TODO: Figure out the performance method of image sharing
        // for multi device rendering and the rare case where one device
        // uses two queue families.
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    swapchainCreateInfo.preTransform = m_swapchainSupportDetails.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    swapchainCreateInfo.oldSwapchain = m_swapchain;

    m_swapchain = m_device->createSwapchainKHR(swapchainCreateInfo);

    m_swapchainImages.clear();
    auto swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain);
    for (auto swapchainImage : swapchainImages)
        m_swapchainImages.push_back(SwapchainImage_t{swapchainImage});

    m_framebufferCount = m_swapchainImages.size();

    m_framebuffers = std::vector<vk::Framebuffer>(m_framebufferCount);
}

void Engine::CreateImageViews()
{
    m_imageViews.resize(m_swapchainImages.size());

    for (auto &swapchainImage : m_swapchainImages)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo(
            vk::ImageViewCreateFlags(), swapchainImage.image, vk::ImageViewType::e2D, m_swapchainFormat.format,
            vk::ComponentMapping(/*r*/ vk::ComponentSwizzle::eIdentity,
                                 /*g*/ vk::ComponentSwizzle::eIdentity,
                                 /*b*/ vk::ComponentSwizzle::eIdentity,
                                 /*a*/ vk::ComponentSwizzle::eIdentity),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        swapchainImage.view = m_device->createImageView(imageViewCreateInfo);
        m_imageViews.push_back(swapchainImage.view);
    }
}

void Engine::FindQueueFamilies()
{
    std::vector<vk::QueueFamilyProperties> queueFamilies = m_phyDev.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            m_graphicsQueueFamilyIndex = i;
            m_graphicsQueuesAvailable = queueFamily.queueCount - 1;
            m_graphicsQueuesMax = queueFamily.queueCount;
        }
        // Search for a dedicated compute queue family
        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute &&
            queueFamily.queueFlags ^ vk::QueueFlagBits::eGraphics)
        {
            m_computeQueueFamilyIndex = i;
            m_computeQueuesAvailable = queueFamily.queueCount;
            m_computeQueuesMax = queueFamily.queueCount;
        }
        vk::Bool32 presentSupport = m_phyDev.getSurfaceSupportKHR(i, m_surface);
        if (presentSupport)
            m_presentQueueFamilyIndex = i;
        i++;
    }
}

// TODO: Move to EnginePipeline
void Engine::CreateQueues()
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    float controlQueuePriority = 1.0F;
    float otherQueuePriority = 1.0F / m_graphicsQueuesAvailable;

    float *graphicsQueuePriorities = (float *)malloc(sizeof(float) * m_graphicsQueuesMax);

    graphicsQueuePriorities[0] = controlQueuePriority;
    for (uint32_t i = 1; i < m_graphicsQueuesAvailable; i++)
        graphicsQueuePriorities[i] = otherQueuePriority;

    vk::DeviceQueueCreateInfo graphicsQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_graphicsQueueFamilyIndex,
                                                      m_graphicsQueuesAvailable, graphicsQueuePriorities);
    queueCreateInfos.push_back(graphicsQueueCreateInfo);

    float *computeQueuePriorities = (float *)malloc(sizeof(float) * m_computeQueuesAvailable);

    for (uint32_t i = 0; i < m_computeQueuesAvailable; i++)
        computeQueuePriorities[i] = 1.0F;

    vk::DeviceQueueCreateInfo computeQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_computeQueueFamilyIndex,
                                                     m_computeQueuesAvailable, computeQueuePriorities);
    queueCreateInfos.push_back(computeQueueCreateInfo);

    if (m_presentQueueFamilyIndex != m_computeQueueFamilyIndex)
    {
        float queuePriority = 1.0F;

        vk::DeviceQueueCreateInfo presentQueueCreateInfo(vk::DeviceQueueCreateFlags(), m_presentQueueFamilyIndex, 1,
                                                         &queuePriority);
        queueCreateInfos.push_back(presentQueueCreateInfo);
    }

    /*vk::PhysicalDeviceFeatures requestedFeatures = {};
    requestedFeatures.geometryShader = VK_TRUE;
    requestedFeatures.multiViewport = VK_TRUE;*/

    vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos.size(), queueCreateInfos.data());
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    m_device = m_phyDev.createDeviceUnique(deviceCreateInfo);
    m_graphicsQueue = m_device.get().getQueue(m_graphicsQueueFamilyIndex, 0);
    m_computeQueue = m_device.get().getQueue(m_computeQueueFamilyIndex, 0);

    m_presentQueue = m_device.get().getQueue(m_presentQueueFamilyIndex, 0);
}
// TODO: Move to EnginePipeline
void Engine::CreateCommandPools()
{
    vk::CommandPoolCreateInfo graphicsCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_graphicsQueueFamilyIndex);
    vk::CommandPoolCreateInfo computeCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_computeQueueFamilyIndex);

    m_graphicsCommandPool = m_device.get().createCommandPool(graphicsCmdPoolCreateInfo);
    m_computeCommandPool = m_device.get().createCommandPool(computeCmdPoolCreateInfo);

    vk::CommandPoolCreateInfo presentCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_presentQueueFamilyIndex);
    m_presentCommandPool = m_device.get().createCommandPool(presentCmdPoolCreateInfo);
}

/*
void Engine::QueueCommandBuffer(vk::UniqueSemaphore& imageAcqSemaphore) {

}
*/
void Engine::CreateDepthStencil()
{

    vk::ImageCreateInfo depthStencilImageCreateInfo{vk::ImageCreateFlags(),
                                                    vk::ImageType::e2D,
                                                    kDepthFormat,
                                                    vk::Extent3D{m_windowExtents.width, m_windowExtents.height, 1},
                                                    1,
                                                    1,
                                                    vk::SampleCountFlagBits::e1,
                                                    vk::ImageTiling::eOptimal,
                                                    vk::ImageUsageFlagBits::eDepthStencilAttachment};

    m_depthImage = m_device->createImage(depthStencilImageCreateInfo);

    vk::MemoryRequirements memReqs = m_device->getImageMemoryRequirements(m_depthImage);

    vk::MemoryAllocateInfo newImageMemAlloc{
        memReqs.size, GetMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};

    vk::DeviceMemory newImageDevMem = m_device->allocateMemory(newImageMemAlloc);

    m_devMemAllocs.push_back(newImageDevMem);

    m_device->bindImageMemory(m_depthImage, newImageDevMem, 0);

    vk::ImageViewCreateInfo newImageViewCreateInfo{
        vk::ImageViewCreateFlags{},
        m_depthImage,
        vk::ImageViewType::e2D,
        kDepthFormat,
        vk::ComponentMapping{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
                             vk::ComponentSwizzle::eA},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}};

    m_depthImageView = m_device->createImageView(newImageViewCreateInfo);
}

} // namespace Engine