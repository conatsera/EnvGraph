#ifndef ENVGRAPH_PIPELINES_KINECT_MESH_H
#define ENVGRAPH_PIPELINES_KINECT_MESH_H

#include <vulkan/vulkan.hpp>
#include <glm/mat4x4.hpp>

#include "../pipelines.h"

namespace Engine {
namespace Pipelines {

enum KinectMeshBufferIDs : BufferID_t
{
    kViewParamsBuffer = 0,
    kDepthBuffer = 1,
    kColorBuffer = 2,
    kScanBuffer = 3
};

enum KinectMeshImageIDs : BufferID_t
{
    kDepthImage = 0,
    kColorImage = 1
};

typedef std::function<bool(uint16_t*, glm::vec<4, uint8_t>*)> BufferGenFunc;

constexpr const vk::Extent2D kDefaultScanExtents{512,512};

class KinectMesh : public EnginePipeline<1, 2, 2, 0, 1, 0, 1> {
public:
    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass) override;

    virtual void CleanupEnginePipeline(const vk::UniqueDevice& device) override;

    void SetupShaders(const vk::UniqueDevice &device)
    {
        SetupVertexShader(device);
        // SetupFragmentBuffer(device);
    };

    virtual void PreRender(const vk::Device &device, vk::Extent2D windowExtents) override;

    virtual void Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer,
                        vk::Extent2D windowExtents) override;

    const vk::Extent2D GetScanExtents() const { return m_scanExtents; }

    const bool IsValid() const {
        return (m_scanExtents.width != 0 && m_scanExtents.height != 0) &&
               (m_bufferGenCb);
    }

    void SetupBufferCallback(BufferGenFunc bufferCb);

private:
    // TODO: Move this to the GraphicsEnginePipelineBase
    void CreateViewModelProjection(const vk::UniqueDevice &device);
    void CreateShaders(const vk::UniqueDevice &device);

    void SetupVertexShader(const vk::UniqueDevice &device);
    void SetupFragmentBuffer(const vk::UniqueDevice &device);

private:
    struct
    {
        glm::highp_mat4 mvp;
        glm::uvec2      window;
        glm::vec2       viewOffset;
    } m_viewParams;
    uint8_t *m_viewParamsBuffer = nullptr;

    vk::UniqueShaderModule m_kinectMeshVertShaderModule;
    vk::UniqueShaderModule m_kinectMeshFragShaderModule;

    DescriptorSetID m_meshDescSetID;

    vk::ImageView  m_depthImageView;
    EngineBuffer_t m_depthEngBuffer;
    uint16_t*      m_depthBuffer = nullptr;

    vk::ImageView         m_colorImageView;
    EngineBuffer_t        m_colorEngBuffer;
    glm::vec<4, uint8_t>* m_colorBuffer;

    BufferGenFunc         m_bufferGenCb;

    const vk::Extent2D m_scanExtents{kDefaultScanExtents};

    std::vector<glm::vec4> m_scanBuffer = std::vector<glm::vec4>(kDefaultScanExtents.width*kDefaultScanExtents.height);
    EngineBuffer_t m_scanEngBuf;
};

} // namespace Pipelines
} // namespace Engine

//#include "kinect_mesh.cpp"

#endif