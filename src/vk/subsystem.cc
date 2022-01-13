#include "vk/subsystem.h"

#include <iostream>
#include <set>

#include "common.h"

#include "engine.h"
#include "log.h"

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

namespace EnvGraph
{

constexpr const uint64_t kFenceTimeout = 100000000;

GraphicsSubsystem<GpuApiSetting::VULKAN>::GraphicsSubsystem()
{
}

GraphicsSubsystem<GpuApiSetting::VULKAN>::~GraphicsSubsystem()
{
    DeInit();
}

bool VkSubsystem::Init(Engine *engine)
{
    if (!m_initialized)
    {
        m_parentEngine = engine;
        return InitVulkan();
    }
    else
        return true;
}

void VkSubsystem::DeInit()
{
    if (m_initialized)
    {
        for (const auto &framebuffer : m_framebuffers)
            m_device->destroyFramebuffer(framebuffer);

        for (const auto &imageView : m_imageViews)
        {
            m_device->destroyImageView(imageView);
        }
        m_device->destroyImage(m_depthImage);
        m_device->destroyImageView(m_depthImageView);

        for (const auto &devMem : m_deviceMemBufferAllocs)
        {
            m_device->unmapMemory(devMem);
            m_device->freeMemory(devMem);
        }

        for (const auto &devMem : m_deviceMemAllocs)
        {
            m_device->freeMemory(devMem);
        }

        m_device->destroyCommandPool(m_graphicsCommandPool);
        m_device->destroyCommandPool(m_computeCommandPool);
        m_device->destroyCommandPool(m_presentCommandPool);

        m_device->destroySwapchainKHR(m_swapchain);

        m_instance->destroySurfaceKHR(m_surface);
    }
}

void VkSubsystem::Start()
{
    m_enabled = true;
    m_renderThread = std::thread([this] {
        auto clearValues = std::vector<vk::ClearValue>(2);

        clearValues[0].color.float32[0] = 0.2f;
        clearValues[0].color.float32[1] = 0.2f;
        clearValues[0].color.float32[2] = 0.2f;
        clearValues[0].color.float32[3] = 0.2f;
        clearValues[1].depthStencil.depth = 1.0f;
        clearValues[1].depthStencil.stencil = 0;

        vk::SemaphoreCreateInfo imageAcquiredSemaphoreCI{vk::SemaphoreCreateFlags{0}};

        vk::UniqueSemaphore imageAcquiredSemaphore = m_device->createSemaphoreUnique(imageAcquiredSemaphoreCI);
        auto renderRes = m_parentEngine->GetRenderResolution();

        vk::RenderPassBeginInfo renderPassBI{
            m_renderPass.get(), {}, vk::Rect2D{{0, 0}, {renderRes.x, renderRes.y}}, clearValues};

        vk::Viewport viewport{0, 0, (float)renderRes.x, (float)renderRes.y, 0.0f, 1.0f};

        vk::Rect2D scissor{vk::Offset2D{0, 0}, vk::Extent2D{renderRes.x, renderRes.y}};

        vk::PresentInfoKHR present;
        present.swapchainCount = 1;
        present.waitSemaphoreCount = 0;
        present.pWaitSemaphores = nullptr;
        present.pResults = nullptr;

        vk::Result presentResult;

        vk::SubmitInfo submitInfo;

        vk::UniqueFence graphicsCommandBufferFence = m_device->createFenceUnique(vk::FenceCreateInfo{});

        vk::UniqueFence computeCommandBufferFence = m_device->createFenceUnique(vk::FenceCreateInfo{});

        std::vector<vk::ImageView> attachments(2);
        attachments[1] = m_depthImageView;

        vk::FramebufferCreateInfo framebufferCI{
            vk::FramebufferCreateFlags{}, m_renderPass.get(), attachments, renderRes.x, renderRes.y, 1};

        for (size_t i = 0; i < m_framebufferCount; i++)
        {
            attachments[0] = m_swapchainImages[i].view;
            m_framebuffers[i] = m_device->createFramebuffer(framebufferCI);
        }

        while (m_enabled)
        {
            vk::CommandBufferAllocateInfo graphicsCommandBufferAI{m_graphicsCommandPool,
                                                                  vk::CommandBufferLevel::ePrimary, 1};

            vk::UniqueCommandBuffer graphicsCommandBuffer;
            graphicsCommandBuffer.swap(m_device->allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

            graphicsCommandBuffer->begin(vk::CommandBufferBeginInfo{});

            for (auto enginePipeline : m_pipelines)
            {
                if (enginePipeline->HasPreRenderStage())
                    enginePipeline->PreRender({m_device, renderRes});
            }

            if (vk::Result::eSuccess == m_device->acquireNextImageKHR(m_swapchain, UINT64_MAX,
                                                                      imageAcquiredSemaphore.get(), nullptr,
                                                                      &m_currentFramebuffer))
            {
                renderPassBI.framebuffer = m_framebuffers[m_currentFramebuffer];

                graphicsCommandBuffer->beginRenderPass(renderPassBI, vk::SubpassContents::eInline);

                graphicsCommandBuffer->setViewport(0, viewport);

                graphicsCommandBuffer->setScissor(0, scissor);

                for (auto enginePipeline : m_pipelines)
                {
                    const Pipelines::QueueRequirements_t queueReqs = enginePipeline->GetQueueRequirements();
                    if (queueReqs.compute > 0)
                        enginePipeline->Compute({m_device});
                    if (queueReqs.graphics > 0)
                        enginePipeline->Render({m_device, graphicsCommandBuffer, renderRes});
                }

                graphicsCommandBuffer->endRenderPass();

                graphicsCommandBuffer->end();

                constexpr vk::PipelineStageFlags pipelineStageFlags[1] = {
                    vk::PipelineStageFlagBits::eColorAttachmentOutput};

                submitInfo = {1,      &(imageAcquiredSemaphore.get()), pipelineStageFlags,
                              1,      &(graphicsCommandBuffer.get()),  0,
                              nullptr};

                m_graphicsQueue.submit(submitInfo, graphicsCommandBufferFence.get());

                vk::Result res;
                do
                {
                    res = m_device->waitForFences(graphicsCommandBufferFence.get(), VK_TRUE, kFenceTimeout);
                } while (res == vk::Result::eTimeout);
                m_device->resetFences(graphicsCommandBufferFence.get());

                present.pSwapchains = &m_swapchain;
                present.pImageIndices = &m_currentFramebuffer;

                try
                {
                    presentResult = m_presentQueue.presentKHR(present);
                }
                catch (const vk::OutOfDateKHRError &)
                {
                    m_enabled = false;
                    break;
                }

                if (presentResult != vk::Result::eSuccess)
                {
                    std::cout << "Error: " << vk::to_string(presentResult) << std::endl;
                }
            }
        }
    });
}

void VkSubsystem::Stop()
{
}

void VkSubsystem::NewPipeline(Pipelines::Pipeline *newPipeline)
{
    newPipeline->Setup({m_device, m_phyDeviceMemProps, m_graphicsQueueFamilyIndex, m_computeQueueFamilyIndex,
                        m_graphicsQueuesMax - m_graphicsQueuesAvailable, m_computeQueuesMax - m_computeQueuesAvailable,
                        m_renderPass});
    const auto queueReqs = newPipeline->GetQueueRequirements();

    m_graphicsQueuesAvailable -= queueReqs.graphics;
    m_computeQueuesAvailable -= queueReqs.compute;

    m_pipelines.push_back(newPipeline);
}

void VkSubsystem::SetupRenderPass()
{
    vk::SemaphoreCreateInfo imageAcquiredSemaphoreCI{vk::SemaphoreCreateFlags{0}};

    vk::UniqueSemaphore imageAcquiredSemaphore = m_device->createSemaphoreUnique(imageAcquiredSemaphoreCI);

    if (vk::Result::eSuccess ==
        m_device->acquireNextImageKHR(m_swapchain, UINT64_MAX, imageAcquiredSemaphore.get(), nullptr).result)
    {
        std::vector<vk::AttachmentDescription> attachments{
            {vk::AttachmentDescriptionFlags{}, m_swapchainFormat.format, vk::SampleCountFlagBits::e1,
             vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
             vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR},
            {vk::AttachmentDescriptionFlags{}, kDepthFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
             vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
             vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal}};

        vk::AttachmentReference colorRef{0, vk::ImageLayout::eColorAttachmentOptimal};
        vk::AttachmentReference depthRef{1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

        vk::SubpassDescription subpassDesc{vk::SubpassDescriptionFlags{},
                                           vk::PipelineBindPoint::eGraphics,
                                           0,
                                           nullptr,
                                           1,
                                           &colorRef,
                                           nullptr,
                                           &depthRef,
                                           0,
                                           nullptr};

        vk::SubpassDependency subpassDep{VK_SUBPASS_EXTERNAL,
                                         0,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                         vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                         vk::AccessFlags{0},
                                         vk::AccessFlagBits::eColorAttachmentWrite,
                                         vk::DependencyFlags{}};

        vk::RenderPassCreateInfo renderPassCI{vk::RenderPassCreateFlags{}, attachments, subpassDesc, subpassDep};

        m_renderPass = m_device->createRenderPassUnique(renderPassCI);
    }
}

void VkSubsystem::EnableRenderPass()
{
}

void VkSubsystem::DisableRenderPass()
{
}

void VkSubsystem::EnableComputePass()
{
}

void VkSubsystem::DisableComputePass()
{
}

bool VkSubsystem::CheckRenderResolutionLimits(Extent renderRes) const
{
    return (renderRes.x <= m_phyDeviceProps.limits.maxImageDimension2D) &&
           (renderRes.y <= m_phyDeviceProps.limits.maxImageDimension2D);
}

void UpdateRenderRes()
{

}

bool checkDeviceExtensionSupport(vk::PhysicalDevice phyDev)
{
    std::vector<vk::ExtensionProperties> availExts = phyDev.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(kRequiredDeviceExtensions.begin(), kRequiredDeviceExtensions.end());

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

inline bool isPhyDeviceSuitable(vk::PhysicalDevice phyDev, vk::SurfaceKHR &surface,
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

bool VkSubsystem::InitVulkan()
{
    
    if (!CreateInstance())
        return false;
#ifndef NDEBUG
    SetupDebugUtils();
#endif

    vk::Win32SurfaceCreateInfoKHR win32SurfaceCreateInfo(vk::Win32SurfaceCreateFlagsKHR(), GetModuleHandle(NULL),
                                                         m_parentEngine->GetView()->GetWindow());
    m_surface = m_instance->createWin32SurfaceKHR(win32SurfaceCreateInfo);
    /*VkSurfaceKHR surface;
    if (m_parentEngine->GetView()->GetVkSurface(m_instance.get(), &surface))
    {
        m_surface = surface;
    }
    else
    {
        std::cerr << "Failed to get VkSurface from UI::View" << std::endl;
        return false;
    }*/

    std::vector<vk::PhysicalDevice> phyDevs = m_instance->enumeratePhysicalDevices();

    for (const auto &phyDev : phyDevs)
    {
        if (isPhyDeviceSuitable(phyDev, m_surface, m_swapchainSupportDetails))
        {
            m_phyDevice = phyDev;
            break;
        }
    }

    if (!m_phyDevice)
    {
        std::cerr << "No suitable device found" << std::endl;
        return false;
    }

    m_phyDeviceMemProps = m_phyDevice.getMemoryProperties();
    m_phyDeviceProps = m_phyDevice.getProperties();

    if (SetupDevice())
    {
        SetupRenderPass();
        m_initialized = true;
        return true;
    }
    else
    {
        return false;
    }
}

inline void checked_vector_add(std::vector<char const *> &v, char const * s)
{
    if (std::find(v.begin(), v.end(), s) ==
        v.end())
    {
        v.push_back(s);
    }
}

bool VkSubsystem::CreateInstance()
{
#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    static vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

    std::vector<char const *> enabledLayers;
    enabledLayers.reserve(m_extraInstanceLayers.size());
    for (auto const &layer : m_extraInstanceLayers)
    {
        enabledLayers.push_back(layer.data());
    }
#if !defined(NDEBUG)
    // Enable standard validation layer
    checked_vector_add(enabledLayers, "VK_LAYER_KHRONOS_validation");
#endif

    std::vector<char const *> enabledExtensions;

    // Extra extensions
    enabledExtensions.reserve(m_extraInstanceExtensions.size());
    for (auto const &ext : m_extraInstanceExtensions)
    {
        enabledExtensions.push_back(ext.data());
    }

    // Default engine extensions
    enabledExtensions.push_back("VK_KHR_surface");
#if VK_USE_PLATFORM_XLIB_KHR
    enabledExtensions.push_back("VK_KHR_xlib_surface");
#endif
#if VK_USE_PLATFORM_WIN32_KHR
    enabledExtensions.push_back("VK_KHR_win32_surface");
    enabledExtensions.push_back("VK_KHR_display");
    enabledExtensions.push_back("VK_KHR_get_display_properties2");
    enabledExtensions.push_back("VK_KHR_get_surface_capabilities2");
#endif

#if !defined(NDEBUG)
    checked_vector_add(enabledExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // TODO: Add engine logging
    // for (auto &layer : enabledLayers)
    //    std::cout << layer << std::endl;

    // create a UniqueInstance
    vk::ApplicationInfo applicationInfo(m_parentEngine->GetAppName().c_str(), 1, kEngineName, 1, VK_API_VERSION_1_1);
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, (uint32_t)enabledLayers.size(),
                                              enabledLayers.data(), (uint32_t)enabledExtensions.size(),
                                              enabledExtensions.data());
    try {  
        m_instance = vk::createInstanceUnique(instanceCreateInfo);
    }
    catch (vk::SystemError &e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
    // initialize function pointers for instance
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
#else
#if !defined(NDEBUG)
    static bool initialized = false;
    if (!initialized)
    {
        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            m_instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            m_instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
        assert(pfnVkCreateDebugUtilsMessengerEXT && pfnVkDestroyDebugUtilsMessengerEXT);
        initialized = true;
    }
#endif
#endif
    return true;
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

void VkSubsystem::SetupDebugUtils()
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, &debugCallback));
}
#endif

bool VkSubsystem::SetupDevice()
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    if (!SetupQueues(queueCreateInfos))
        return false;

    vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), (uint32_t)queueCreateInfos.size(),
                                          queueCreateInfos.data());
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    try {
        m_device = m_phyDevice.createDeviceUnique(deviceCreateInfo);
    }
    catch (vk::SystemError& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }
    m_graphicsQueue = m_device.get().getQueue(m_graphicsQueueFamilyIndex, 0);
    m_computeQueue = m_device.get().getQueue(m_computeQueueFamilyIndex, 0);
    m_presentQueue = m_device.get().getQueue(m_presentQueueFamilyIndex, 0);

    if (!CreateSwapChain())
        return false;
    CreateImageViews();
    CreateDepthStencil();
    CreateSharedCommandPools();
    return true;
}

bool VkSubsystem::SetupQueues(std::vector<vk::DeviceQueueCreateInfo> &queueCreateInfos)
{
    std::vector<vk::QueueFamilyProperties> queueFamilies = m_phyDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            m_graphicsQueueFamilyIndex = i;
            m_graphicsQueuesAvailable = queueFamily.queueCount;
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
        vk::Bool32 presentSupport = m_phyDevice.getSurfaceSupportKHR(i, m_surface);
        if (presentSupport)
            m_presentQueueFamilyIndex = i;
        i++;
    }

