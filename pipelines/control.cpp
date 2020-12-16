#include "control.h"

#include "../engine_defs.h"

namespace Engine {
namespace Pipelines {

void ControlEnginePipeline::Setup(
    const vk::UniqueDevice& device,
    vk::PhysicalDeviceMemoryProperties phyDevMemProps,
    const uint32_t graphicsQueueFamilyIndex,
    const uint32_t computeQueueFamilyIndex,
    const uint32_t graphicsQueueStartIndex,
    const uint32_t computeQueueStartIndex) {

}

void ControlEnginePipeline::CreateViewModelProjection(
    const vk::UniqueDevice& device) {
  auto projection = glm::perspective(glm::radians(45.0F), 1.0F, 0.1F, 100.0F);
  auto view = glm::lookAt(glm::vec3(-5, 3, -10), glm::vec3(0, 0, 0),
                          glm::vec3(0, -1, 0));
  auto model = glm::mat4(1.0F);
  // clang-format off
  auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f,-1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.0f, 0.0f, 0.5f, 1.0f);
  // clang-format on

  m_mvp = clip * projection * view * model;

  vk::BufferCreateInfo mvpBufferCreateInfo{
      vk::BufferCreateFlags{}, sizeof(m_mvp),
      vk::BufferUsageFlagBits::eUniformBuffer};

  vk::DescriptorSetLayoutBinding mvpLayoutBinding{
      0, vk::DescriptorType::eUniformBuffer, 1,
      vk::ShaderStageFlagBits::eVertex};

  vk::DescriptorSetLayoutCreateInfo mvpLayoutCreateInfo{
      vk::DescriptorSetLayoutCreateFlags(), 1, &mvpLayoutBinding};

  DescriptorSetID mvpSetID =
      AddDescriptorLayoutSets(device, mvpLayoutCreateInfo);

  uint8_t* mvpBuffer = nullptr;
  CreateNewBuffer(device, mvpSetID,
                                            mvpBufferCreateInfo, mvpBuffer);

  if (mvpBuffer != nullptr) memcpy(mvpBuffer, &m_mvp, sizeof(m_mvp));
}
} // namespace Pipelines
} // namespace Engine