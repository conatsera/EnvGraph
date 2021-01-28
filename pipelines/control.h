#pragma once
#ifndef GRAPHICS_PIPELINES_CONTROL_H
#define GRAPHICS_PIPELINES_CONTROL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include "../pipelines.h"

namespace Engine
{
namespace Pipelines
{

enum ControlEngineBufferIDs : BufferID_t
{
    kMvpBufferID = 0,
    kCubeBufferID = 1
};

enum ControlEngineSamplerIDs : SamplerID_t
{
    kTextureSamplerID = 0
};

class ControlEnginePipeline : public EnginePipeline<1, 1, 1, 1, 1, 0>
{
  public:
    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass) override;

    virtual void CleanupEnginePipeline(const vk::UniqueDevice &device) override;

    void SetupShaders(const vk::UniqueDevice &device)
    {
        SetupVertexBuffer(device);
        // SetupFragmentBuffer(device);
    };

    virtual void Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer,
                        vk::Extent2D windowExtents) override;

  private:
    void CreateViewModelProjection(const vk::UniqueDevice &device);
    void CreateShaders(const vk::UniqueDevice &device);

    void SetupVertexBuffer(const vk::UniqueDevice &device);
    void SetupFragmentBuffer(const vk::UniqueDevice &device);

  private:
    glm::highp_mat4 m_mvp;

    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

    vk::UniqueShaderModule m_controlVertShaderModule;
    vk::UniqueShaderModule m_controlFragShaderModule;

    EngineBuffer_t m_cubeEngBuf;
    std::byte *m_vertData;
    vk::Sampler m_textureSampler;
};

} // namespace Pipelines
} // namespace Engine

#endif