    if (m_presentQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED)
    {
        std::cerr << "Could not find a present capable queue" << std::endl;
        return false;
    }

    //float controlQueuePriority = 1.0F;
    //float otherQueuePriority = 1.0F / m_graphicsQueuesAvailable;
    float queuePriority = 1.0F;

    float *graphicsQueuePriorities = (float *)malloc(sizeof(float) * m_graphicsQueuesAvailable);

    //graphicsQueuePriorities[0] = queuePriority;
    for (uint32_t i = 0; i < m_graphicsQueuesAvailable; i++)
        graphicsQueuePriorities[i] = queuePriority;

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

    return true;
}

void VkSubsystem::ChooseSwapChainSurfaceFormat()
{
    for (const auto &availFormat : m_swapchainSupportDetails.formats)
    {
        if (availFormat.format == vk::Format::eB8G8R8A8Unorm &&
            availFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            m_swapchainFormat = availFormat;
    }

    m_swapchainFormat = m_swapchainSupportDetails.formats[0];
}

void VkSubsystem::ChooseSwapChainExtent()
{
    vk::SurfaceCapabilitiesKHR caps = m_swapchainSupportDetails.capabilities;

    if (caps.currentExtent.width != UINT32_MAX)
    {
        m_parentEngine->SetRenderResolution({caps.currentExtent.width, caps.currentExtent.height});
    }
    else
    {
        // If the surface extent wasn't already set (by SDL, for example), retrieve the extents of the view and resize the surface
        Extent actualExtent = m_parentEngine->GetView()->GetExtent();

        actualExtent.x = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, actualExtent.x));
        actualExtent.y = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, actualExtent.y));

        m_parentEngine->SetRenderResolution(actualExtent);
    }
}

