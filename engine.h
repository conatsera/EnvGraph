#pragma once
#ifndef GRAPHICS_ENGINE_H
#define GRAPHICS_ENGINE_H

#include <atomic>
#include <glm/glm.hpp>
#include <mutex>
#include <thread>

#include <xcb/xcb.h>

#include <vulkan/vulkan.hpp>

#include "engine_defs.h"
#include "pipelines.h"
#include "pipelines/control.h"
#include "vk_utils.h"

namespace Engine {

typedef struct _SwapChainSupportDetails {
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
} SwapChainSupportDetails;

typedef struct SwapchainImage {
  vk::Image image;
  vk::ImageView view;

  SwapchainImage(vk::Image imageIn) : image(imageIn) {};
} SwapchainImage_t;

class Engine {
 public:
#ifdef _WIN32
  Engine(HWND window, std::vector<std::string> const& extraExtensions = {});
#else
  Engine(xcb_connection_t* xConnnection, xcb_window_t window,
         std::vector<std::string> const& extraExtensions = {});
#endif
  ~Engine();

  void DisplayFiducial() const;

  const SwapChainSupportDetails GetSwapchainSupportDetails() const {
    return m_swapchainSupportDetails;
  };

  const vk::DescriptorSetLayoutBinding GetMVPLayoutBinding() const {
    return m_mvpLayoutBinding;
  }

  void EnableComputePass() { m_computeEnabled = true; }
  void DisableComputePass() { m_computeEnabled = false; }

  void QueuePipelines(uint32_t newPipelines) {
    m_queuedPipelines.store(m_queuedPipelines.load() + newPipelines);
  };
  void DequeuePipelines(uint32_t removedPipelines) {
    m_queuedPipelines.store(m_queuedPipelines.load() - removedPipelines);
  };
  void NewPipeline(std::shared_ptr<Pipelines::EnginePipelineBase> newPipeline);

  void CreateComponentPipelines();

  void StartRender();

  void UpdateWindowExtents(uint32_t width, uint32_t height) {
    m_windowExtentChange = true;

    if (width != m_windowExtents.width || height != m_windowExtents.height)
    {
      m_engineEnabled = false;
      m_renderThread.join();
      
      m_windowExtents.width = width;
      m_windowExtents.height = height;

      CreateSwapChain();
      CreateImageViews();
      CreateDepthStencil();

      for (auto &pipeline : m_enginePipelines)
        pipeline->Resized(m_windowExtents);

      StartRender();
    }
  }

 private:
#ifndef TESTING
  void InitVulkan(std::vector<std::string> const& extraExtensions);
#endif

  // Begin Vulkan setup
#ifndef NDEBUG
  void SetupDebugMessenger();
#endif

  uint32_t GetMemoryType(uint32_t typeBits,
                         vk::MemoryPropertyFlags requirements_mask);

  void FindQueueFamilies();
  void CreateQueues();
#ifndef TESTING
  void SetupDevice();
#endif
  void ChooseSwapSurfaceFormat();
  void ChooseSwapExtent();

  void CreateSwapChain();
  void CreateImageViews();

  void CreateCommandPools();
  void QueueCommandBuffer(vk::Semaphore* imageAcqSemaphore);

  void CreateDepthStencil();

  void CreateControlGraphicsPipeline();
  void SetupRenderPass();

  static void RenderThread(vk::Device device, vk::RenderPass renderPass,
                           vk::SwapchainKHR& swapchain, uint32_t& currentFramebuffer,
                           vk::Queue& graphicsQueue,
                           vk::Queue& computeQueue,
                           vk::Queue& presentQueue,
                           std::vector<vk::Framebuffer>& framebuffers,
                           std::vector<SwapchainImage_t>& swapchainImages,
                           size_t& framebufferCount,
                           std::vector<std::shared_ptr<Pipelines::EnginePipelineBase>>& enginePipelines,
                           vk::ImageView& depthImageView,
                           vk::CommandPool& graphicsCommandPool, vk::Extent2D& windowExtents, bool& engineEnabled, Engine* context);

