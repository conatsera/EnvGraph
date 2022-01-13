#ifndef ENVGRAPH_VK_SUBSYSTEM_H
#define ENVGRAPH_VK_SUBSYSTEM_H

#include <vector>
#include <thread>

#include <vulkan/vulkan.hpp>

#include "../subsystems.h"
#include "../pipelines.h"

namespace EnvGraph
{

constexpr const vk::Format kDepthFormat = vk::Format::eD16Unorm;
const std::vector<const char *> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

typedef struct _SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
} SwapChainSupportDetails;

typedef struct SwapchainImage
{
    vk::Image image;
    vk::ImageView view;

    SwapchainImage(vk::Image imageIn) : image(imageIn){};
} SwapchainImage_t;

using VkSubsystem = GraphicsSubsystem<GpuApiSetting::VULKAN>;

template<>
class GraphicsSubsystem<GpuApiSetting::VULKAN>
{
  public:
    GraphicsSubsystem();
    ~GraphicsSubsystem();

    bool Init(Engine *engine);
    void DeInit();

    void Start();
    void Stop();

    void NewPipeline(Pipelines::Pipeline *newPipeline);

    void EnableRenderPass();
    void DisableRenderPass();
    void EnableComputePass();
    void DisableComputePass();

    bool CheckRenderResolutionLimits(Extent renderRes) const;
    void UpdateRenderResolution() {
        if (m_enabled)
        {
            m_enabled = false;
            m_renderThread.join();

            CreateSwapChain();
            CreateImageViews();
            CreateDepthStencil();

            // for (auto &pipeline : m_pipelines)
            //    pipeline->Resized(m_windowExtents);

            Start();
        }
    };

  private:
    bool InitVulkan();
    bool CreateInstance();
#ifndef NDEBUG
    void SetupDebugUtils();
#endif

    bool SetupDevice();
    void SetupRenderPass();

    bool SetupQueues(std::vector<vk::DeviceQueueCreateInfo> &queueCreateInfos);
    void ChooseSwapChainSurfaceFormat();
    void ChooseSwapChainExtent();
    bool CreateSwapChain();
    void CreateImageViews();
    void CreateDepthStencil();
    //   Shared command pools to be used for standard and lightweight custom pipelines
    void CreateSharedCommandPools();

    uint32_t GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask);

  private:
    Engine *m_parentEngine = nullptr;
    bool m_initialized = false;
    bool m_enabled = false;
    std::thread m_renderThread;

    std::vector<std::string> m_extraInstanceLayers{};
    std::vector<std::string> m_extraInstanceExtensions{};
    vk::UniqueInstance m_instance;

#ifndef NDEBUG
    vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
#endif

    vk::PhysicalDevice m_phyDevice;
    vk::PhysicalDeviceFeatures m_phyDeviceFeatures;
    vk::PhysicalDeviceMemoryProperties m_phyDeviceMemProps;
    vk::PhysicalDeviceProperties m_phyDeviceProps;

    vk::SurfaceKHR m_surface;

    SwapChainSupportDetails m_swapchainSupportDetails;
    vk::SurfaceFormatKHR m_swapchainFormat;
    vk::SwapchainKHR m_swapchain;
    std::vector<SwapchainImage_t> m_swapchainImages;

    vk::UniqueDevice m_device;
    std::vector<vk::DeviceMemory> m_deviceMemBufferAllocs;
    std::vector<vk::DeviceMemory> m_deviceMemAllocs;

    std::vector<vk::ImageView> m_imageViews;
    size_t m_framebufferCount = 0;
    uint32_t m_currentFramebuffer = 0;
    std::vector<vk::Framebuffer> m_framebuffers;

    vk::Image m_depthImage;
    vk::ImageView m_depthImageView;

    uint32_t m_graphicsQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_graphicsQueuesMax = 0;
    uint32_t m_graphicsQueuesAvailable = 0;

    uint32_t m_presentQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    uint32_t m_computeQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_computeQueuesMax = 0;
    uint32_t m_computeQueuesAvailable = 0;

    vk::Queue m_graphicsQueue;
    vk::CommandPool m_graphicsCommandPool;
    vk::CommandBuffer m_graphicsCommandBuffer;

    vk::Queue m_presentQueue;
    vk::CommandPool m_presentCommandPool;
    vk::CommandBuffer m_presentCommandBuffer;

    vk::Queue m_computeQueue;
    vk::CommandPool m_computeCommandPool;
    vk::CommandBuffer m_computeCommandBuffer;

    vk::UniqueRenderPass m_renderPass;

    std::vector<Pipelines::Pipeline *> m_pipelines;
};

} // namespace EnvGraph

#endif