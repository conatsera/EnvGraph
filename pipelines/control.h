#pragma once
#ifndef GRAPHICS_PIPELINES_CONTROL_H
#define GRAPHICS_PIPELINES_CONTROL_H

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../pipelines.h"

namespace Engine {
namespace Pipelines {

class ControlEnginePipeline : public EnginePipeline<1, 1, 1, 1, 0> {
 public:
  void Setup(const vk::UniqueDevice& device,
             vk::PhysicalDeviceMemoryProperties phyDevMemProps,
             const uint32_t graphicsQueueFamilyIndex,
             const uint32_t computeQueueFamilyIndex,
             const uint32_t graphicsQueueStartIndex,
             const uint32_t computeQueueStartIndex) override;

 private:
  void CreateViewModelProjection(const vk::UniqueDevice& device);

 private:
  glm::highp_mat4 m_mvp;
};

} // namespace Pipelines
} // namespace Engine

#endif