  void SetupFramebuffers();

 private:
  std::thread m_renderThread;
  bool m_engineEnabled = false;

  vk::DispatchLoaderDynamic m_dldy;
  vk::UniqueInstance m_instance;
  vk::InstanceCreateInfo m_instanceCreateInfo;

#ifndef NDEBUG
  vk::UniqueDebugUtilsMessengerEXT m_debugMessenger;
#endif

  vk::PhysicalDevice m_phyDev;
  vk::PhysicalDeviceFeatures m_phyDevFeatures;
  vk::PhysicalDeviceMemoryProperties m_phyDevMemProps;
  vk::PhysicalDeviceProperties m_phyDevProps;

  vk::UniqueDevice m_device;

  std::vector<vk::DeviceMemory> m_devMemBufferAllocs;
  std::vector<vk::DeviceMemory> m_devMemAllocs;

  std::vector<vk::ImageView> m_imageViews;
  size_t m_framebufferCount = 0;
  uint32_t m_currentFramebuffer = 0;
  std::vector<vk::Framebuffer> m_framebuffers;

  uint32_t m_graphicsQueueFamilyIndex;
  uint32_t m_graphicsQueuesMax;
  uint32_t m_graphicsQueuesAvailable;
  std::unique_ptr<Pipelines::ControlEnginePipeline> m_controlGraphicsPipeline;

  vk::Queue m_graphicsQueue;
  vk::CommandPool m_graphicsCommandPool;
  vk::UniqueCommandBuffer m_graphicsCommandBuffer;

  std::vector<std::shared_ptr<Pipelines::EnginePipelineBase>> m_enginePipelines;

  vk::Image m_depthImage;
  vk::ImageView m_depthImageView;

  vk::DescriptorSetLayoutBinding m_mvpLayoutBinding;

  vk::SurfaceKHR m_surface;

  SwapChainSupportDetails m_swapchainSupportDetails;
  vk::SwapchainKHR m_swapchain;
  std::vector<SwapchainImage_t> m_swapchainImages;
  vk::SurfaceFormatKHR m_swapchainFormat;

  vk::Extent2D m_windowExtents{kDefaultWidth, kDefaultHeight};
  bool m_windowExtentChange = false;

  vk::UniqueRenderPass m_renderPass;

  vk::Extent2D m_extent;

  uint32_t m_presentQueueFamilyIndex;
  vk::Queue m_presentQueue;
  vk::CommandPool m_presentCommandPool;
  vk::UniqueCommandBuffer m_presentCommandBuffer;
  // EnginePipeline m_present;

  bool m_computeEnabled = false;
  uint32_t m_computeQueueFamilyIndex;
  uint32_t m_computeQueuesMax;
  uint32_t m_computeQueuesAvailable;

  vk::Queue m_computeQueue;
  vk::CommandPool m_computeCommandPool;
  vk::UniqueCommandBuffer m_computeCommandBuffer;
  vk::UniqueFence m_computeCommandBufferFence;

  std::mutex m_pipelineCreationLock;
  std::atomic<uint32_t> m_queuedPipelines = 0;

  std::vector<BufferBarrierResource> m_bufferBarrierResources;
  std::vector<ImageBarrierResource> m_imageBarrierResources;

#ifdef _WIN32
  HWND m_window;
#else
  xcb_connection_t* m_xConnection;
  xcb_window_t m_window;
#endif

#ifdef TESTING
 public:
  vk::SurfaceKHR* GetSurfaceDebug() { return &m_surface; };
  vk::Instance* GetInstanceDebug() { return &m_instance.get(); };
  void InitVulkan(std::vector<std::string> const& extraExtensions);
  void SetupDevice();
#endif
};

}  // namespace Engine

#endif