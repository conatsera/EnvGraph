#include "control.h"

#include <iostream>

#include "../cube_data.h"
#include "../engine_defs.h"

namespace Engine
{
namespace Pipelines
{

void ControlEnginePipeline::Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                                  const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                                  const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                                  const vk::UniqueRenderPass &renderPass)
{
    EnginePipeline::Setup(device, phyDevMemProps, graphicsQueueFamilyIndex, computeQueueFamilyIndex,
                          graphicsQueueStartIndex, computeQueueStartIndex, renderPass);
    CreateViewModelProjection(device);

    SetupShaders(device);
    SetupPipeline(device);

    CreateShaders(device);

    vertexInputBindingDesc.stride = sizeof(g_vb_solid_face_colors_Data[0]);
    vertexInfoStateCI.setVertexBindingDescriptions(vertexInputBindingDesc);

    vertexInputAttrDesc = {{0, 0, vk::Format::eR32G32B32A32Sfloat, 0},
                                                                         {1, 0, vk::Format::eR32G32B32A32Sfloat, 16}};

    vertexInfoStateCI.setVertexAttributeDescriptions(vertexInputAttrDesc);    

    colorBlendAttachmentState.colorWriteMask = (vk::ColorComponentFlags)(0xf);

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

void ControlEnginePipeline::CleanupEnginePipeline(const vk::UniqueDevice &device)
{
    device->destroyShaderModule(m_controlVertShaderModule.get());
    device->destroyShaderModule(m_controlFragShaderModule.get());

    EnginePipeline::CleanupEnginePipeline(device);
}

void ControlEnginePipeline::CreateViewModelProjection(const vk::UniqueDevice &device)
{
    auto projection = glm::perspective(glm::radians(45.0F), 1.0F, 0.1F, 100.0F);
    auto view = glm::lookAt(glm::vec3(-5, 3, -10), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
    auto model = glm::mat4(1.0F);
    // clang-format off
    auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f,-1.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.5f, 0.0f,
                          0.0f, 0.0f, 0.5f, 1.0f);
    // clang-format on

    m_mvp = clip * projection * view * model;

    vk::BufferCreateInfo mvpBufferCreateInfo{vk::BufferCreateFlags{}, sizeof(m_mvp),
                                             vk::BufferUsageFlagBits::eUniformBuffer};

    vk::DescriptorSetLayoutBinding mvpLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1,
                                                    vk::ShaderStageFlagBits::eVertex};

    std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{mvpLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo mvpLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(), layoutBindings};

    DescriptorSetID mvpSetID = AddDescriptorLayoutSets(device, mvpLayoutCreateInfo);

    uint8_t *mvpBuffer = nullptr;
    EngineBuffer_t mvpEngBuf = CreateNewBuffer(device, ControlEngineBufferIDs::kMvpBufferID, 0, mvpSetID,
                                               mvpBufferCreateInfo, reinterpret_cast<void **>(&mvpBuffer),
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (mvpBuffer != nullptr)
        memcpy(mvpBuffer, &m_mvp, sizeof(m_mvp));

    device->bindBufferMemory(mvpEngBuf.buffer, mvpEngBuf.devMem, 0);
}

void ControlEnginePipeline::CreateShaders(const vk::UniqueDevice &device)
{
    std::vector<uint32_t> controlVertSpriv;

    vk::PipelineShaderStageCreateInfo controlVertPipelineShaderStageCI{
        vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, nullptr, "main"};
#include "control.vert.h"
    vk::ShaderModuleCreateInfo controlVertShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(control_vert),
                                                         control_vert};

    m_controlVertShaderModule = device->createShaderModuleUnique(controlVertShaderModuleCI);

    controlVertPipelineShaderStageCI.setModule(m_controlVertShaderModule.get());

    m_shaderStages.push_back(controlVertPipelineShaderStageCI);

    std::vector<uint32_t> controlFragSpriv;

    vk::PipelineShaderStageCreateInfo controlFragPipelineShaderStageCI{
        vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, nullptr, "main"};
#include "control.frag.h"
    vk::ShaderModuleCreateInfo controlFragShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(control_frag),
                                                         control_frag};

    m_controlFragShaderModule = device->createShaderModuleUnique(controlFragShaderModuleCI);

    controlFragPipelineShaderStageCI.setModule(m_controlFragShaderModule.get());

    m_shaderStages.push_back(controlFragPipelineShaderStageCI);
}

void ControlEnginePipeline::SetupVertexBuffer(const vk::UniqueDevice &device)
{
    vk::BufferCreateInfo vertBufferCI{vk::BufferCreateFlags{}, g_vb_solid_face_colors_Data_Size,
                                      vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive};

    m_cubeEngBuf = CreateNewBuffer(device, kCubeBufferID, vertBufferCI, reinterpret_cast<void **>(&m_vertData),
                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    memcpy(m_vertData, g_vb_solid_face_colors_Data, g_vb_solid_face_colors_Data_Size);

    device->unmapMemory(m_cubeEngBuf.devMem);

    device->bindBufferMemory(m_cubeEngBuf.buffer, m_cubeEngBuf.devMem, 0);
}

void ControlEnginePipeline::SetupFragmentBuffer(const vk::UniqueDevice &device)
{
    vk::SamplerCreateInfo samplerCI = vk::SamplerCreateInfo();
    samplerCI.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerCI.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerCI.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerCI.mipLodBias = 0.0;
    samplerCI.maxAnisotropy = 1;
    samplerCI.minLod = 0.0;
    samplerCI.maxLod = 0.0;
    samplerCI.borderColor = vk::BorderColor::eFloatOpaqueWhite;

    m_textureSampler = CreateNewSampler(device, kTextureSamplerID, 1, 0, samplerCI);
}

void ControlEnginePipeline::Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer,
                                   vk::Extent2D windowExtents)
{
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
    cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets,
                                 nullptr);

    constexpr const vk::DeviceSize offsets[1]{0};
    cmdBuffer.bindVertexBuffers(0, 1, &m_cubeEngBuf.buffer, offsets);
}

} // namespace Pipelines
} // namespace Engine