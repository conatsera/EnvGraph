#include "kinect_mesh.h"

#include <cstdint>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include "../vk_utils.h"

namespace Engine
{
namespace Pipelines
{

void KinectMesh::Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass)
{
    m_hasPreRenderStage = true;
    EnginePipeline::Setup(device, phyDevMemProps, graphicsQueueFamilyIndex, computeQueueFamilyIndex,
                          graphicsQueueStartIndex, computeQueueStartIndex, renderPass);
    CreateViewModelProjection(device);

    m_pushConstantRanges[0] = {vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::vec2) * 3};

    SetupShaders(device);
    SetupPipeline(device);

    CreateShaders(device);

    vertexInputBindingDesc.setStride(sizeof(glm::vec4) + sizeof(glm::ivec4));

    // 3D Scan coordinates (xyz) and filter opacity (w)
    vertexInputAttrDesc = {
        {0, 0, vk::Format::eR32G32B32A32Sfloat, 0},
        {1, 0, vk::Format::eR32G32B32A32Uint, 16}
    };

    vertexInfoStateCI.setVertexAttributeDescriptions(vertexInputAttrDesc);

    inputAssemblyStateCI.setTopology(vk::PrimitiveTopology::eTriangleList);

    vk::GraphicsPipelineCreateInfo graphicsPipelineCI{vk::PipelineCreateFlags{},
                                                      m_shaderStages,
                                                      &vertexInfoStateCI,
                                                      &inputAssemblyStateCI,
                                                      nullptr,
                                                      &viewportStateCI,
                                                      &rasterStateCI,
                                                      &multisampleStateCI,
                                                      &depthStencilStateCI,
                                                      &colorBlendStateCI,
                                                      &dynamicStateCI,
                                                      m_pipelineLayout.get(),
                                                      renderPass.get(),
                                                      0,
                                                      nullptr,
                                                      0};

    m_pipeline = device->createGraphicsPipelineUnique(m_pipelineCache.get(), graphicsPipelineCI).value;
}

void KinectMesh::CleanupEnginePipeline(const vk::UniqueDevice &device)
{
    EnginePipeline::CleanupEnginePipeline(device);
}

void KinectMesh::CreateViewModelProjection(const vk::UniqueDevice &device)
{
    auto projection = glm::perspective(glm::radians(90.0F), 1.0F, 0.1F, 10.0F);
    auto view = glm::lookAt(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
    auto model = glm::mat4(1.0F);
    // clang-format off
  /*auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f,-1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.0f, 0.0f, 0.5f, 1.0f);*/
    // clang-format on

    m_viewParams.mvp = projection * view * model;
    m_viewParams.window = glm::uvec2{kDefaultWidth, kDefaultHeight};
    m_viewParams.viewOffset = glm::vec2{0, 0};

    vk::BufferCreateInfo viewParamsBufferCreateInfo{vk::BufferCreateFlags{}, sizeof(m_viewParams),
                                                    vk::BufferUsageFlagBits::eUniformBuffer};

    vk::DescriptorSetLayoutBinding viewParamsLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1,
                                                           vk::ShaderStageFlagBits::eVertex};

    vk::DescriptorSetLayoutBinding depthImageLayoutBinding{1, vk::DescriptorType::eStorageImage, 1,
                                                             vk::ShaderStageFlagBits::eVertex};

    vk::DescriptorSetLayoutBinding colorImageLayoutBinding{2, vk::DescriptorType::eStorageImage, 1,
                                                             vk::ShaderStageFlagBits::eVertex};

    std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings{
        viewParamsLayoutBinding, depthImageLayoutBinding, colorImageLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo viewParamsLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(), layoutBindings};

    m_meshDescSetID = AddDescriptorLayoutSets(device, viewParamsLayoutCreateInfo);

    EngineBuffer_t viewParamsEngBuf =
        CreateNewBuffer(device, KinectMeshBufferIDs::kViewParamsBuffer, 0, m_meshDescSetID, viewParamsBufferCreateInfo,
                        reinterpret_cast<void **>(&m_viewParamsBuffer),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (m_viewParamsBuffer != nullptr)
        memcpy(m_viewParamsBuffer, &m_viewParams, sizeof(m_viewParams));

    device->bindBufferMemory(viewParamsEngBuf.buffer, viewParamsEngBuf.devMem, 0);
}

void KinectMesh::CreateShaders(const vk::UniqueDevice &device)
{
    vk::PipelineShaderStageCreateInfo kinectMeshVertPipelineShaderStageCI{
        vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, nullptr, "main"};
#include "kinect_mesh.vert.h"
    vk::ShaderModuleCreateInfo kinectMeshVertShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(kinect_mesh_vert),
                                                            kinect_mesh_vert};

    m_kinectMeshVertShaderModule = device->createShaderModuleUnique(kinectMeshVertShaderModuleCI);

    kinectMeshVertPipelineShaderStageCI.setModule(m_kinectMeshVertShaderModule.get());

    m_shaderStages.push_back(kinectMeshVertPipelineShaderStageCI);

    vk::PipelineShaderStageCreateInfo kinectMeshFragPipelineShaderStageCI{
        vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, nullptr, "main"};
#include "control.frag.h"
    vk::ShaderModuleCreateInfo kinectMeshFragShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(control_frag),
                                                            control_frag};

    m_kinectMeshFragShaderModule = device->createShaderModuleUnique(kinectMeshFragShaderModuleCI);

    kinectMeshFragPipelineShaderStageCI.setModule(m_kinectMeshFragShaderModule.get());

    m_shaderStages.push_back(kinectMeshFragPipelineShaderStageCI);
}

