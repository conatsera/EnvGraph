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
    vertexInputAttrDesc = {{0, 0, vk::Format::eR32G32B32A32Sfloat, 0}, {1, 0, vk::Format::eR32G32B32A32Uint, 16}};

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

    std::array<vk::DescriptorSetLayoutBinding, 3> layoutBindings{viewParamsLayoutBinding, depthImageLayoutBinding,
                                                                 colorImageLayoutBinding};

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

constexpr const size_t kDepthBufferSize = 1024 * 1024 * sizeof(uint16_t);
constexpr const size_t kColorBufferSize = 4096 * 3072 * sizeof(uint8_t) * 4;

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

    m_depthImageView =
        CreateNewImage(device, kDepthImage, 1, m_meshDescSetID, depthImageCI, depthImageSR, vk::ImageLayout::eGeneral);

    vk::BufferCreateInfo depthBufferCI{vk::BufferCreateFlags(), kDepthBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
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

    m_colorImageView =
        CreateNewImage(device, kColorImage, 2, m_meshDescSetID, colorImageCI, colorImageSR, vk::ImageLayout::eGeneral);

    vk::BufferCreateInfo colorBufferCI{vk::BufferCreateFlags(), kColorBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                                       vk::SharingMode::eExclusive};

    m_colorEngBuffer =
        CreateNewBuffer(device, kColorBuffer, colorBufferCI, reinterpret_cast<void **>(&m_colorBuffer),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    device->bindBufferMemory(m_depthEngBuffer.buffer, m_depthEngBuffer.devMem, 0);
    device->bindBufferMemory(m_colorEngBuffer.buffer, m_colorEngBuffer.devMem, 0);
}

void KinectMesh::SetupBufferCallback(BufferGenFunc bufferGenCb)
{
    m_bufferGenCb = bufferGenCb;
}

void KinectMesh::PreRender(vk::Device const &device, vk::Extent2D windowExtent)
{
    m_bufferGenCb(m_depthBuffer, m_colorBuffer);

    constexpr const vk::ImageSubresourceRange imageSR{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    std::array<vk::MappedMemoryRange, 2> imageBufferMemRanges{
        vk::MappedMemoryRange{m_depthEngBuffer.devMem, 0, kDepthBufferSize},
        vk::MappedMemoryRange{m_colorEngBuffer.devMem, 0, kColorBufferSize},
    };

    device.flushMappedMemoryRanges(imageBufferMemRanges);

    vk::ImageMemoryBarrier depthCopyBarrier{vk::AccessFlags{},           vk::AccessFlagBits::eTransferWrite,
                                            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                            VK_QUEUE_FAMILY_IGNORED,     VK_QUEUE_FAMILY_IGNORED,
                                            m_images[kDepthImage],       imageSR};

    vk::ImageMemoryBarrier colorCopyBarrier{vk::AccessFlags{},           vk::AccessFlagBits::eTransferWrite,
                                            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                            VK_QUEUE_FAMILY_IGNORED,     VK_QUEUE_FAMILY_IGNORED,
                                            m_images[kColorImage],       imageSR};

    std::array<vk::ImageMemoryBarrier, 2> copyBarriers{depthCopyBarrier, colorCopyBarrier};

    vk::CommandBufferAllocateInfo graphicsCommandBufferAI{m_graphicsCommandPool.get(), vk::CommandBufferLevel::ePrimary,
                                                          1};

    vk::UniqueCommandBuffer cmdBuffer;
    cmdBuffer.swap(device.allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

    vk::CommandBufferBeginInfo cmdBufBI{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    cmdBuffer->begin(cmdBufBI);

    cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                               copyBarriers);

    vk::BufferImageCopy copyRegion = {};
    copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent.depth = 1;
    copyRegion.imageExtent.width = 1024;
    copyRegion.imageExtent.height = 1024;

    cmdBuffer->copyBufferToImage(m_depthEngBuffer.buffer, m_images[kDepthImage], vk::ImageLayout::eTransferDstOptimal,
                                 copyRegion);

    copyRegion.imageExtent.width = 4096;
    copyRegion.imageExtent.height = 3072;

    cmdBuffer->copyBufferToImage(m_colorEngBuffer.buffer, m_images[kColorImage], vk::ImageLayout::eTransferDstOptimal,
                                 copyRegion);

    std::array<vk::ImageMemoryBarrier, 2> useBarriers{
        vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                               vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED,
                               VK_QUEUE_FAMILY_IGNORED, m_images[kDepthImage], imageSR},
        vk::ImageMemoryBarrier{vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                               vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED,
                               VK_QUEUE_FAMILY_IGNORED, m_images[kColorImage], imageSR},
    };

    cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexShader, {}, {},
                               {}, useBarriers);

    vk::SubmitInfo bufferImageCopiesSI{{}, {}, cmdBuffer.get(), {}};

    cmdBuffer->end();

    m_graphicsQueues[0].submit(bufferImageCopiesSI);

    device.waitIdle();
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

    const std::array<glm::vec2, 3> tempFilterParams = {glm::vec2{1, -1}, glm::vec2{1, -1}, glm::vec2{1, -1}};
    cmdBuffer.pushConstants(m_pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::vec2) * 3,
                            tempFilterParams.data());

    cmdBuffer.draw(m_scanExtents.width * m_scanExtents.height * 3, 1, 0, 0);
}

} // namespace Pipelines
} // namespace Engine