bool VkSubsystem::CreateSwapChain()
{
    querySwapchainSupport(m_phyDevice, m_surface, m_swapchainSupportDetails);

    ChooseSwapChainSurfaceFormat();
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
    ChooseSwapChainExtent();

    uint32_t imageCount = m_swapchainSupportDetails.capabilities.minImageCount + 1;
    if (m_swapchainSupportDetails.capabilities.maxImageCount > 0 &&
        imageCount > m_swapchainSupportDetails.capabilities.maxImageCount)
        imageCount = m_swapchainSupportDetails.capabilities.maxImageCount;

    auto renderRes = m_parentEngine->GetRenderResolution();

    vk::SwapchainCreateInfoKHR swapchainCreateInfo(
        vk::SwapchainCreateFlagsKHR(), m_surface, imageCount, m_swapchainFormat.format, m_swapchainFormat.colorSpace,
        vk::Extent2D{renderRes.x, renderRes.y}, 1, vk::ImageUsageFlagBits::eColorAttachment);

    uint32_t queueFamilyIndices[] = {m_graphicsQueueFamilyIndex, m_presentQueueFamilyIndex};

    if (m_graphicsQueueFamilyIndex != m_presentQueueFamilyIndex)
    {
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

    try {
        m_swapchain = m_device->createSwapchainKHR(swapchainCreateInfo);
    }
    catch (vk::SystemError& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    m_swapchainImages.clear();
    auto swapchainImages = m_device->getSwapchainImagesKHR(m_swapchain);
    for (auto swapchainImage : swapchainImages)
        m_swapchainImages.push_back(SwapchainImage_t{swapchainImage});

    m_framebufferCount = m_swapchainImages.size();

    m_framebuffers = std::vector<vk::Framebuffer>(m_framebufferCount);

    return true;
}

void VkSubsystem::CreateImageViews()
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

void VkSubsystem::CreateDepthStencil()
{
    auto renderRes = m_parentEngine->GetRenderResolution();

    vk::ImageCreateInfo depthStencilImageCreateInfo{vk::ImageCreateFlags(),
                                                    vk::ImageType::e2D,
                                                    kDepthFormat,
                                                    vk::Extent3D{renderRes.x, renderRes.y, 1},
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

    m_deviceMemAllocs.push_back(newImageDevMem);

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

void VkSubsystem::CreateSharedCommandPools()
{
    vk::CommandPoolCreateInfo graphicsCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_graphicsQueueFamilyIndex);
    vk::CommandPoolCreateInfo computeCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_computeQueueFamilyIndex);

    m_graphicsCommandPool = m_device.get().createCommandPool(graphicsCmdPoolCreateInfo);
    m_computeCommandPool = m_device.get().createCommandPool(computeCmdPoolCreateInfo);

    vk::CommandPoolCreateInfo presentCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_presentQueueFamilyIndex);
    m_presentCommandPool = m_device.get().createCommandPool(presentCmdPoolCreateInfo);
}

uint32_t VkSubsystem::GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask)
{
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < m_phyDeviceMemProps.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((m_phyDeviceMemProps.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
}

} // namespace EnvGraph