#include "engine.h"

#include <iostream>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

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
}

Engine::~Engine() {
  for (const auto& imageView : m_imageViews) {
    m_device->destroyImageView(imageView);
  }
  m_device->destroyImageView(m_depthImageView);

  for (const auto& enginePipeline : m_enginePipelines)
    enginePipeline->CleanupEnginePipeline(m_device);

  for (const auto& devMem : m_devMemBufferAllocs) {
    m_device->unmapMemory(devMem);
    m_device->freeMemory(devMem);
  }

  for (const auto& devMem : m_devMemAllocs) {
    m_device->freeMemory(devMem);
  }
}
#ifndef NDEBUG
void Engine::SetupDebugMessenger() {
  m_debugMessenger = createDebugUtilsMessenger(m_instance);
}
#endif

void Engine::SetupRenderPass() {
  // vk::AttachmentDescription shaderAttachments[3];
}

void Engine::CreateControlGraphicsPipeline() {
m_controlGraphicsPipeline.Setup(
      m_device, m_phyDevMemProps,
      m_graphicsQueueFamilyIndex,
      m_computeQueueFamilyIndex,
      m_graphicsQueuesMax - m_graphicsQueuesAvailable,
      m_computeQueuesMax - m_computeQueuesAvailable);

  const QueueRequirements_t controlGraphicsQueueReqs =
      m_controlGraphicsPipeline.GetQueueRequirements();
  m_graphicsQueuesAvailable -= controlGraphicsQueueReqs.graphics;
  m_computeQueuesAvailable -= controlGraphicsQueueReqs.compute;
}

void Engine::NewPipeline(std::shared_ptr<EnginePipelineBase> newPipeline) {
  const std::lock_guard<std::mutex> lock(m_pipelineCreationLock);
  m_enginePipelines.push_back(newPipeline);
  m_queuedPipelines.store(m_queuedPipelines.load() - 1);
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
                      m_computeQueuesMax - m_computeQueuesAvailable);

      const QueueRequirements_t queueReqs = pipeline->GetQueueRequirements();
      m_graphicsQueuesAvailable -= queueReqs.graphics;
      m_computeQueuesAvailable -= queueReqs.compute;
    }
  }
  }
}

} // namespace Engine