void KinectMesh::SetupVertexShader(const vk::UniqueDevice &device)
{
    vk::BufferCreateInfo vertBufferCI{vk::BufferCreateFlags{},
                                      sizeof(glm::vec4) * m_scanExtents.width * m_scanExtents.height * 3,
                                      vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive};

    m_scanEngBuf =
        CreateNewBuffer(device, kScanBuffer, vertBufferCI, reinterpret_cast<void **>(m_scanBuffer.data()),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    device->bindBufferMemory(m_scanEngBuf.buffer, m_scanEngBuf.devMem, 0);

    vk::ImageCreateInfo depthImageCI{vk::ImageCreateFlags(),
                                     vk::ImageType::e2D,
                                     vk::Format::eR16Uint,
                                     vk::Extent3D{1024, 1024, 1},
                                     1,
                                     1,
                                     vk::SampleCountFlagBits::e1,
                                     vk::ImageTiling::eOptimal,
                                     vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                                     vk::SharingMode::eExclusive,
                                     {},
                                     vk::ImageLayout::eUndefined};

    vk::ImageSubresourceRange depthImageSR{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    m_depthImageView = CreateNewImage(device, kDepthImage, 1, m_meshDescSetID, depthImageCI, depthImageSR,
                                      vk::ImageLayout::eGeneral);

    constexpr const size_t depthBufferSize = 1024 * 1024 * sizeof(uint16_t);

    vk::BufferCreateInfo depthBufferCI{vk::BufferCreateFlags(), depthBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                                       vk::SharingMode::eExclusive};

    m_depthEngBuffer =
        CreateNewBuffer(device, kDepthBuffer, depthBufferCI, reinterpret_cast<void **>(&m_depthBuffer),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    vk::ImageCreateInfo colorImageCI{vk::ImageCreateFlags(),
                                     vk::ImageType::e2D,
                                     vk::Format::eB8G8R8A8Unorm,
                                     vk::Extent3D{4096, 3072, 1},
                                     1,
                                     1,
                                     vk::SampleCountFlagBits::e1,
                                     vk::ImageTiling::eOptimal,
                                     vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
                                     vk::SharingMode::eExclusive,
                                     {},
                                     vk::ImageLayout::eUndefined};

    vk::ImageSubresourceRange colorImageSR{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    m_colorImageView = CreateNewImage(device, kColorImage, 2, m_meshDescSetID, colorImageCI, colorImageSR,
                                      vk::ImageLayout::eGeneral);

    constexpr const size_t colorBufferSize = 4096 * 3072 * sizeof(uint8_t) * 4;

    vk::BufferCreateInfo colorBufferCI{vk::BufferCreateFlags(), colorBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                                       vk::SharingMode::eExclusive};

    m_colorEngBuffer =
        CreateNewBuffer(device, kColorBuffer, colorBufferCI, reinterpret_cast<void **>(&m_colorBuffer),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);    
}

void KinectMesh::SetupBufferCallback(BufferGenFunc bufferGenCb) {
    m_bufferGenCb = bufferGenCb;
}

void KinectMesh::PreRender(vk::Device const &device, vk::Extent2D windowExtent)
{
    m_bufferGenCb(m_depthBuffer, m_colorBuffer);
}

void KinectMesh::Render(vk::Device const &device, const vk::CommandBuffer &cmdBuffer, vk::Extent2D windowExtent)
{
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets,
                                 nullptr);

    memcpy(m_viewParamsBuffer, &m_viewParams, sizeof(m_viewParams));

    const vk::DeviceSize offsets[1]{// TODO: offset into scan to avoid measuring null pixels
                                    0};
    cmdBuffer.bindVertexBuffers(0, 1, &m_scanEngBuf.buffer, offsets);

    cmdBuffer.draw(m_scanExtents.width * m_scanExtents.height * 3, 1, 0, 0);
}

} // namespace Pipelines
} // namespace Engine
