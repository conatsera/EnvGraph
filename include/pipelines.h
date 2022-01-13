#ifndef ENVGRAPH_PIPELINE_H
#define ENVGRAPH_PIPELINE_H

#include <array>

#include "common.h"
#include "graphics/objects.h"

#if GPU_API_SETTING == 0
#include <vulkan/vulkan.hpp>
#endif

namespace EnvGraph
{
namespace Pipelines
{

typedef struct QueueRequirements
{
    uint32_t graphics;
    uint32_t compute;
} QueueRequirements_t;

// A DescriptorGroup represents an underlying DescriptorSet (VK) or DescriptorTable (DX12)
typedef uint32_t DescriptorGroupID;

template <GpuApiSetting setting> struct SetupInfo;
template <GpuApiSetting setting> struct PreRenderInfo;
template <GpuApiSetting setting> struct RenderInfo;
template <GpuApiSetting setting> struct ComputeInfo;

#if GPU_API_SETTING == 0

template <> struct Pipelines::SetupInfo<GpuApiSetting::VULKAN>
{
    vk::UniqueDevice &device;
    vk::PhysicalDeviceMemoryProperties phyDevMemProps;
    const uint32_t graphicsQueueFamilyIndex;
    const uint32_t computeQueueFamilyIndex;
    const uint32_t graphicsQueueStartIndex;
    const uint32_t computeQueueStartIndex;
    const vk::UniqueRenderPass &renderPass;
};

template <> struct Pipelines::PreRenderInfo<GpuApiSetting::VULKAN>
{
    vk::UniqueDevice &device;
    Extent renderRes;
};

template <> struct Pipelines::RenderInfo<GpuApiSetting::VULKAN>
{
    vk::UniqueDevice &device;
    vk::UniqueCommandBuffer &commandBuffer;
    Extent renderRes;
};

template <> struct Pipelines::ComputeInfo<GpuApiSetting::VULKAN>
{
    vk::UniqueDevice &device;
};

#endif

// The pipeline is restricted to non-API specific commands and contains various stages defined by the 
// derived class
class Pipeline
{
  public:
    virtual void Setup(SetupInfo<kGpuApiSetting> setupInfo) = 0;

    virtual void Cleanup() = 0;

    virtual const QueueRequirements_t GetQueueRequirements() const = 0;

    bool IsSetup() const
    {
        return m_setupComplete;
    }
    bool HasPreRenderStage() const
    {
        return m_hasPreRenderStage;
    }
    bool HasComputeStage() const
    {
        return m_hasComputeStage;
    }

    virtual void PreRender(PreRenderInfo<kGpuApiSetting> runInfo){};
    virtual void Render(RenderInfo<kGpuApiSetting> runInfo){};
    virtual void Compute(ComputeInfo<kGpuApiSetting> runInfo){};

  protected:
    //const DescriptorGroupID AddDescriptorGroup()
    //{
    //}
   
  protected:
    bool m_setupComplete = false;
    bool m_hasPreRenderStage = false;
    bool m_hasComputeStage = false;
};

} // namespace Pipelines

} // namespace EnvGraph

#endif