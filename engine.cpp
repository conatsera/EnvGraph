#include "engine.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>

namespace Engine {

using namespace Pipelines;

#if defined(_WIN32)
Engine::Engine(HWND windowHandle,
               std::vector<std::string> const& extraExtensions)
    : m_window(windowHandle) {
#else
Engine::Engine(xcb_connection_t* xConnnection, xcb_window_t windowHandle,
               std::vector<std::string> const& extraExtensions)
    : m_xConnection(xConnnection), m_window(windowHandle) {
#endif
  m_instance = createInstance("k4a-constellation-tracking", "No Engine",
                              m_instanceCreateInfo, {}, extraExtensions);

#ifndef TESTING
  InitVulkan(extraExtensions);
#endif

  SetupRenderPass();
  // SetupFramebuffers();
  // CreateControlGraphicsPipeline();
}

Engine::~Engine() {
  m_engineEnabled = false;

  for (auto& enginePipeline : m_enginePipelines) {
    enginePipeline->CleanupEnginePipeline(m_device);
  }

  m_renderThread.join();

  for (const auto& framebuffer : m_framebuffers)
    m_device->destroyFramebuffer(framebuffer);

  for (const auto& imageView : m_imageViews) {
    m_device->destroyImageView(imageView);
  }
  m_device->destroyImage(m_depthImage);
  m_device->destroyImageView(m_depthImageView);

  for (const auto& devMem : m_devMemBufferAllocs) {
    m_device->unmapMemory(devMem);
    m_device->freeMemory(devMem);
  }

  for (const auto& devMem : m_devMemAllocs) {
    m_device->freeMemory(devMem);
  }

  m_device->destroyCommandPool(m_graphicsCommandPool);
  m_device->destroyCommandPool(m_computeCommandPool);
  m_device->destroyCommandPool(m_presentCommandPool);

  m_device->destroySwapchainKHR(m_swapchain);

  m_instance->destroySurfaceKHR(m_surface);
}
#ifndef NDEBUG
void Engine::SetupDebugMessenger() {
  m_debugMessenger = createDebugUtilsMessenger(m_instance);
}
#endif

void Engine::CreateControlGraphicsPipeline() {
  m_controlGraphicsPipeline = std::make_unique<ControlEnginePipeline>();

  m_controlGraphicsPipeline->Setup(
      m_device, m_phyDevMemProps, m_graphicsQueueFamilyIndex,
      m_computeQueueFamilyIndex,
      m_graphicsQueuesMax - m_graphicsQueuesAvailable,
      m_computeQueuesMax - m_computeQueuesAvailable, m_renderPass);

  const QueueRequirements_t controlGraphicsQueueReqs =
      m_controlGraphicsPipeline->GetQueueRequirements();

  m_graphicsQueuesAvailable -= controlGraphicsQueueReqs.graphics;
  m_computeQueuesAvailable -= controlGraphicsQueueReqs.compute;
}

void Engine::NewPipeline(std::shared_ptr<EnginePipelineBase> newPipeline) {
  const std::lock_guard<std::mutex> lock(m_pipelineCreationLock);
  m_enginePipelines.push_back(newPipeline);
  m_queuedPipelines.store(m_queuedPipelines.load() - 1);

  newPipeline->Setup(m_device, m_phyDevMemProps, m_graphicsQueueFamilyIndex,
                     m_computeQueueFamilyIndex,
                     m_graphicsQueuesMax - m_graphicsQueuesAvailable,
                     m_computeQueuesMax - m_computeQueuesAvailable,
                     m_renderPass);

  const QueueRequirements_t controlGraphicsQueueReqs =
      newPipeline->GetQueueRequirements();

  m_graphicsQueuesAvailable -= controlGraphicsQueueReqs.graphics;
  m_computeQueuesAvailable -= controlGraphicsQueueReqs.compute;
}

void Engine::CreateComponentPipelines() {
  if (m_queuedPipelines.load() == 0) {
    const std::lock_guard<std::mutex> lock(m_pipelineCreationLock);
    std::cout << "Creating components" << std::endl;
    for (const auto& pipeline : m_enginePipelines) {
      if (!pipeline->IsSetup()) {
        std::cout << "Creating new pipeline" << std::endl;
        pipeline->Setup(m_device, m_phyDevMemProps, m_graphicsQueueFamilyIndex,
                        m_computeQueueFamilyIndex,
                        m_graphicsQueuesMax - m_graphicsQueuesAvailable,
                        m_computeQueuesMax - m_computeQueuesAvailable,
                        m_renderPass);

        const QueueRequirements_t queueReqs = pipeline->GetQueueRequirements();
        m_graphicsQueuesAvailable -= queueReqs.graphics;
        m_computeQueuesAvailable -= queueReqs.compute;
      }
    }
  }
}

void Engine::SetupRenderPass() {
  vk::SemaphoreCreateInfo imageAcquiredSemaphoreCI{vk::SemaphoreCreateFlags{0}};

  vk::UniqueSemaphore imageAcquiredSemaphore =
      m_device->createSemaphoreUnique(imageAcquiredSemaphoreCI);

  if (vk::Result::eSuccess ==
      m_device
          ->acquireNextImageKHR(m_swapchain, UINT64_MAX,
                                imageAcquiredSemaphore.get(), nullptr)
          .result) {
    std::vector<vk::AttachmentDescription> attachments{
        {vk::AttachmentDescriptionFlags{}, m_swapchainFormat.format,
         vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
         vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
         vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
         vk::ImageLayout::ePresentSrcKHR},
        {vk::AttachmentDescriptionFlags{}, kDepthFormat,
         vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
         vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
         vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
         vk::ImageLayout::eDepthStencilAttachmentOptimal}};

    vk::AttachmentReference colorRef{0,
                                     vk::ImageLayout::eColorAttachmentOptimal};
    vk::AttachmentReference depthRef{
        1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

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

    vk::SubpassDependency subpassDep{
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags{0},
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags{}};

    vk::RenderPassCreateInfo renderPassCI{vk::RenderPassCreateFlags{},
                                          attachments, subpassDesc, subpassDep};

    m_renderPass = m_device->createRenderPassUnique(renderPassCI);
  }
}
/*
void Engine::SetupFramebuffers() {
  vk::CommandBufferAllocateInfo graphicsCommandBufferAI{
      m_graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1};

  m_graphicsCommandBuffer.swap(
      m_device->allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

  m_graphicsCommandBuffer->begin(vk::CommandBufferBeginInfo{});

  std::vector<vk::ImageView> attachments(2);
  attachments[1] = m_depthImageView;

  vk::FramebufferCreateInfo framebufferCI{vk::FramebufferCreateFlags{},
                                          m_renderPass.get(),
                                          attachments,
                                          kDefaultWidth,
                                          kDefaultHeight,
                                          1};

  for (auto i = 0; i < m_framebufferCount; i++) {
    attachments[0] = m_swapchainImages[i].view;
    m_framebuffers[i] = m_device->createFramebufferUnique(framebufferCI);
  }
}
*/
void Engine::RenderThread(
    vk::Device device, vk::RenderPass renderPass, vk::SwapchainKHR& swapchain,
    uint32_t& currentFramebuffer, vk::Queue& graphicsQueue,
    vk::Queue& computeQueue, vk::Queue& presentQueue,
    std::vector<vk::Framebuffer>& framebuffers,
    std::vector<SwapchainImage_t>& swapchainImages, size_t& framebufferCount,
    std::vector<std::shared_ptr<Pipelines::EnginePipelineBase>>&
        enginePipelines,
    vk::ImageView& depthImageView, vk::CommandPool& graphicsCommandPool, vk::Extent2D& windowExtents,
    bool& engineEnabled, Engine* context) {
  auto clearValues = std::vector<vk::ClearValue>(2);

  clearValues[0].color.float32[0] = 0.2f;
  clearValues[0].color.float32[1] = 0.2f;
  clearValues[0].color.float32[2] = 0.2f;
  clearValues[0].color.float32[3] = 0.2f;
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;

  vk::SemaphoreCreateInfo imageAcquiredSemaphoreCI{vk::SemaphoreCreateFlags{0}};

  vk::UniqueSemaphore imageAcquiredSemaphore =
      device.createSemaphoreUnique(imageAcquiredSemaphoreCI);

  vk::RenderPassBeginInfo renderPassBI{
      renderPass,
      {},
      vk::Rect2D{{0, 0}, {windowExtents.width , windowExtents.height}},
      clearValues};

  vk::Viewport viewport{0,    0,   (float)windowExtents.width, (float)windowExtents.height,
                                  0.0f, 1.0f};

  vk::Rect2D scissor{vk::Offset2D{0, 0},
                               vk::Extent2D{windowExtents.width, windowExtents.height}};

  vk::PresentInfoKHR present;
  present.swapchainCount = 1;
  present.waitSemaphoreCount = 0;
  present.pWaitSemaphores = nullptr;
  present.pResults = nullptr;

  vk::Result presentResult;

  vk::SubmitInfo submitInfo;

  vk::UniqueFence graphicsCommandBufferFence =
      device.createFenceUnique(vk::FenceCreateInfo{});

  vk::UniqueFence computeCommandBufferFence =
      device.createFenceUnique(vk::FenceCreateInfo{});

  std::vector<vk::ImageView> attachments(2);
  attachments[1] = depthImageView;

  vk::FramebufferCreateInfo framebufferCI{vk::FramebufferCreateFlags{},
                                          renderPass,
                                          attachments,
                                          windowExtents.width,
                                          windowExtents.height,
                                          1};

  for (size_t i = 0; i < framebufferCount; i++) {
    attachments[0] = swapchainImages[i].view;
    framebuffers[i] = device.createFramebuffer(framebufferCI);
  }

  while (engineEnabled) {
    vk::CommandBufferAllocateInfo graphicsCommandBufferAI{
        graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1};

    vk::UniqueCommandBuffer graphicsCommandBuffer;
    graphicsCommandBuffer.swap(
        device.allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

    graphicsCommandBuffer->begin(vk::CommandBufferBeginInfo{});

    for (auto enginePipeline : enginePipelines) {
      if (enginePipeline->HasPreRenderStage())
        enginePipeline->PreRender(device, windowExtents);
    }

    if (vk::Result::eSuccess ==
        device.acquireNextImageKHR(swapchain, UINT64_MAX,
                                    imageAcquiredSemaphore.get(), nullptr,
                                    &currentFramebuffer)) {
      renderPassBI.framebuffer = framebuffers[currentFramebuffer];

      graphicsCommandBuffer->beginRenderPass(renderPassBI,
                                             vk::SubpassContents::eInline);

      // m_controlGraphicsPipeline->Render(m_device, m_graphicsCommandBuffer);

      graphicsCommandBuffer->setViewport(0, viewport);

      graphicsCommandBuffer->setScissor(0, scissor);

      for (auto enginePipeline : enginePipelines) {
        const QueueRequirements_t queueReqs =
            enginePipeline->GetQueueRequirements();
        if (queueReqs.graphics > 0)
          enginePipeline->Render(device, graphicsCommandBuffer.get(), windowExtents);
        //if (queueReqs.compute > 0)
        //  enginePipeline->Compute(device, computeCommandBuffer);
      }

      graphicsCommandBuffer->endRenderPass();

      graphicsCommandBuffer->end();

      constexpr vk::PipelineStageFlags pipelineStageFlags[1] = {
          vk::PipelineStageFlagBits::eColorAttachmentOutput};

      submitInfo = {1,      &(imageAcquiredSemaphore.get()), pipelineStageFlags,
                    1,      &(graphicsCommandBuffer.get()),  0,
                    nullptr};

      graphicsQueue.submit(submitInfo, graphicsCommandBufferFence.get());
      
      vk::Result res;
      do {
        res = device.waitForFences(graphicsCommandBufferFence.get(), VK_TRUE,
                                    kFenceTimeout);
      } while (res == vk::Result::eTimeout);
      device.resetFences(graphicsCommandBufferFence.get());

      present.pSwapchains = &swapchain;
      present.pImageIndices = &currentFramebuffer;

      try{
        presentResult = presentQueue.presentKHR(present);
      } catch (const vk::OutOfDateKHRError& e)
      {
        engineEnabled = false;
        break;
      }

      if (presentResult != vk::Result::eSuccess) {
        std::cout << "Error: " << vk::to_string(presentResult) << std::endl;
      }
    }
  }
}

void Engine::StartRender() {
  m_engineEnabled = true;
  m_renderThread =
      std::thread(RenderThread,
                  m_device.get(), m_renderPass.get(),
                  std::ref(m_swapchain),  std::ref(m_currentFramebuffer),
                  std::ref(m_graphicsQueue), std::ref(m_computeQueue),
                  std::ref(m_presentQueue), std::ref(m_framebuffers),
                  std::ref(m_swapchainImages), std::ref(m_framebufferCount),
                  std::ref(m_enginePipelines), std::ref(m_depthImageView),
                  std::ref(m_graphicsCommandPool), std::ref(m_windowExtents), std::ref(m_engineEnabled), this);
}

}  // namespace Engine